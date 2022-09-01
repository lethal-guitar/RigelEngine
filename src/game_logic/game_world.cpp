/* Copyright (C) 2019, Nikolai Wuttke. All rights reserved.
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

#include "game_world.hpp"

#include "assets/resource_loader.hpp"
#include "base/match.hpp"
#include "data/game_options.hpp"
#include "data/game_traits.hpp"
#include "data/map.hpp"
#include "data/sound_ids.hpp"
#include "data/strings.hpp"
#include "data/unit_conversions.hpp"
#include "engine/base_components.hpp"
#include "engine/entity_tools.hpp"
#include "engine/graphical_effects.hpp"
#include "engine/motion_smoothing.hpp"
#include "engine/physical_components.hpp"
#include "frontend/game_service_provider.hpp"
#include "frontend/user_profile.hpp"
#include "game_logic/actor_tag.hpp"
#include "game_logic/behavior_controller.hpp"
#include "game_logic/collectable_components.hpp"
#include "game_logic/dynamic_geometry_components.hpp"
#include "game_logic/enemies/dying_boss.hpp"
#include "game_logic/world_state.hpp"
#include "renderer/upscaling.hpp"
#include "renderer/viewport_utils.hpp"
#include "ui/menu_element_renderer.hpp"
#include "ui/utils.hpp"

#include <loguru.hpp>

#include <cassert>
#include <iomanip>
#include <sstream>


namespace rigel::game_logic
{

using namespace engine;
using namespace std;

using data::PlayerModel;
using engine::components::InterpolateMotion;
using engine::components::WorldPosition;


namespace
{

constexpr auto BOSS_LEVEL_INTRO_MUSIC = "CALM.IMF";

constexpr auto HEALTH_BAR_LABEL_START_X = 0;
constexpr auto HEALTH_BAR_LABEL_START_Y = 0;
constexpr auto HEALTH_BAR_TILE_INDEX = 4 * 40 + 1;
constexpr auto HEALTH_BAR_START_PX = base::Vec2{data::tilesToPixels(5), 0};


void drawBossHealthBar(
  const int health,
  const ui::MenuElementRenderer& textRenderer,
  const engine::TiledTexture& uiSpriteSheet)
{
  textRenderer.drawSmallWhiteText(
    HEALTH_BAR_LABEL_START_X, HEALTH_BAR_LABEL_START_Y, "BOSS");

  const auto healthBarSize = base::Size{health, data::GameTraits::tileSize};
  uiSpriteSheet.renderTileStretched(
    HEALTH_BAR_TILE_INDEX, {HEALTH_BAR_START_PX, healthBarSize});
}


int healthOrZero(entityx::Entity entity)
{
  using game_logic::components::Shootable;

  return entity.has_component<Shootable>()
    ? entity.component<Shootable>()->mHealth
    : 0;
}


[[nodiscard]] auto setupIngameViewport(
  renderer::Renderer* pRenderer,
  const int screenShakeOffsetX)
{
  auto saved = renderer::saveState(pRenderer);

  const auto offset =
    data::GameTraits::inGameViewportOffset + base::Vec2{screenShakeOffsetX, 0};
  renderer::setLocalTranslation(pRenderer, offset);
  renderer::setLocalClipRect(
    pRenderer, base::Rect<int>{{}, data::GameTraits::inGameViewportSize});

  return saved;
}


[[nodiscard]] auto setupIngameViewportWidescreen(
  renderer::Renderer* pRenderer,
  const renderer::WidescreenViewportInfo& info,
  const int screenShakeOffsetX)
{
  auto saved = renderer::saveState(pRenderer);

  const auto scale = pRenderer->globalScale();
  const auto offset =
    base::Vec2{screenShakeOffsetX, data::GameTraits::inGameViewportOffset.y};
  const auto newTranslation =
    renderer::scaleVec(offset, scale) + base::Vec2{info.mLeftPaddingPx, 0};
  pRenderer->setGlobalTranslation(newTranslation);
  pRenderer->setClipRect({});

  return saved;
}


auto viewportSizeWideScreen(
  renderer::Renderer* pRenderer,
  const data::GameOptions& options)
{
  const auto info = renderer::determineWidescreenViewport(pRenderer);
  const auto style =
    ui::effectiveHudStyle(options.mWidescreenHudStyle, pRenderer);
  const auto hudWidth =
    style == data::WidescreenHudStyle::Classic ? ui::HUD_WIDTH_RIGHT : 0;
  return base::Size{
    info.mWidthTiles - hudWidth, data::GameTraits::mapViewportSize.height};
}


[[nodiscard]] auto setupWidescreenTopRowViewport(
  renderer::Renderer* pRenderer,
  const renderer::WidescreenViewportInfo& info,
  const int screenShakeOffsetX)
{
  auto saved = renderer::saveState(pRenderer);

  const auto scale = pRenderer->globalScale();
  pRenderer->setGlobalTranslation(
    {base::round(info.mLeftPaddingPx + screenShakeOffsetX * scale.x),
     pRenderer->globalTranslation().y});
  pRenderer->setClipRect({});
  return saved;
}


std::vector<base::Vec2> collectRadarDots(
  entityx::EntityManager& entities,
  const base::Vec2& playerPosition)
{
  using engine::components::Active;
  using engine::components::WorldPosition;
  using game_logic::components::AppearsOnRadar;

  std::vector<base::Vec2> radarDots;

  entities.each<WorldPosition, AppearsOnRadar, Active>(
    [&](
      entityx::Entity,
      const WorldPosition& position,
      const AppearsOnRadar&,
      const Active&) {
      const auto positionRelativeToPlayer = position - playerPosition;
      if (ui::isVisibleOnRadar(positionRelativeToPlayer))
      {
        radarDots.push_back(positionRelativeToPlayer);
      }
    });

  return radarDots;
}


template <typename ValueT>
std::string vec2String(const base::Vec2T<ValueT>& vec, const int width)
{
  std::stringstream stream;
  // clang-format off
  stream
    << std::setw(width) << std::fixed << std::setprecision(2) << vec.x << ", "
    << std::setw(width) << std::fixed << std::setprecision(2) << vec.y;
  // clang-format on
  return stream.str();
}


std::vector<engine::WaterEffectArea> collectWaterEffectAreas(
  entityx::EntityManager& es,
  const base::Vec2& cameraPosition,
  const base::Size& viewportSize)
{
  using engine::components::BoundingBox;
  using game_logic::components::ActorTag;

  std::vector<engine::WaterEffectArea> result;

  const auto screenBox = BoundingBox{cameraPosition, viewportSize};

  es.each<ActorTag, WorldPosition, BoundingBox>([&](
                                                  entityx::Entity,
                                                  const ActorTag& tag,
                                                  const WorldPosition& position,
                                                  const BoundingBox& bbox) {
    using T = ActorTag::Type;

    const auto isWaterArea =
      tag.mType == T::AnimatedWaterArea || tag.mType == T::WaterArea;
    if (isWaterArea)
    {
      const auto worldSpaceBbox = engine::toWorldSpace(bbox, position);

      if (screenBox.intersects(worldSpaceBbox))
      {
        const auto topLeftPx =
          data::tilesToPixels(worldSpaceBbox.topLeft - cameraPosition);
        const auto sizePx = data::tilesToPixels(worldSpaceBbox.size);
        const auto hasAnimatedSurface = tag.mType == T::AnimatedWaterArea;

        result.push_back(
          engine::WaterEffectArea{{topLeftPx, sizePx}, hasAnimatedSurface});
      }
    }
  });

  return result;
}


base::Size clampedSectionSize(
  const base::Vec2& sectionStart,
  const base::Size& sectionSize,
  const data::map::Map& map)
{
  return {
    std::min(sectionSize.width, map.width() - sectionStart.x),
    std::min(sectionSize.height, map.height() - sectionStart.y)};
}

} // namespace


GameWorld::GameWorld(
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
      renderer::determineWidescreenViewport(mpRenderer).mWidthPx,
      data::GameTraits::viewportHeightPx)
  , mPreviousWindowSize(mpRenderer->windowSize())
  , mPreviousHudStyle(mpOptions->mWidescreenHudStyle)
  , mWidescreenModeWasOn(widescreenModeOn())
  , mPerElementUpscalingWasEnabled(mpOptions->mPerElementUpscalingEnabled)
  , mMotionSmoothingWasEnabled(mpOptions->mMotionSmoothing)
{
  LOG_SCOPE_FUNCTION(INFO);

  loadLevel(initialInput);

  if (playerPositionOverride)
  {
    mpState->mPlayer.position() = *playerPositionOverride;
    mpState->mCamera.centerViewOnPlayer();
    updateGameLogic(initialInput);
    mpState->mPreviousCameraPosition = mpState->mCamera.position();
  }

  if (showWelcomeMessage)
  {
    mMessageDisplay.setMessage(data::Messages::WelcomeToDukeNukem2);
  }

  // earth quake message overrides welcome message
  if (mpState->mEarthQuakeEffect)
  {
    showTutorialMessage(data::TutorialMessageId::EarthQuake);
  }

  // radar dish message overrides earth quake message
  if (mpState->mRadarDishCounter.radarDishesPresent())
  {
    mMessageDisplay.setMessage(data::Messages::FindAllRadars);
  }

  LOG_F(
    INFO,
    "Level %d (episode %d) successfully loaded",
    sessionId.mLevel + 1,
    sessionId.mEpisode + 1);
}


GameWorld::~GameWorld() = default;


bool GameWorld::levelFinished() const
{
  return mpState->mLevelFinished;
}


std::set<data::Bonus> GameWorld::achievedBonuses() const
{
  std::set<data::Bonus> bonuses;

  if (!mpState->mBonusInfo.mPlayerTookDamage)
  {
    bonuses.insert(data::Bonus::NoDamageTaken);
  }

  const auto counts = countBonusRelatedItems(
    const_cast<entityx::EntityManager&>(mpState->mEntities));

  if (mpState->mBonusInfo.mInitialCameraCount > 0 && counts.mCameraCount == 0)
  {
    bonuses.insert(data::Bonus::DestroyedAllCameras);
  }

  // NOTE: This is a bug (?) in the original game - if a level doesn't contain
  // any fire bombs, bonus 6 will be awarded, as if the player had destroyed
  // all fire bombs.
  if (counts.mFireBombCount == 0)
  {
    bonuses.insert(data::Bonus::DestroyedAllFireBombs);
  }

  if (
    mpState->mBonusInfo.mInitialMerchandiseCount > 0 &&
    counts.mMerchandiseCount == 0)
  {
    bonuses.insert(data::Bonus::CollectedAllMerchandise);
  }

  if (mpState->mBonusInfo.mInitialWeaponCount > 0 && counts.mWeaponCount == 0)
  {
    bonuses.insert(data::Bonus::CollectedEveryWeapon);
  }

  if (
    mpState->mBonusInfo.mInitialLaserTurretCount > 0 &&
    counts.mLaserTurretCount == 0)
  {
    bonuses.insert(data::Bonus::DestroyedAllSpinningLaserTurrets);
  }

  if (
    mpState->mBonusInfo.mInitialBonusGlobeCount ==
    mpState->mBonusInfo.mNumShotBonusGlobes)
  {
    bonuses.insert(data::Bonus::ShotAllBonusGlobes);
  }

  return bonuses;
}


void GameWorld::receive(const rigel::events::CheckPointActivated& event)
{
  mpState->mActivatedCheckpoint =
    CheckpointData{mpPlayerModel->makeCheckpoint(), event.mPosition};
  mMessageDisplay.setMessage(data::Messages::FoundRespawnBeacon);
}


void GameWorld::receive(const rigel::events::ExitReached& event)
{
  if (
    mpState->mRadarDishCounter.radarDishesPresent() && event.mCheckRadarDishes)
  {
    showTutorialMessage(data::TutorialMessageId::RadarsStillFunctional);
  }
  else
  {
    mpState->mLevelFinished = true;
  }
}


void GameWorld::receive(const rigel::events::HintMachineMessage& event)
{
  mMessageDisplay.setMessage(
    event.mText, ui::MessagePriority::HintMachineMessage);
}


void GameWorld::receive(const rigel::events::PlayerDied& event)
{
  mpState->mPlayerDied = true;
}


void GameWorld::receive(const rigel::events::PlayerTookDamage& event)
{
  mpState->mBonusInfo.mPlayerTookDamage = true;
}


void GameWorld::receive(const rigel::events::PlayerMessage& event)
{
  mMessageDisplay.setMessage(event.mText);
}


void GameWorld::receive(const rigel::events::PlayerTeleported& event)
{
  mpState->mTeleportTargetPosition = event.mNewPosition;
}


void GameWorld::receive(const rigel::events::ScreenFlash& event)
{
  flashScreen(event.mColor);
}


void GameWorld::receive(const rigel::events::ScreenShake& event)
{
  mpState->mScreenShakeOffsetX = event.mAmount;
}


void GameWorld::receive(const rigel::events::TutorialMessage& event)
{
  showTutorialMessage(event.mId);
}


void GameWorld::receive(const game_logic::events::ShootableKilled& event)
{
  using game_logic::components::ActorTag;
  auto entity = event.mEntity;
  if (!entity.has_component<ActorTag>())
  {
    return;
  }

  const auto& position = *entity.component<const WorldPosition>();

  using AT = ActorTag::Type;
  const auto type = entity.component<ActorTag>()->mType;
  switch (type)
  {
    case AT::Reactor:
      onReactorDestroyed(position);
      break;

    case AT::ShootableBonusGlobe:
      ++mpState->mBonusInfo.mNumShotBonusGlobes;
      break;

    default:
      break;
  }
}


void GameWorld::receive(const rigel::events::BossActivated& event)
{
  mpState->mActiveBossEntity = event.mBossEntity;
  mpState->mBossStartingHealth = healthOrZero(event.mBossEntity);
  mpServiceProvider->playMusic(mpState->mLevelMusicFile);
}


void GameWorld::receive(const rigel::events::BossDestroyed& event)
{
  mpState->mBossDeathAnimationStartPending = true;
}


void GameWorld::receive(const rigel::events::CloakPickedUp& event)
{
  mpState->mCloakPickupPosition = event.mPosition;
}


void GameWorld::receive(const rigel::events::CloakExpired&)
{
  if (mpState->mCloakPickupPosition)
  {
    mpState->mEntityFactory.spawnActor(
      data::ActorID::White_box_cloaking_device, *mpState->mCloakPickupPosition);
  }
}


void GameWorld::loadLevel(const PlayerInput& initialInput)
{
  createNewState();

  mpState->mCamera.centerViewOnPlayer();
  updateGameLogic(initialInput);
  mpState->mPreviousCameraPosition = mpState->mCamera.position();

  if (data::isBossLevel(mSessionId.mLevel))
  {
    mpServiceProvider->playMusic(BOSS_LEVEL_INTRO_MUSIC);
  }
  else
  {
    mpServiceProvider->playMusic(mpState->mLevelMusicFile);
  }
}


void GameWorld::createNewState()
{
  if (mpState)
  {
    unsubscribe(mpState->mEventManager);
  }

  mpState = std::make_unique<WorldState>(
    mpServiceProvider,
    mpRenderer,
    mpResources,
    mpPlayerModel,
    mpOptions,
    mpSpriteFactory,
    mSessionId);

  subscribe(mpState->mEventManager);
}


void GameWorld::subscribe(entityx::EventManager& eventManager)
{
  eventManager.subscribe<rigel::events::CheckPointActivated>(*this);
  eventManager.subscribe<rigel::events::ExitReached>(*this);
  eventManager.subscribe<rigel::events::HintMachineMessage>(*this);
  eventManager.subscribe<rigel::events::PlayerDied>(*this);
  eventManager.subscribe<rigel::events::PlayerTookDamage>(*this);
  eventManager.subscribe<rigel::events::PlayerMessage>(*this);
  eventManager.subscribe<rigel::events::PlayerTeleported>(*this);
  eventManager.subscribe<rigel::events::ScreenFlash>(*this);
  eventManager.subscribe<rigel::events::ScreenShake>(*this);
  eventManager.subscribe<rigel::events::TutorialMessage>(*this);
  eventManager.subscribe<rigel::game_logic::events::ShootableKilled>(*this);
  eventManager.subscribe<rigel::events::BossActivated>(*this);
  eventManager.subscribe<rigel::events::BossDestroyed>(*this);
  eventManager.subscribe<rigel::events::CloakPickedUp>(*this);
  eventManager.subscribe<rigel::events::CloakExpired>(*this);
}


void GameWorld::unsubscribe(entityx::EventManager& eventManager)
{
  eventManager.unsubscribe<rigel::events::CheckPointActivated>(*this);
  eventManager.unsubscribe<rigel::events::ExitReached>(*this);
  eventManager.unsubscribe<rigel::events::HintMachineMessage>(*this);
  eventManager.unsubscribe<rigel::events::PlayerDied>(*this);
  eventManager.unsubscribe<rigel::events::PlayerTookDamage>(*this);
  eventManager.unsubscribe<rigel::events::PlayerMessage>(*this);
  eventManager.unsubscribe<rigel::events::PlayerTeleported>(*this);
  eventManager.unsubscribe<rigel::events::ScreenFlash>(*this);
  eventManager.unsubscribe<rigel::events::ScreenShake>(*this);
  eventManager.unsubscribe<rigel::events::TutorialMessage>(*this);
  eventManager.unsubscribe<rigel::game_logic::events::ShootableKilled>(*this);
  eventManager.unsubscribe<rigel::events::BossActivated>(*this);
  eventManager.unsubscribe<rigel::events::BossDestroyed>(*this);
  eventManager.unsubscribe<rigel::events::CloakPickedUp>(*this);
  eventManager.unsubscribe<rigel::events::CloakExpired>(*this);
}


bool GameWorld::needsPerElementUpscaling() const
{
  return mpSpriteFactory->hasHighResReplacements() ||
    mpState->mMapRenderer.hasHighResReplacements() ||
    mUiSpriteSheet.isHighRes();
}


void GameWorld::updateGameLogic(const PlayerInput& input)
{
  mpState->mBackdropFlashColor = std::nullopt;
  mpState->mScreenFlashColor = std::nullopt;

  if (mpState->mReactorDestructionFramesElapsed)
  {
    updateReactorDestructionEvent();
  }

  if (mpState->mEarthQuakeEffect)
  {
    mpState->mEarthQuakeEffect->update();
  }

  mHudRenderer.updateAnimation();
  mMessageDisplay.update();

  updateMotionSmoothingStates();

  if (mpState->mActiveBossEntity && mpState->mBossDeathAnimationStartPending)
  {
    engine::removeSafely<game_logic::components::PlayerDamaging>(
      mpState->mActiveBossEntity);
    mpState->mActiveBossEntity
      .replace<game_logic::components::BehaviorController>(
        behaviors::DyingBoss{mSessionId.mEpisode});
    mpState->mBossDeathAnimationStartPending = false;
  }

  const auto& viewportSize = widescreenModeOn()
    ? viewportSizeWideScreen(mpRenderer, *mpOptions)
    : data::GameTraits::mapViewportSize;

  mpState->mMapRenderer.updateAnimatedMapTiles();
  engine::updateAnimatedSprites(mpState->mEntities);
  ++mpState->mWaterAnimStep;
  if (mpState->mWaterAnimStep >= 4)
  {
    mpState->mWaterAnimStep = 0;
  }

  mpState->mPlayerInteractionSystem.updatePlayerInteraction(
    input, mpState->mEntities);
  mpState->mPlayer.update(input);
  mpState->mPreviousCameraPosition = mpState->mCamera.position();
  mpState->mCamera.update(input, viewportSize);

  engine::markActiveEntities(
    mpState->mEntities, mpState->mCamera.position(), viewportSize);
  mpState->mBehaviorControllerSystem.update(
    mpState->mEntities,
    PerFrameState{
      input,
      viewportSize,
      mpState->mRadarDishCounter.numRadarDishes(),
      mpState->mIsOddFrame,
      mpState->mEarthQuakeEffect && mpState->mEarthQuakeEffect->isQuaking()});

  mpState->mPhysicsSystem.updatePhase1(mpState->mEntities);

  // Collect items after physics, so that any collectible
  // items are in their final positions for this frame.
  mpState->mItemContainerSystem.updateItemBounce(mpState->mEntities);
  mpState->mPlayerInteractionSystem.updateItemCollection(mpState->mEntities);
  mpState->mPlayerDamageSystem.update(mpState->mEntities);
  mpState->mDamageInflictionSystem.update(mpState->mEntities);
  mpState->mItemContainerSystem.update(mpState->mEntities);
  mpState->mPlayerProjectileSystem.update(mpState->mEntities);

  mpState->mEffectsSystem.update(mpState->mEntities);
  mpState->mLifeTimeSystem.update(
    mpState->mEntities, mpState->mCamera.position(), viewportSize);

  // Now process any MovingBody objects that have been spawned after phase 1
  mpState->mPhysicsSystem.updatePhase2(mpState->mEntities);

  mpState->mParticles.update();

  if (!mpOptions->mMotionSmoothing)
  {
    mpState->mSpriteRenderingSystem.update(
      mpState->mEntities, viewportSize, mpState->mCamera.position(), 1.0f);
  }

  mpState->mIsOddFrame = !mpState->mIsOddFrame;
}


void GameWorld::render(const float interpolationFactor)
{
  if (
    widescreenModeOn() != mWidescreenModeWasOn ||
    mpOptions->mPerElementUpscalingEnabled != mPerElementUpscalingWasEnabled ||
    mPreviousWindowSize != mpRenderer->windowSize())
  {
    mSpecialEffects.rebuildBackgroundBuffer(*mpOptions);
  }

  if (
    widescreenModeOn() != mWidescreenModeWasOn ||
    mPreviousWindowSize != mpRenderer->windowSize() ||
    mPreviousHudStyle != mpOptions->mWidescreenHudStyle)
  {
    const auto& viewportSize = widescreenModeOn()
      ? viewportSizeWideScreen(mpRenderer, *mpOptions)
      : data::GameTraits::mapViewportSize;

    mpState->mCamera.recenter(viewportSize);
    mpState->mPreviousCameraPosition = mpState->mCamera.position();
  }

  if (mpOptions->mMotionSmoothing != mMotionSmoothingWasEnabled)
  {
    updateMotionSmoothingStates();
    mMotionSmoothingWasEnabled = mpOptions->mMotionSmoothing;
  }

  auto setupWorldClipRect =
    [&](const auto& renderStart, const auto& viewportSize) {
      const auto clampedSize =
        clampedSectionSize(renderStart, viewportSize, mpState->mMap);
      const auto clampedSizePx = data::tilesToPixels(clampedSize);

      auto saved = renderer::saveState(mpRenderer);
      renderer::setLocalClipRect(mpRenderer, {{}, clampedSizePx});
      return saved;
    };

  auto drawParticlesAndDebugOverlay =
    [&](const ViewportParams& viewportParams) {
      renderer::setLocalTranslation(mpRenderer, viewportParams.mCameraOffset);
      mpState->mParticles.render(
        viewportParams.mRenderStartPosition, interpolationFactor);
      mpState->mDebuggingSystem.update(
        mpState->mEntities,
        viewportParams.mRenderStartPosition,
        viewportParams.mViewportSize,
        interpolationFactor);
    };

  auto drawWorld = [&](const base::Size& viewportSize) {
    const auto viewportParams =
      determineSmoothScrollViewport(viewportSize, interpolationFactor);

    // prevent out of bounds areas from showing the backdrop/sprites
    auto clipRectGuard =
      setupWorldClipRect(viewportParams.mRenderStartPosition, viewportSize);

    if (mpState->mScreenFlashColor)
    {
      mpRenderer->clear(*mpState->mScreenFlashColor);
      return;
    }

    if (mpOptions->mPerElementUpscalingEnabled)
    {
      drawMapAndSprites(viewportParams, interpolationFactor);

      {
        const auto saved = mLowResLayer.bindAndReset();
        mpRenderer->clear({0, 0, 0, 0});
        drawParticlesAndDebugOverlay(viewportParams);
      }

      mLowResLayer.render(0, 0);
    }
    else
    {
      drawMapAndSprites(viewportParams, interpolationFactor);
      drawParticlesAndDebugOverlay(viewportParams);
    }
  };

  auto drawTopRow = [&, this](int maxWidthPx) {
    if (mpState->mActiveBossEntity)
    {
      const auto health = healthOrZero(mpState->mActiveBossEntity);

      const auto maxHealthBarSize = maxWidthPx - HEALTH_BAR_START_PX.x;
      if (mpState->mBossStartingHealth <= maxHealthBarSize)
      {
        drawBossHealthBar(health, mTextRenderer, mUiSpriteSheet);
      }
      else
      {
        const auto healthPercentage =
          float(health) / mpState->mBossStartingHealth;
        const auto healthPercentagePx =
          base::round(healthPercentage * maxHealthBarSize);
        drawBossHealthBar(healthPercentagePx, mTextRenderer, mUiSpriteSheet);
      }
    }
    else
    {
      mMessageDisplay.render();
    }
  };

  auto drawHud = [&, this]() {
    const auto radarDots =
      collectRadarDots(mpState->mEntities, mpState->mPlayer.orientedPosition());
    mHudRenderer.renderClassicHud(*mpPlayerModel, radarDots);
  };

  auto drawWidescreenHud = [&](const int viewportWidth) {
    const auto radarDots =
      collectRadarDots(mpState->mEntities, mpState->mPlayer.orientedPosition());
    mHudRenderer.renderWidescreenHud(
      viewportWidth, mpOptions->mWidescreenHudStyle, *mpPlayerModel, radarDots);
  };


  if (widescreenModeOn())
  {
    mpServiceProvider->markCurrentFrameAsWidescreen();

    const auto info = renderer::determineWidescreenViewport(mpRenderer);
    const auto viewportSize = viewportSizeWideScreen(mpRenderer, *mpOptions);

    if (!mWidescreenModeWasOn && !mpOptions->mMotionSmoothing)
    {
      mpState->mSpriteRenderingSystem.update(
        mpState->mEntities, viewportSize, mpState->mCamera.position(), 1.0f);
    }

    if (mpOptions->mPerElementUpscalingEnabled)
    {
      {
        const auto saved = setupIngameViewportWidescreen(
          mpRenderer, info, mpState->mScreenShakeOffsetX);

        drawWorld(viewportSize);

        drawWidescreenHud(info.mWidthTiles);
      }

      auto saved = setupWidescreenTopRowViewport(
        mpRenderer, info, mpState->mScreenShakeOffsetX);
      drawTopRow(data::tilesToPixels(viewportSize.width));
    }
    else
    {
      const auto saved = renderer::saveState(mpRenderer);
      mpRenderer->setClipRect({});

      mpRenderer->setGlobalTranslation(
        base::Vec2{mpState->mScreenShakeOffsetX, 0});
      drawTopRow(data::tilesToPixels(viewportSize.width));

      mpRenderer->setGlobalTranslation(base::Vec2{
        mpState->mScreenShakeOffsetX,
        data::GameTraits::inGameViewportOffset.y});
      drawWorld(viewportSize);

      drawWidescreenHud(info.mWidthTiles);
    }
  }
  else
  {
    {
      const auto saved =
        setupIngameViewport(mpRenderer, mpState->mScreenShakeOffsetX);

      drawWorld(data::GameTraits::mapViewportSize);
      drawHud();
    }

    auto saved = renderer::saveState(mpRenderer);
    renderer::setLocalTranslation(
      mpRenderer,
      {mpState->mScreenShakeOffsetX + data::GameTraits::inGameViewportOffset.x,
       0});
    drawTopRow(data::GameTraits::inGameViewportSize.width);
  }

  mPreviousHudStyle = mpOptions->mWidescreenHudStyle;
  mWidescreenModeWasOn = widescreenModeOn();
  mPerElementUpscalingWasEnabled = mpOptions->mPerElementUpscalingEnabled;
  mPreviousWindowSize = mpRenderer->windowSize();
}


auto GameWorld::determineSmoothScrollViewport(
  const base::Size& viewportSizeOriginal,
  float interpolationFactor) const -> ViewportParams
{
  const auto& state = *mpState;

  if (!mpOptions->mMotionSmoothing)
  {
    return {
      base::cast<float>(state.mCamera.position()),
      {},
      state.mCamera.position(),
      viewportSizeOriginal};
  }

  auto currentCameraPosition = state.mCamera.position();
  auto previousCameraPosition = state.mPreviousCameraPosition;

  const auto direction = currentCameraPosition - previousCameraPosition;

  if (direction.x < 0)
  {
    std::swap(currentCameraPosition.x, previousCameraPosition.x);
  }
  if (direction.y < 0)
  {
    std::swap(currentCameraPosition.y, previousCameraPosition.y);
  }

  const auto interpolationX =
    direction.x < 0 ? 1.0f - interpolationFactor : interpolationFactor;
  const auto interpolationY =
    direction.y < 0 ? 1.0f - interpolationFactor : interpolationFactor;

  const auto interpolatedCameraPosition = base::Vec2f{
    base::lerp(
      previousCameraPosition.x, currentCameraPosition.x, interpolationX),
    base::lerp(
      previousCameraPosition.y, currentCameraPosition.y, interpolationY),
  };

  const auto viewportSize = base::Size{
    viewportSizeOriginal.width + (direction.x != 0 ? 2 : 0),
    viewportSizeOriginal.height + (direction.y != 0 ? 2 : 0)};

  const auto cameraOffset =
    base::Vec2{
      base::round(data::tilesToPixels(interpolatedCameraPosition.x)),
      base::round(data::tilesToPixels(interpolatedCameraPosition.y))} -
    data::tilesToPixels(previousCameraPosition);

  return {
    interpolatedCameraPosition,
    cameraOffset * -1,
    previousCameraPosition,
    viewportSize};
}


void GameWorld::updateMotionSmoothingStates()
{
  if (mpOptions->mMotionSmoothing)
  {
    // Store current positions of all interpolated entities for use as
    // previous positions after the current update is done.
    mpState->mEntities.each<InterpolateMotion, WorldPosition>(
      [&](entityx::Entity, InterpolateMotion& data, const WorldPosition& pos) {
        data.mPreviousPosition = pos;
      });
  }
}


void GameWorld::drawMapAndSprites(
  const ViewportParams& params,
  const float interpolationFactor)
{
  using game_logic::components::TileDebris;

  auto& state = *mpState;

  auto renderBackdrop = [&]() {
    if (state.mBackdropFlashColor)
    {
      mpRenderer->drawFilledRectangle(
        {{}, data::tilesToPixels(params.mViewportSize)},
        *state.mBackdropFlashColor);
    }
    else
    {
      state.mMapRenderer.renderBackdrop(
        params.mInterpolatedCameraPosition, params.mViewportSize);
    }
  };

  auto renderTileDebris = [&]() {
    state.mEntities.each<TileDebris, WorldPosition>(
      [&](
        entityx::Entity e, const TileDebris& debris, const WorldPosition& pos) {
        const auto pixelPosition =
          engine::interpolatedPixelPosition(e, interpolationFactor);
        state.mMapRenderer.renderSingleTile(
          debris.mTileIndex,
          pixelPosition - data::tilesToPixels(params.mRenderStartPosition));
      });
  };

  auto renderBackgroundLayers = [&]() {
    state.mMapRenderer.renderBackground(
      params.mRenderStartPosition, params.mViewportSize);
    state.mDynamicGeometrySystem.renderDynamicBackgroundSections(
      params.mRenderStartPosition, params.mViewportSize, interpolationFactor);
    state.mSpriteRenderingSystem.renderRegularSprites(mSpecialEffects);
  };

  auto renderForegroundLayers = [&]() {
    state.mMapRenderer.renderForeground(
      params.mRenderStartPosition, params.mViewportSize);
    state.mDynamicGeometrySystem.renderDynamicForegroundSections(
      params.mRenderStartPosition, params.mViewportSize, interpolationFactor);
    state.mSpriteRenderingSystem.renderForegroundSprites(mSpecialEffects);
    renderTileDebris();
  };


  auto outerStateSave = renderer::saveState(mpRenderer);

  if (mpOptions->mMotionSmoothing)
  {
    mpState->mSpriteRenderingSystem.update(
      mpState->mEntities,
      params.mViewportSize,
      params.mRenderStartPosition,
      interpolationFactor);
  }

  const auto waterEffectAreas = collectWaterEffectAreas(
    state.mEntities, params.mRenderStartPosition, params.mViewportSize);
  if (
    waterEffectAreas.empty() &&
    !mpState->mSpriteRenderingSystem.cloakEffectSpritesVisible())
  {
    renderBackdrop();

    renderer::setLocalTranslation(mpRenderer, params.mCameraOffset);
    renderBackgroundLayers();
    renderForegroundLayers();
  }
  else
  {
    {
      auto saved = mSpecialEffects.bindBackgroundBuffer();
      renderBackdrop();

      renderer::setLocalTranslation(mpRenderer, params.mCameraOffset);
      renderBackgroundLayers();
    }

    mSpecialEffects.drawBackgroundBuffer();

    renderer::setLocalTranslation(mpRenderer, params.mCameraOffset);

    mSpecialEffects.drawWaterEffect(waterEffectAreas, state.mWaterAnimStep);
    renderForegroundLayers();
  }
}


bool GameWorld::widescreenModeOn() const
{
  return mpOptions->mWidescreenModeOn &&
    renderer::canUseWidescreenMode(mpRenderer);
}


void GameWorld::processEndOfFrameActions()
{
  handlePlayerDeath();
  handleTeleporter();

  mpState->mScreenShakeOffsetX = 0;
}


void GameWorld::activateFullHealthCheat()
{
  mpPlayerModel->resetHealthAndScore();
}


void GameWorld::activateGiveItemsCheat()
{
  namespace ex = entityx;
  using game_logic::components::CollectableItemForCheat;
  using game_logic::components::RadarDish;

  // Destroy all radar dishes
  mpState->mEntities.each<RadarDish>(
    [](ex::Entity entity, const RadarDish&) { entity.destroy(); });

  // Give all key items (circuit board, blue key, cloak) found in the level,
  // and a weapon.
  // The message shown after activating the cheat says it's a "random weapon",
  // but that's not true. The 3rd weapon found in spawn order is always given,
  // or an earlier one if the level contains less than three weapon pickups.
  // Key items are removed from the level, but weapons and the cloak are not.
  std::optional<data::WeaponType> weaponToGive;

  mpState->mEntities.each<CollectableItemForCheat>(
    [&, weaponsFound = 0](
      ex::Entity entity, const CollectableItemForCheat& item) mutable {
      base::match(
        item.mGivenItem,
        [&](const data::InventoryItemType inventoryItem) {
          if (
            inventoryItem == data::InventoryItemType::BlueKey ||
            inventoryItem == data::InventoryItemType::CircuitBoard)
          {
            mpPlayerModel->giveItem(inventoryItem);
            entity.destroy();
          }
          else if (
            inventoryItem == data::InventoryItemType::CloakingDevice &&
            !mpState->mPlayer.isCloaked())
          {
            mpPlayerModel->giveItem(inventoryItem);
          }
        },

        [&](const data::WeaponType weapon) {
          if (weaponsFound < 3)
          {
            weaponToGive = weapon;
            ++weaponsFound;
          }
        });
    });

  if (weaponToGive)
  {
    mpPlayerModel->switchToWeapon(*weaponToGive);
  }
}


void GameWorld::quickSave()
{
  if (!mpOptions->mQuickSavingEnabled || mpState->mPlayer.isDead())
  {
    return;
  }

  LOG_F(INFO, "Creating quick save");

  auto pStateCopy = std::make_unique<WorldState>(
    mpServiceProvider,
    mpRenderer,
    mpResources,
    mpPlayerModel,
    mpOptions,
    mpSpriteFactory,
    mSessionId);
  pStateCopy->synchronizeTo(
    *mpState, mpServiceProvider, mpPlayerModel, mSessionId);

  mpQuickSave = std::make_unique<QuickSaveData>(
    QuickSaveData{*mpPlayerModel, std::move(pStateCopy)});

  mMessageDisplay.setMessage("Quick saved.", ui::MessagePriority::Menu);

  LOG_F(INFO, "Quick save created");
}


void GameWorld::quickLoad()
{
  if (!canQuickLoad())
  {
    return;
  }

  LOG_F(INFO, "Loading quick save");

  *mpPlayerModel = mpQuickSave->mPlayerModel;
  mpState->synchronizeTo(
    *mpQuickSave->mpState, mpServiceProvider, mpPlayerModel, mSessionId);
  mpState->mPreviousCameraPosition = mpState->mCamera.position();
  mMessageDisplay.setMessage("Quick save restored.", ui::MessagePriority::Menu);

  if (!mpOptions->mMotionSmoothing)
  {
    const auto& viewportSize = widescreenModeOn()
      ? viewportSizeWideScreen(mpRenderer, *mpOptions)
      : data::GameTraits::mapViewportSize;
    mpState->mSpriteRenderingSystem.update(
      mpState->mEntities, viewportSize, mpState->mCamera.position(), 1.0f);
  }

  LOG_F(INFO, "Quick save loaded");
}


bool GameWorld::canQuickLoad() const
{
  return mpOptions->mQuickSavingEnabled && mpQuickSave;
}


void GameWorld::onReactorDestroyed(const base::Vec2& position)
{
  flashScreen(data::GameTraits::INGAME_PALETTE[7]);

  mpState->mEntityFactory.spawnProjectile(
    ProjectileType::ReactorDebris,
    position + base::Vec2{-1, 0},
    ProjectileDirection::Left);
  mpState->mEntityFactory.spawnProjectile(
    ProjectileType::ReactorDebris,
    position + base::Vec2{3, 0},
    ProjectileDirection::Right);

  const auto shouldDoSpecialEvent = mpState->mBackdropSwitchCondition ==
    data::map::BackdropSwitchCondition::OnReactorDestruction;
  if (!mpState->mReactorDestructionFramesElapsed && shouldDoSpecialEvent)
  {
    mpState->mMapRenderer.switchBackdrops();
    mpState->mBackdropSwitched = true;
    mpState->mReactorDestructionFramesElapsed = 0;
  }
}


void GameWorld::updateReactorDestructionEvent()
{
  auto& framesElapsed = *mpState->mReactorDestructionFramesElapsed;
  if (framesElapsed >= 14)
  {
    return;
  }

  if (framesElapsed == 13)
  {
    mMessageDisplay.setMessage(data::Messages::DestroyedEverything);
  }
  else if (framesElapsed % 2 == 1)
  {
    if (mpOptions->mEnableScreenFlashes)
    {
      mpState->mBackdropFlashColor = base::Color{255, 255, 255, 255};
    }

    mpServiceProvider->playSound(data::SoundId::BigExplosion);
  }

  ++framesElapsed;
}


void GameWorld::handlePlayerDeath()
{
  if (mpState->mPlayerDied)
  {
    mpState->mPlayerDied = false;
    mpState->mActiveBossEntity = {};

    if (mpState->mActivatedCheckpoint)
    {
      restartFromCheckpoint();
    }
    else
    {
      restartLevel();
    }
  }
}


void GameWorld::restartLevel()
{
  mpServiceProvider->fadeOutScreen();

  *mpPlayerModel = mPlayerModelAtLevelStart;
  loadLevel({});

  if (mpState->mRadarDishCounter.radarDishesPresent())
  {
    mMessageDisplay.setMessage(data::Messages::FindAllRadars);
  }

  render();

  mpServiceProvider->fadeInScreen();
}


void GameWorld::restartFromCheckpoint()
{
  mpServiceProvider->fadeOutScreen();

  const auto shouldSwitchBackAfterRespawn = mpState->mBackdropSwitchCondition ==
    data::map::BackdropSwitchCondition::OnTeleportation;
  if (mpState->mBackdropSwitched && shouldSwitchBackAfterRespawn)
  {
    mpState->mMapRenderer.switchBackdrops();
    mpState->mBackdropSwitched = false;
  }

  mpPlayerModel->restoreFromCheckpoint(mpState->mActivatedCheckpoint->mState);
  mpState->mPlayer.reSpawnAt(mpState->mActivatedCheckpoint->mPosition);

  mpState->mCamera.centerViewOnPlayer();
  updateGameLogic({});
  mpState->mPreviousCameraPosition = mpState->mCamera.position();
  render();

  mpServiceProvider->fadeInScreen();
}


void GameWorld::handleTeleporter()
{
  if (!mpState->mTeleportTargetPosition)
  {
    return;
  }

  mpServiceProvider->fadeOutScreen();

  mpState->mPlayer.position() = *mpState->mTeleportTargetPosition;
  mpState->mTeleportTargetPosition = std::nullopt;

  const auto switchBackdrop = mpState->mBackdropSwitchCondition ==
    data::map::BackdropSwitchCondition::OnTeleportation;
  if (switchBackdrop)
  {
    mpState->mMapRenderer.switchBackdrops();
    mpState->mBackdropSwitched = !mpState->mBackdropSwitched;
  }

  mpState->mCamera.centerViewOnPlayer();
  updateGameLogic({});
  render(1.0f);
  mpState->mPreviousCameraPosition = mpState->mCamera.position();
  mpServiceProvider->fadeInScreen();
}


void GameWorld::showTutorialMessage(const data::TutorialMessageId id)
{
  if (!mpPlayerModel->tutorialMessages().hasBeenShown(id))
  {
    mMessageDisplay.setMessage(data::messageText(id));
    mpPlayerModel->tutorialMessages().markAsShown(id);
  }
}


void GameWorld::flashScreen(const base::Color& color)
{
  if (mpOptions->mEnableScreenFlashes)
  {
    mpState->mScreenFlashColor = color;
  }
}


void GameWorld::printDebugText(std::ostream& stream) const
{
  stream << "Scroll: " << vec2String(mpState->mCamera.position(), 4) << '\n'
         << "Player: " << vec2String(mpState->mPlayer.position(), 4) << '\n'
         << "Entities: " << mpState->mEntities.size() << '\n';

  if (mpOptions->mPerElementUpscalingEnabled)
  {
    stream << "Hi-res mode ON\n";
  }
}

} // namespace rigel::game_logic
