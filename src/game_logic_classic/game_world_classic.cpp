/* Copyright (C) 2022, Nikolai Wuttke. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "game_world_classic.hpp"

#include "assets/file_utils.hpp"
#include "assets/resource_loader.hpp"
#include "base/spatial_types_printing.hpp"
#include "base/string_utils.hpp"
#include "base/warnings.hpp"
#include "data/strings.hpp"
#include "data/unit_conversions.hpp"
#include "engine/random_number_generator.hpp"
#include "frontend/game_service_provider.hpp"
#include "frontend/user_profile.hpp"
#include "game_logic_common/utils.hpp"
#include "renderer/upscaling.hpp"
#include "renderer/viewport_utils.hpp"

#include "actors.h"
#include "game.h"

#include <loguru.hpp>


using namespace rigel;
using rigel::game_logic::detail::Bridge;
using rigel::game_logic::detail::State;


namespace
{

word* allocateWordBuffer(
  Context* ctx,
  const size_t size,
  const ChunkType chunkType)
{
  return static_cast<word*>(MM_PushChunk(ctx, word(size), chunkType));
}


Bridge& getBridge(Context* ctx)
{
  return *static_cast<Bridge*>(ctx->pRigelBridge);
}


static data::InventoryItemType convertItemType(const word id)
{
  using IT = data::InventoryItemType;

  switch (id)
  {
    case ACT_CIRCUIT_CARD:
      return IT::CircuitBoard;

    case ACT_BLUE_KEY:
      return IT::BlueKey;

    case ACT_RAPID_FIRE_ICON:
      return IT::RapidFire;

    case ACT_SPECIAL_HINT_GLOBE_ICON:
      return IT::SpecialHintGlobe;

    case ACT_CLOAKING_DEVICE_ICON:
      return IT::CloakingDevice;
  }

  assert(false);
  return IT::RapidFire;
}


data::map::TileIndex convertTileIndex(const uint16_t rawIndex)
{
  if (rawIndex & 0x8000)
  {
    // Extract the solid index, discard the masked one.
    return rawIndex & 0x3FF;
  }
  else
  {
    return assets::convertTileIndex(rawIndex);
  }
}


} // namespace


//
// Hook functions - functions called by the game code
//

byte RandomNumber(Context* ctx)
{
  ctx->gmRngIndex++;
  return byte(rigel::engine::RANDOM_NUMBER_TABLE[ctx->gmRngIndex]);
}


void PlaySound(Context* ctx, int16_t id)
{
  getBridge(ctx).mpServiceProvider->playSound(
    static_cast<rigel::data::SoundId>(id));
}


void StopMusic(Context* ctx)
{
  getBridge(ctx).mpServiceProvider->stopMusic();
}


void SetScreenShift(Context* ctx, byte amount)
{
  getBridge(ctx).mScreenShift = amount;
}


void HUD_ShowOnRadar(Context* ctx, word x, word y)
{
  int16_t x1 = ctx->plPosX - 17;
  int16_t y1 = ctx->plPosY - 17;

  if (
    int16_t(x) > x1 && x < ctx->plPosX + 16 && int16_t(y) > y1 &&
    y < ctx->plPosY + 16)
  {
    x1 = x - ctx->plPosX;
    y1 = y - ctx->plPosY;

    getBridge(ctx).mRadarDots.push_back({x1, y1});
  }
}


void SetPixel(Context* ctx, word x, word y, byte color)
{
  getBridge(ctx).mPixelsToDraw.push_back(
    game_logic::detail::PixelDrawCmd{x, y, color});
}


void DrawTileDebris(Context* ctx, word tileIndex, word x, word y)
{
  getBridge(ctx).mTileDebrisToDraw.push_back(
    game_logic::detail::TileDrawCmd{tileIndex, x, y});
}


word Map_GetTile(Context* ctx, word x, word y)
{
  if (int16_t(y) < 0)
  {
    return 0;
  }

  return *(ctx->mapData + x + (y << ctx->mapWidthShift));
}


void Map_SetTile(Context* ctx, word tileIndex, word x, word y)
{
  const auto originalIndex = *(ctx->mapData + x + (y << ctx->mapWidthShift));
  *(ctx->mapData + x + (y << ctx->mapWidthShift)) = tileIndex;

  if (tileIndex != originalIndex)
  {
    getBridge(ctx).mpMap->setTileAt(0, x, y, convertTileIndex(tileIndex));

    if (tileIndex & 0x8000)
    {
      const auto maskedIndex =
        ((tileIndex & 0x7C00) >> 10) + data::GameTraits::CZone::numSolidTiles;
      getBridge(ctx).mpMap->setTileAt(
        1, x, y, static_cast<data::map::TileIndex>(maskedIndex));
    }
    else if (tileIndex == 0)
    {
      getBridge(ctx).mpMap->setTileAt(1, x, y, 0);
    }

    getBridge(ctx).mpMapRenderer->markAsChanged({x, y});
  }
}


void ShowInGameMessage(Context* ctx, const MessageId id)
{
  const auto msg = [id]() {
    switch (id)
    {
      case MID_DESTROYED_EVERYTHING:
        return data::Messages::DestroyedEverything;
      case MID_OH_WELL:
        return data::Messages::LettersCollectedWrongOrder;
      case MID_ACCESS_GRANTED:
        return data::Messages::AccessGranted;
      case MID_OPENING_DOOR:
        return data::Messages::OpeningDoor;
      case MID_INVINCIBLE:
        return data::Messages::FoundCloak;
      case MID_HINT_GLOBE:
        return data::Messages::FoundSpecialHintGlobe;
      case MID_CLOAK_DISABLING:
        return data::Messages::CloakTimingOut;
      case MID_RAPID_FIRE_DISABLING:
        return data::Messages::RapidFireTimingOut;
      case MID_SECTOR_SECURE:
        return data::Messages::FoundRespawnBeacon;
      case MID_FORCE_FIELD_DESTROYED:
        return data::Messages::ForceFieldDestroyed;

      default:
        return "";
    }
  }();

  getBridge(ctx).mpMessageDisplay->setMessage(msg);
}


void ShowLevelSpecificHint(Context* ctx)
{
  auto& bridge = getBridge(ctx);

  if (
    auto hintText =
      bridge.mLevelHints.getHint(ctx->gmCurrentEpisode, ctx->gmCurrentLevel))
  {
    bridge.mpMessageDisplay->setMessage(
      *hintText, ui::MessagePriority::HintMachineMessage);
  }
}


void ShowTutorial(Context* ctx, TutorialId index)
{
  const auto id = static_cast<rigel::data::TutorialMessageId>(index);

  if (!getBridge(ctx).mpPlayerModel->tutorialMessages().hasBeenShown(id))
  {
    getBridge(ctx).mpMessageDisplay->setMessage(data::messageText(id));
    getBridge(ctx).mpPlayerModel->tutorialMessages().markAsShown(id);
  }
}


void AddInventoryItem(Context* ctx, word item)
{
  getBridge(ctx).mpPlayerModel->giveItem(convertItemType(item));
}


bool RemoveFromInventory(Context* ctx, word item)
{
  const auto type = convertItemType(item);

  if (!getBridge(ctx).mpPlayerModel->hasItem(type))
  {
    return false;
  }

  getBridge(ctx).mpPlayerModel->removeItem(type);
  return true;
}


void DrawActor(
  Context* ctx,
  word id,
  word frame,
  word x,
  word y,
  word drawStyle)
{
  getBridge(ctx).mSpritesToDraw.push_back(
    game_logic::detail::SpriteDrawCmd{id, frame, x, y, drawStyle});
}


void DrawWaterArea(Context* ctx, word left, word top, word animStep)
{
  getBridge(ctx).mWaterAreasToDraw.push_back(
    game_logic::detail::WaterAreaDrawCmd{left, top, animStep});
}


void RaiseError(Context* ctx, const char* msg)
{
  getBridge(ctx).mpErrorMessage = msg;
}


namespace rigel::game_logic
{

namespace
{

const base::Color SCREEN_FLASH_COLORS[] = {
  data::GameTraits::INGAME_PALETTE[0],
  data::GameTraits::INGAME_PALETTE[15],
  data::GameTraits::INGAME_PALETTE[7],
  data::GameTraits::INGAME_PALETTE[0]};


void relayInput(const PlayerInput& input, State* pState)
{
  pState->inputMoveUp = input.mUp;
  pState->inputMoveDown = input.mDown;
  pState->inputMoveLeft = input.mLeft;
  pState->inputMoveRight = input.mRight;
  pState->inputFire = input.mFire.mIsPressed || input.mFire.mWasTriggered;
  pState->inputJump = input.mJump.mIsPressed || input.mJump.mWasTriggered;
}


void relayPlayerModel(const data::PlayerModel& playerModel, State* pState)
{
  pState->plWeapon = static_cast<byte>(playerModel.weapon());
  pState->plScore = dword(playerModel.score());
  pState->plAmmo = byte(playerModel.ammo());
  pState->plHealth = byte(playerModel.health());
}


std::unique_ptr<State> createState(
  const data::GameSessionId& sessionId,
  const assets::ResourceLoader& resources,
  const data::PlayerModel& playerModel,
  Bridge* pBridge)
{
  auto pState = std::make_unique<State>();

  pState->pRigelBridge = pBridge;

  MM_Init(pState.get());
  InitParticleSystem(pState.get());

  // Recreate the effect of the original game's LoadActorInfo() function
  {
    const auto actorInfo = resources.file("ACTRINFO.MNI");
    pState->gfxActorInfoData =
      allocateWordBuffer(pState.get(), actorInfo.size(), CT_COMMON);
    std::memcpy(pState->gfxActorInfoData, actorInfo.data(), actorInfo.size());
  }

  pState->gmBeaconActivated = false;
  ResetGameState(pState.get());

  pState->gmCurrentLevel = byte(sessionId.mLevel);
  pState->gmCurrentEpisode = byte(sessionId.mEpisode);
  pState->gmDifficulty = byte(sessionId.mDifficulty) + 1;

  relayPlayerModel(playerModel, pState.get());

  return pState;
}

} // namespace


Bridge::Bridge(
  const assets::ResourceLoader& resources,
  data::map::Map* pMap,
  IGameServiceProvider* pServiceProvider,
  ui::IngameMessageDisplay* pMessageDisplay,
  data::PlayerModel* pPlayerModel)
  : mLevelHints(resources.loadHintMessages())
  , mpMap(pMap)
  , mpServiceProvider(pServiceProvider)
  , mpMessageDisplay(pMessageDisplay)
  , mpPlayerModel(pPlayerModel)
{
}


void Bridge::resetForNewFrame()
{
  mSpritesToDraw.clear();
  mPixelsToDraw.clear();
  mTileDebrisToDraw.clear();
  mWaterAreasToDraw.clear();
  mRadarDots.clear();
}


struct GameWorld_Classic::QuickSaveData
{
  QuickSaveData(
    const data::PlayerModel& playerModel,
    const data::map::Map& map,
    const State& state)
    : mPlayerModel(playerModel)
    , mMap(map)
    , mState(state)
  {
  }

  data::PlayerModel mPlayerModel;
  data::map::Map mMap;
  State mState;
};


GameWorld_Classic::GameWorld_Classic(
  data::PlayerModel* pPlayerModel,
  const data::GameSessionId& sessionId,
  GameMode::Context context,
  std::optional<base::Vec2> playerPositionOverride,
  bool showWelcomeMessage,
  const PlayerInput& initialInput)
  : mpRenderer(context.mpRenderer)
  , mpServiceProvider(context.mpServiceProvider)
  , mUiSpriteSheet(
      renderer::Texture{mpRenderer, context.mpResources->loadUiSpriteSheet()},
      data::GameTraits::viewportSize,
      mpRenderer)
  , mTextRenderer(&mUiSpriteSheet, mpRenderer, *context.mpResources)
  , mpPlayerModel(pPlayerModel)
  , mpOptions(&context.mpUserProfile->mOptions)
  , mpResources(context.mpResources)
  , mpSpriteFactory(context.mpSpriteFactory)
  , mImageIdTable(engine::buildImageIdTable(*context.mpResources))
  , mSessionId(sessionId)
  , mPlayerModelAtLevelStart(*mpPlayerModel)
  , mHudRenderer(
      sessionId.mLevel + 1,
      mpOptions,
      mpRenderer,
      &mUiSpriteSheet,
      renderer::Texture{
        mpRenderer,
        context.mpResources->loadWideHudFrameImage()},
      renderer::Texture{
        mpRenderer,
        context.mpResources->loadUltrawideHudFrameImage()},
      mpSpriteFactory)
  , mMessageDisplay(mpServiceProvider, &mTextRenderer)
  , mSpecialEffects(mpRenderer, *mpOptions)
  , mLowResLayer(
      mpRenderer,
      data::GameTraits::viewportWidthPx,
      data::GameTraits::viewportHeightPx)
  , mPreviousWindowSize(mpRenderer->windowSize())
  , mPerElementUpscalingWasEnabled(mpOptions->mPerElementUpscalingEnabled)
  , mBridge(
      *context.mpResources,
      &mMap,
      mpServiceProvider,
      &mMessageDisplay,
      mpPlayerModel)
  , mpState(
      createState(sessionId, *context.mpResources, *mpPlayerModel, &mBridge))
{
  LOG_SCOPE_FUNCTION(INFO);

  loadLevel(sessionId);

  if (playerPositionOverride)
  {
    mpState->plPosX = word(playerPositionOverride->x);
    mpState->plPosY = word(playerPositionOverride->y);
  }

  CenterViewOnPlayer(mpState.get());

  if (showWelcomeMessage)
  {
    mMessageDisplay.setMessage(data::Messages::WelcomeToDukeNukem2);
  }

  // This also shows the "earthquake" message, if applicable
  updateGameLogic(initialInput);

  if (mpState->gmRadarDishesLeft)
  {
    mMessageDisplay.setMessage(data::Messages::FindAllRadars);
  }

  LOG_F(
    INFO,
    "Level %d (episode %d) successfully loaded (classic mode)",
    sessionId.mLevel + 1,
    sessionId.mEpisode + 1);
}


GameWorld_Classic::~GameWorld_Classic() = default;


bool GameWorld_Classic::levelFinished() const
{
  return mpState->gmGameState == GS_LEVEL_FINISHED ||
    mpState->gmGameState == GS_EPISODE_FINISHED;
}


std::set<data::Bonus> GameWorld_Classic::achievedBonuses() const
{
  std::set<data::Bonus> result;

  if (
    mpState->gmCamerasDestroyed == mpState->gmCamerasInLevel &&
    mpState->gmCamerasDestroyed)
  {
    result.insert(data::Bonus::DestroyedAllCameras);
  }

  if (!mpState->gmPlayerTookDamage)
  {
    result.insert(data::Bonus::NoDamageTaken);
  }

  if (
    mpState->gmWeaponsCollected == mpState->gmWeaponsInLevel &&
    mpState->gmWeaponsCollected)
  {
    result.insert(data::Bonus::CollectedEveryWeapon);
  }

  if (
    mpState->gmMerchCollected == mpState->gmMerchInLevel &&
    mpState->gmMerchCollected)
  {
    result.insert(data::Bonus::CollectedAllMerchandise);
  }

  if (
    mpState->gmTurretsDestroyed == mpState->gmTurretsInLevel &&
    mpState->gmTurretsDestroyed)
  {
    result.insert(data::Bonus::DestroyedAllSpinningLaserTurrets);
  }

  // [BUG] ?? Unlike the other bonuses, 6 and 7 are granted if the level never
  // contained any bomb boxes or bonus globes (orbs) to begin with. Not sure if
  // this is intentional or an oversight.

  if (mpState->gmBombBoxesLeft == 0)
  {
    result.insert(data::Bonus::DestroyedAllFireBombs);
  }

  if (mpState->gmOrbsLeft == 0)
  {
    result.insert(data::Bonus::ShotAllBonusGlobes);
  }

  return result;
}


bool GameWorld_Classic::needsPerElementUpscaling() const
{
  return mpSpriteFactory->hasHighResReplacements() ||
    mMapRenderer->hasHighResReplacements() || mUiSpriteSheet.isHighRes();
}


void GameWorld_Classic::updateGameLogic(const PlayerInput& input)
{
  mMapRenderer->updateAnimatedMapTiles();

  mBridge.resetForNewFrame();

  const auto beaconWasActive = mpState->gmBeaconActivated;
  const auto bossWasActive = mpState->gmBossActivated;

  relayInput(input, mpState.get());

  // Run original logic
  UpdateAndDrawGame(mpState.get());

  if (mBridge.mpErrorMessage)
  {
    throw std::runtime_error(mBridge.mpErrorMessage);
  }

  mHudRenderer.updateAnimation();
  mMessageDisplay.update();

  // When teleporting, we don't want to sync the backdrop here, since that would
  // make the new backdrop already visible while fading out from the starting
  // location. We sync the backdrop after the fade-out, in
  // processEndOfFrameActions().
  if (!mpState->gmIsTeleporting)
  {
    syncBackdrop();
  }

  syncPlayerModel();

  if (mpState->gmBeaconActivated && !beaconWasActive)
  {
    mCheckpointState = mpPlayerModel->makeCheckpoint();
  }

  if (mpState->gmBossActivated && !bossWasActive)
  {
    mpServiceProvider->playMusic(mMusicFile);
  }

  mMapRenderer->rebuildChangedBlocks(mMap);
}


void GameWorld_Classic::render(float)
{
  auto drawTopRow = [&]() {
    if (mpState->gmBossActivated)
    {
      ui::drawBossHealthBar(
        mpState->gmBossHealth,
        mpState->gmBossStartingHealth,
        data::GameTraits::inGameViewportSize.width,
        mTextRenderer,
        mUiSpriteSheet);
    }
    else
    {
      mMessageDisplay.render();
    }
  };


  if (
    mpOptions->mPerElementUpscalingEnabled != mPerElementUpscalingWasEnabled ||
    mPreviousWindowSize != mpRenderer->windowSize())
  {
    mSpecialEffects.rebuildBackgroundBuffer(*mpOptions);
  }

  {
    auto saved = setupIngameViewport(mpRenderer, mBridge.mScreenShift);

    drawWorld();
    mHudRenderer.renderClassicHud(*mpPlayerModel, mBridge.mRadarDots);
  }

  auto saved = renderer::saveState(mpRenderer);
  renderer::setLocalTranslation(
    mpRenderer,
    {mBridge.mScreenShift + data::GameTraits::inGameViewportOffset.x, 0});
  drawTopRow();

  mPerElementUpscalingWasEnabled = mpOptions->mPerElementUpscalingEnabled;
  mPreviousWindowSize = mpRenderer->windowSize();
}


void GameWorld_Classic::drawWorld()
{
  if (mpState->gfxFlashScreen && mpOptions->mEnableScreenFlashes)
  {
    mpRenderer->clear(SCREEN_FLASH_COLORS[mpState->gfxScreenFlashColor]);
    return;
  }

  const auto region = base::Rect<int>{
    {mpState->gmCameraPosX, mpState->gmCameraPosY},
    data::GameTraits::mapViewportSize};


  auto drawParticles = [&]() {
    for (const auto& request : mBridge.mPixelsToDraw)
    {
      mpRenderer->drawPoint(
        {request.x, request.y},
        data::GameTraits::INGAME_PALETTE[request.color]);
    }
  };


  if (mpOptions->mPerElementUpscalingEnabled)
  {
    drawMapAndSprites(region);

    {
      const auto saved = mLowResLayer.bindAndReset();
      mpRenderer->clear({0, 0, 0, 0});
      drawParticles();
    }

    mLowResLayer.render(0, 0);
  }
  else
  {
    drawMapAndSprites(region);
    drawParticles();
  }
}


void GameWorld_Classic::drawMapAndSprites(const base::Rect<int>& region)
{
  using namespace detail;

  auto destRect = [&](const SpriteDrawCmd& request) {
    RIGEL_DISABLE_WARNINGS

    // For C macros used below
    auto ctx = mpState.get();

    const auto offset =
      mpState->gfxActorInfoData[request.id] + request.frame * 8;
    const auto topLeft = base::Vec2{request.x, request.y} -
      base::Vec2{mpState->gmCameraPosX, mpState->gmCameraPosY} -
      base::Vec2{0, AINFO_HEIGHT(offset) - 1} +
      base::Vec2{AINFO_X_OFFSET(offset), AINFO_Y_OFFSET(offset)};
    return base::Rect<int>{
      data::tilesToPixels(topLeft),
      data::tilesToPixels(
        base::Size{AINFO_WIDTH(offset), AINFO_HEIGHT(offset)})};

    RIGEL_RESTORE_WARNINGS
  };


  auto drawSprite = [&](const SpriteDrawCmd& request) {
    const auto imageId = mImageIdTable[request.id] + request.frame;

    if (request.drawStyle == DS_WHITEFLASH)
    {
      const auto innerGuard = renderer::saveState(mpRenderer);
      mpRenderer->setOverlayColor(data::GameTraits::INGAME_PALETTE[15]);
      mpSpriteFactory->textureAtlas().draw(imageId, destRect(request));
    }
    else if (request.drawStyle == DS_TRANSLUCENT)
    {
      const auto [textureId, texCoords] =
        mpSpriteFactory->textureAtlas().drawData(imageId);
      mSpecialEffects.drawCloakEffect(textureId, texCoords, destRect(request));
    }
    else
    {
      mpSpriteFactory->textureAtlas().draw(imageId, destRect(request));
    }
  };


  auto drawBackdrop = [&]() {
    if (
      mpState->gmReactorDestructionStep &&
      mpState->gmReactorDestructionStep < 14 &&
      mpState->gfxCurrentDisplayPage && mpOptions->mEnableScreenFlashes)
    {
      mpRenderer->drawFilledRectangle(
        {{}, data::tilesToPixels(data::GameTraits::mapViewportSize)},
        data::GameTraits::INGAME_PALETTE[15]);
    }
    else
    {
      mMapRenderer->renderBackdrop(
        base::cast<float>(region.topLeft), region.size);
    }
  };


  auto drawBackgroundLayers = [&]() {
    mMapRenderer->renderBackground(region.topLeft, region.size);

    for (const auto& request : mBridge.mSpritesToDraw)
    {
      if (request.drawStyle != DS_INVISIBLE && request.drawStyle != DS_IN_FRONT)
      {
        drawSprite(request);
      }
    }
  };


  auto drawForegroundLayers = [&]() {
    mMapRenderer->renderForeground(region.topLeft, region.size);

    for (const auto& request : mBridge.mSpritesToDraw)
    {
      if (request.drawStyle != DS_INVISIBLE && request.drawStyle == DS_IN_FRONT)
      {
        drawSprite(request);
      }
    }

    for (const auto& request : mBridge.mTileDebrisToDraw)
    {
      mMapRenderer->renderSingleTile(
        convertTileIndex(request.tileIndex),
        data::tilesToPixels(base::Vec2{request.x, request.y}));
    }
  };

  updateVisibleWaterAreas();

  const auto cloakEffectSpritesVisible = std::any_of(
    std::begin(mBridge.mSpritesToDraw),
    std::end(mBridge.mSpritesToDraw),
    [](const SpriteDrawCmd& request) {
      return request.drawStyle == DS_TRANSLUCENT;
    });

  if (mVisibleWaterAreas.empty() && !cloakEffectSpritesVisible)
  {
    drawBackdrop();
    drawBackgroundLayers();
    drawForegroundLayers();
  }
  else
  {
    {
      auto saved = mSpecialEffects.bindBackgroundBuffer();
      drawBackdrop();
      drawBackgroundLayers();
    }

    mSpecialEffects.drawBackgroundBuffer();

    if (!mVisibleWaterAreas.empty())
    {
      // In the original game logic, each water area actor has its own
      // animation step. But all actors start out with the same step value, and
      // they are all updated each frame. Consequently, the animation step is
      // effectively a single global value, even if not represented as such.
      // Here, we look for the first water area with an animation step, and use
      // that as the global value for all water areas.
      const auto iFirstAnimatedArea = std::find_if(
        mBridge.mWaterAreasToDraw.begin(),
        mBridge.mWaterAreasToDraw.end(),
        [](const WaterAreaDrawCmd& cmd) { return cmd.animStep != 0; });

      const auto waterAnimStep =
        iFirstAnimatedArea != mBridge.mWaterAreasToDraw.end()
        ? iFirstAnimatedArea->animStep - 1
        : 0;
      mSpecialEffects.drawWaterEffect(mVisibleWaterAreas, waterAnimStep);
    }

    drawForegroundLayers();
  }
}


void GameWorld_Classic::updateVisibleWaterAreas()
{
  mVisibleWaterAreas.clear();

  const auto cameraPosition =
    base::Vec2{mpState->gmCameraPosX, mpState->gmCameraPosY};
  const auto screenBbox =
    base::Rect<int>{cameraPosition, data::GameTraits::mapViewportSize};

  for (const auto& request : mBridge.mWaterAreasToDraw)
  {
    const auto pos = base::Vec2{request.left, request.top};

    if (!screenBbox.intersects({pos, {2, 2}}))
    {
      continue;
    }

    const auto pxPos = data::tilesToPixels(pos - cameraPosition);

    if (request.animStep == 0)
    {
      mVisibleWaterAreas.push_back(engine::WaterEffectArea{
        pxPos, data::tilesToPixels(base::Size{2, 2}), false});
    }
    else
    {
      const auto size = data::tilesToPixels(base::Size{2, 1});
      mVisibleWaterAreas.push_back(engine::WaterEffectArea{pxPos, size, true});
      mVisibleWaterAreas.push_back(engine::WaterEffectArea{
        pxPos + data::tilesToPixels(base::Vec2{0, 1}), size, false});
    }
  }
}


void GameWorld_Classic::processEndOfFrameActions()
{
  if (mpState->gmIsTeleporting)
  {
    mpServiceProvider->fadeOutScreen();

    mpState->plPosY = mpState->gmTeleportTargetPosY;
    mpState->plPosX = mpState->gmTeleportTargetPosX + 1;
    CenterViewOnPlayer(mpState.get());

    syncBackdrop();

    updateGameLogic({});
    render(1.0f);

    mpServiceProvider->fadeInScreen();

    mpState->gmIsTeleporting = false;
  }

  if (mpState->gmGameState == GS_PLAYER_DIED)
  {
    mpServiceProvider->fadeOutScreen();

    ResetGameState(mpState.get());

    if (mpState->gmBeaconActivated)
    {
      mpPlayerModel->restoreFromCheckpoint(*mCheckpointState);

      mpState->plPosX = mpState->gmBeaconPosX;
      mpState->plPosY = mpState->gmBeaconPosY;
      mpState->plActorId = ACT_DUKE_R;
    }
    else
    {
      *mpPlayerModel = mPlayerModelAtLevelStart;
      loadLevel(mSessionId);

      if (mpState->gmRadarDishesLeft)
      {
        mMessageDisplay.setMessage(data::Messages::FindAllRadars);
      }
    }

    syncBackdrop();

    relayPlayerModel(*mpPlayerModel, mpState.get());

    CenterViewOnPlayer(mpState.get());

    updateGameLogic({});
    render(1.0f);

    mpServiceProvider->fadeInScreen();
  }

  mBridge.mScreenShift = 0;
}


void GameWorld_Classic::updateBackdropAutoScrolling(const engine::TimeDelta dt)
{
  mMapRenderer->updateBackdropAutoScrolling(dt);
}


bool GameWorld_Classic::isPlayerInShip() const
{
  return mpState->plState == PS_USING_SHIP;
}


void GameWorld_Classic::toggleGodMode()
{
  mpState->sysTecMode = !mpState->sysTecMode;
}


bool GameWorld_Classic::isGodModeOn() const
{
  return mpState->sysTecMode;
}


void GameWorld_Classic::activateFullHealthCheat()
{
  mpPlayerModel->resetHealthAndScore();
  relayPlayerModel(*mpPlayerModel, mpState.get());
}


void GameWorld_Classic::activateGiveItemsCheat()
{
  word i;
  word weapons[] = {0, 0, 0};
  byte weaponsFound = 0;

  mpState->gmRadarDishesLeft = 0;

  for (i = 0; i < mpState->gmNumActors; i++)
  {
    ActorState* actor = mpState->gmActorStates + i;

    if (actor->id == ACT_RADAR_DISH)
    {
      actor->deleted = true;
    }

    if (weaponsFound < 3 && actor->id == ACT_GREEN_BOX)
    {
      if (actor->var2 == ACT_ROCKET_LAUNCHER)
      {
        weapons[weaponsFound] = i;
        weaponsFound++;
      }
      else if (actor->var2 == ACT_LASER)
      {
        weapons[weaponsFound] = i;
        weaponsFound++;
      }
      else if (actor->var2 == ACT_FLAME_THROWER)
      {
        weapons[weaponsFound] = i;
        weaponsFound++;
      }
    }
    else if (actor->id == ACT_WHITE_BOX && !actor->deleted)
    {
      switch (actor->var2)
      {
        case ACT_BLUE_KEY:
          AddInventoryItem(mpState.get(), ACT_BLUE_KEY);
          actor->deleted = true;
          break;

        case ACT_CIRCUIT_CARD:
          AddInventoryItem(mpState.get(), ACT_CIRCUIT_CARD);
          actor->deleted = true;
          break;

        case ACT_CLOAKING_DEVICE:
          if (mpState->plCloakTimeLeft == 0)
          {
            AddInventoryItem(mpState.get(), ACT_CLOAKING_DEVICE_ICON);
            mpState->plCloakTimeLeft = CLOAK_TIME;
          }
          break;
      }
    }

    if (weaponsFound != 0)
    {
      word pickupHandle = weapons[weaponsFound - 1];
      ActorState* weaponPickupActor = mpState->gmActorStates + pickupHandle;

      if (weaponPickupActor->var2 != ACT_FLAME_THROWER)
      {
        mpState->plAmmo = MAX_AMMO;
      }
      else
      {
        mpState->plAmmo = MAX_AMMO_FLAMETHROWER;
      }

      mpState->plWeapon = byte(weaponPickupActor->var3);
    }
  }

  syncPlayerModel();
}


void GameWorld_Classic::quickSave()
{
  if (!mpOptions->mQuickSavingEnabled || mpState->gmGameState == GS_PLAYER_DIED)
  {
    return;
  }

  LOG_F(INFO, "Creating quick save");

  mpQuickSave = std::make_unique<QuickSaveData>(*mpPlayerModel, mMap, *mpState);

  mMessageDisplay.setMessage(
    data::Messages::QuickSaved, ui::MessagePriority::Menu);

  LOG_F(INFO, "Quick save created");
}


void GameWorld_Classic::quickLoad()
{
  if (!canQuickLoad())
  {
    return;
  }

  LOG_F(INFO, "Loading quick save");

  *mpPlayerModel = mpQuickSave->mPlayerModel;
  mMap = mpQuickSave->mMap;
  *mpState = mpQuickSave->mState;

  mMapRenderer->rebuildAllBlocks(mMap);

  syncBackdrop();

  mMessageDisplay.setMessage(
    data::Messages::QuickLoaded, ui::MessagePriority::Menu);

  LOG_F(INFO, "Quick save loaded");
}


bool GameWorld_Classic::canQuickLoad() const
{
  return mpOptions->mQuickSavingEnabled && mpQuickSave;
}


void GameWorld_Classic::printDebugText(std::ostream& stream) const
{
  const auto cameraPos =
    base::Vec2{mpState->gmCameraPosX, mpState->gmCameraPosY};
  const auto playerPos = base::Vec2{mpState->plPosX, mpState->plPosY};

  const auto numActors = std::count_if(
    std::begin(mpState->gmActorStates),
    std::begin(mpState->gmActorStates) + mpState->gmNumActors,
    [](const ActorState& actor) { return !actor.deleted; });
  const auto numMapParts = std::count_if(
    std::begin(mpState->gmMovingMapParts),
    std::begin(mpState->gmMovingMapParts) + mpState->gmNumMovingMapParts,
    [](const MovingMapPartState& part) {
      return part.type == 0 || part.type >= 99;
    });
  const auto numEffects = std::count_if(
    std::begin(mpState->gmEffectStates),
    std::end(mpState->gmEffectStates),
    [](const EffectState& effect) { return effect.active != 0; });
  const auto numShots = std::count_if(
    std::begin(mpState->gmPlayerShotStates),
    std::end(mpState->gmPlayerShotStates),
    [](const PlayerShot& shot) { return shot.active != 0; });
  const auto numParticleGroups = std::count_if(
    std::begin(mpState->psParticleGroups),
    std::end(mpState->psParticleGroups),
    [](const ParticleGroup& group) { return group.timeAlive != 0; });
  const auto numTileDebris = mpState->gmExplodingSectionTicksElapsed != 0
    ? (mpState->gmExplodingSectionRight - mpState->gmExplodingSectionLeft) *
      (mpState->gmExplodingSectionBottom - mpState->gmExplodingSectionTop)
    : 0;


  stream << "Scroll:   ";
  outputFixedWidth(stream, cameraPos, 4);
  stream << '\n';

  stream << "Player:   ";
  outputFixedWidth(stream, playerPos, 4);
  stream << '\n';

  auto printNumberAligned = [&](const char* label, const auto num) {
    stream << label << std::setw(3) << std::fixed << std::setprecision(2) << num
           << '\n';
  };

  printNumberAligned("Actors:          ", numActors);
  printNumberAligned("Map parts:       ", numMapParts);
  printNumberAligned("Effects:         ", numEffects);
  printNumberAligned("Player shots:    ", numShots);
  printNumberAligned("Particle groups: ", numParticleGroups);
  printNumberAligned("Tile debris:     ", numTileDebris);

  if (mpOptions->mPerElementUpscalingEnabled)
  {
    stream << "Hi-res mode ON\n";
  }
}


void GameWorld_Classic::loadLevel(const data::GameSessionId& sessionId)
{
  {
    const auto levelDataRaw = mpResources->file(
      assets::levelFileName(sessionId.mEpisode, sessionId.mLevel));

    assets::LeStreamReader reader(levelDataRaw);

    const dword headerSize = reader.readU16();

    if (headerSize >= sizeof(mpState->levelHeaderData))
    {
      throw std::runtime_error(
        "Level has too much data for Classic mode - use enhanced mode");
    }

    const auto czoneFile =
      mpResources->file(strings::trimRight(readFixedSizeString(reader, 13)));

    // Recreate the effect of the original code's LoadLevelHeader() function
    std::memcpy(
      mpState->levelHeaderData,
      levelDataRaw.data() + sizeof(std::uint16_t),
      headerSize);

    // Skip to number of actor words, at byte 43 in the header data. We've
    // already read 13 bytes to get the CZone file name, so we just need to skip
    // over 30 more in order to get to that index.
    reader.skipBytes(30);

    mpState->levelActorListSize = reader.readU16();

    // Verify that the actor list isn't larger than the header itself. The
    // number of actor words is at offset 43, the actor list itself starts at
    // offset 45. Since the list size is given as a number of words, we need to
    // multiply to get the size in bytes. The header has one more data word
    // after the actor list (the width of the map), so we compare against the
    // offset right before that which should be the end of the actor list.
    //
    // We do this verification here because SpawnLevelActors() doesn't do any
    // range checking, so we have to make sure our data is valid before giving
    // it to the unsafe original code.
    if (
      45 + mpState->levelActorListSize * sizeof(word) >
      headerSize - sizeof(word))
    {
      throw std::runtime_error("Invalid or corrupt level file");
    }

    // LoadMapData() in the original code
    mpState->mapData = allocateWordBuffer(mpState.get(), 65500, CT_MAP_DATA);
    std::memcpy(
      mpState->mapData,
      levelDataRaw.data() + headerSize + sizeof(std::uint16_t),
      65500);

    // LoadTileSetAttributes() in the original code
    mpState->gfxTilesetAttributes =
      allocateWordBuffer(mpState.get(), 3600, CT_CZONE);
    std::memcpy(mpState->gfxTilesetAttributes, czoneFile.data(), 3600);
  }

  // Now load the level file again using Rigel's functions, in order to get the
  // map data in the right format as needed by the MapRenderer. This also makes
  // it easier for us to parse the level flags.
  const auto levelData = assets::loadLevel(
    assets::levelFileName(sessionId.mEpisode, sessionId.mLevel),
    *mpResources,
    sessionId.mDifficulty);

  // SetMapSize() in the original code
  mpState->mapWidth = word(levelData.mMap.width());
  mpState->mapWidthShift = word(std::log(mpState->mapWidth) / std::log(2));
  mpState->mapBottom = word(levelData.mMap.height() - 1);
  mpState->mapViewportHeight = VIEWPORT_HEIGHT;

  using SM = data::map::BackdropScrollMode;
  using BSC = data::map::BackdropSwitchCondition;

  // ParseLevelFlags() in the original code
  mpState->mapParallaxHorizontal =
    levelData.mBackdropScrollMode == SM::ParallaxHorizontal;
  mpState->mapHasEarthquake = levelData.mEarthquake;
  mpState->mapHasReactorDestructionEvent =
    levelData.mBackdropSwitchCondition == BSC::OnReactorDestruction;
  mpState->mapSwitchBackdropOnTeleport =
    levelData.mBackdropSwitchCondition == BSC::OnTeleportation;

  mMap = std::move(levelData.mMap);

  mMapRenderer.emplace(
    mpRenderer,
    mMap,
    &mMap.attributeDict(),
    engine::MapRenderer::MapRenderData{
      std::move(levelData.mTileSetImage),
      std::move(levelData.mBackdropImage),
      std::move(levelData.mSecondaryBackdropImage),
      levelData.mBackdropScrollMode});
  mBridge.mpMapRenderer = &(*mMapRenderer);

  mMusicFile = levelData.mMusicFile;

  SpawnLevelActors(mpState.get());

  if (data::isBossLevel(mSessionId.mLevel))
  {
    mpServiceProvider->playMusic(BOSS_LEVEL_INTRO_MUSIC);
  }
  else
  {
    mpServiceProvider->playMusic(mMusicFile);
  }
}


void GameWorld_Classic::syncBackdrop()
{
  if (mpState->bdUseSecondary != mIsUsingSecondaryBackdrop)
  {
    mMapRenderer->switchBackdrops();
    mIsUsingSecondaryBackdrop = mpState->bdUseSecondary;
  }
}


void GameWorld_Classic::syncPlayerModel()
{
  using LT = data::CollectableLetterType;

  mpPlayerModel->mWeapon = static_cast<data::WeaponType>(mpState->plWeapon);
  mpPlayerModel->mScore = mpState->plScore;
  mpPlayerModel->mAmmo = mpState->plAmmo;
  mpPlayerModel->mHealth = mpState->plHealth;

  mpPlayerModel->mCollectedLetters.clear();

  if (mpState->plCollectedLetters & 0x100)
  {
    mpPlayerModel->mCollectedLetters.push_back(LT::N);
  }

  if (mpState->plCollectedLetters & 0x200)
  {
    mpPlayerModel->mCollectedLetters.push_back(LT::U);
  }

  if (mpState->plCollectedLetters & 0x400)
  {
    mpPlayerModel->mCollectedLetters.push_back(LT::K);
  }

  if (mpState->plCollectedLetters & 0x800)
  {
    mpPlayerModel->mCollectedLetters.push_back(LT::E);
  }

  if (mpState->plCollectedLetters & 0x1000)
  {
    mpPlayerModel->mCollectedLetters.push_back(LT::M);
  }
}


} // namespace rigel::game_logic
