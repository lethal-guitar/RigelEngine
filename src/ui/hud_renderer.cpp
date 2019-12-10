/* Copyright (C) 2016, Nikolai Wuttke. All rights reserved.
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

#include "hud_renderer.hpp"

#include "data/game_traits.hpp"
#include "loader/palette.hpp"
#include "loader/resource_loader.hpp"

#include <cmath>
#include <string>


namespace rigel::ui {

using namespace rigel::data;
using namespace rigel::engine;
using namespace rigel::renderer;


namespace {

constexpr auto NUM_HEALTH_SLICES = 8;

constexpr auto RADAR_SIZE_PX = 32;
constexpr auto RADAR_CENTER_POS_X = 288;
constexpr auto RADAR_CENTER_POS_Y = 136;

constexpr auto RADAR_POS_X =
  RADAR_CENTER_POS_X -
  RADAR_SIZE_PX/2 -
  data::GameTraits::inGameViewPortOffset.x;
constexpr auto RADAR_POS_Y =
  RADAR_CENTER_POS_Y -
  RADAR_SIZE_PX/2 -
  data::GameTraits::inGameViewPortOffset.y;

constexpr auto NUM_RADAR_BLINK_STEPS = 4;
constexpr auto RADAR_BLINK_START_COLOR_INDEX = 3;


void drawNumbersBig(
  const int number,
  const int maxDigits,
  const base::Vector& tlPosition,
  const TiledTexture& spriteSheet
) {
  const auto printed = std::to_string(number);

  const auto overflow = maxDigits - static_cast<int>(printed.size());
  const auto inputToSkip = std::max(0, -overflow);
  const auto positionsToSkip = std::max(0, overflow);

  for (auto digit=0; digit<maxDigits; ++digit) {
    const auto tlPositionForDigit = tlPosition + base::Vector{digit*2, 0};

    if (digit >= positionsToSkip) {
      const auto numeralIndex =
        printed[digit - positionsToSkip + inputToSkip] - 0x30;
      spriteSheet.renderTileQuad(numeralIndex*2 + 7*40, tlPositionForDigit);
    }
  }
}


void drawScore(const int score, const TiledTexture& spriteSheet) {
  drawNumbersBig(
    score,
    7,
    base::Vector{2, GameTraits::mapViewPortSize.height + 1},
    spriteSheet);
}


void drawWeaponIcon(const WeaponType type, const TiledTexture& spriteSheet) {
  const auto weaponIndex = static_cast<int>(type);
  spriteSheet.renderTileDoubleQuad(
    weaponIndex*4 + 4 + 5*40,
    base::Vector{17, GameTraits::mapViewPortSize.height + 1});
}


void drawAmmoBar(
  const int currentAmmo,
  const int maxAmmo,
  const TiledTexture& spriteSheet
) {
  // The sprite sheet has 17 bar sizes; index 0 is full, 16 is empty.
  // Starting at col 0, row 23. Each bar is 2 tiles high

  const auto quantizedAmmoCount = static_cast<int>(std::ceil(
    static_cast<float>(currentAmmo) / maxAmmo * 16.0f));

  const auto ammoBarIndex = 16 - quantizedAmmoCount;
  spriteSheet.renderTileSlice(
    ammoBarIndex + 23*40,
    base::Vector{22, GameTraits::mapViewPortSize.height + 1});
}


void drawLevelNumber(const int number, const TiledTexture& spriteSheet) {
  drawNumbersBig(
    number,
    1,
    base::Vector{
      GameTraits::mapViewPortSize.width + 2,
      GameTraits::mapViewPortSize.height},
    spriteSheet);
}


OwningTexture actorToTexture(
  renderer::Renderer* pRenderer,
  const loader::ActorData& data
) {
  return OwningTexture(pRenderer, data.mFrames[0].mFrameImage);
}


}


HudRenderer::InventoryItemTextureMap HudRenderer::makeInventoryItemTextureMap(
  renderer::Renderer* pRenderer,
  const loader::ActorImagePackage& imagePack
) {
  InventoryItemTextureMap map;

  map.emplace(
    InventoryItemType::CircuitBoard,
    actorToTexture(pRenderer, imagePack.loadActor(data::ActorID::White_box_circuit_card)));
  map.emplace(
    InventoryItemType::BlueKey,
    actorToTexture(pRenderer, imagePack.loadActor(data::ActorID::White_box_blue_key)));
  map.emplace(
    InventoryItemType::RapidFire,
    actorToTexture(pRenderer, imagePack.loadActor(data::ActorID::Rapid_fire_icon)));
  map.emplace(
    InventoryItemType::SpecialHintGlobe,
    actorToTexture(pRenderer, imagePack.loadActor(data::ActorID::Special_hint_globe_icon)));
  map.emplace(
    InventoryItemType::CloakingDevice,
    actorToTexture(pRenderer, imagePack.loadActor(data::ActorID::Cloaking_device_icon)));
  return map;
}


HudRenderer::CollectedLetterIndicatorMap
HudRenderer::makeCollectedLetterTextureMap(
  renderer::Renderer* pRenderer,
  const loader::ActorImagePackage& imagePack
) {
  CollectedLetterIndicatorMap map;

  const auto rightScreenEdge = GameTraits::inGameViewPortSize.width;
  const auto bottomScreenEdge = GameTraits::inGameViewPortSize.height;
  const base::Vector letterDrawStart{
    rightScreenEdge - 5*GameTraits::tileSize,
    bottomScreenEdge - 2*GameTraits::tileSize};
  // TODO: Consider using the positions from the loaded actor frames instead
  // of calculating manually?
  const base::Vector letterSize{GameTraits::tileSize, 0};
  map.emplace(
    CollectableLetterType::N,
    CollectedLetterIndicator{
      actorToTexture(pRenderer, imagePack.loadActor(ActorID::Letter_collection_indicator_N)),
      letterDrawStart});
  map.emplace(
    CollectableLetterType::U,
    CollectedLetterIndicator{
      actorToTexture(pRenderer, imagePack.loadActor(ActorID::Letter_collection_indicator_U)),
      letterDrawStart});
  map.emplace(
    CollectableLetterType::K,
    CollectedLetterIndicator{
      actorToTexture(pRenderer, imagePack.loadActor(ActorID::Letter_collection_indicator_K)),
      letterDrawStart + letterSize * 1});
  map.emplace(
    CollectableLetterType::E,
    CollectedLetterIndicator{
      actorToTexture(pRenderer, imagePack.loadActor(ActorID::Letter_collection_indicator_E)),
      letterDrawStart + letterSize * 2});
  map.emplace(
    CollectableLetterType::M,
    CollectedLetterIndicator{
      actorToTexture(pRenderer, imagePack.loadActor(ActorID::Letter_collection_indicator_M)),
      letterDrawStart + letterSize * 3});
  return map;
}


HudRenderer::HudRenderer(
  const int levelNumber,
  renderer::Renderer* pRenderer,
  const loader::ResourceLoader& bundle,
  engine::TiledTexture* pStatusSpriteSheet
)
  : HudRenderer(
      levelNumber,
      pRenderer,
      bundle.mActorImagePackage.loadActor(ActorID::HUD_frame_background),
      makeInventoryItemTextureMap(pRenderer, bundle.mActorImagePackage),
      makeCollectedLetterTextureMap(pRenderer, bundle.mActorImagePackage),
      pStatusSpriteSheet)
{
}


HudRenderer::HudRenderer(
  const int levelNumber,
  renderer::Renderer* pRenderer,
  const loader::ActorData& actorData,
  InventoryItemTextureMap&& inventoryItemTextures,
  CollectedLetterIndicatorMap&& collectedLetterTextures,
  engine::TiledTexture* pStatusSpriteSheet
)
  : mLevelNumber(levelNumber)
  , mpRenderer(pRenderer)
  , mTopRightTexture(mpRenderer, actorData.mFrames[0].mFrameImage)
  , mBottomLeftTexture(mpRenderer, actorData.mFrames[1].mFrameImage)
  , mBottomRightTexture(mpRenderer, actorData.mFrames[2].mFrameImage)
  , mInventoryTexturesByType(std::move(inventoryItemTextures))
  , mCollectedLetterIndicatorsByType(std::move(collectedLetterTextures))
  , mpStatusSpriteSheetRenderer(pStatusSpriteSheet)
  , mRadarSurface(pRenderer, RADAR_SIZE_PX, RADAR_SIZE_PX)
{
}


void HudRenderer::updateAnimation() {
  ++mElapsedFrames;
}


void HudRenderer::render(const data::PlayerModel& playerModel) {
  // Hud background
  // --------------------------------------------------------------------------
  const auto maxX = GameTraits::inGameViewPortSize.width;
  mBottomLeftTexture.render(
    mpRenderer,
    0,
    GameTraits::inGameViewPortSize.height - mBottomLeftTexture.height());

  mBottomRightTexture.render(
    mpRenderer,
    mBottomLeftTexture.width(),
    GameTraits::inGameViewPortSize.height - mBottomRightTexture.height());

  const auto topRightTexturePosX = maxX - mTopRightTexture.width();
  mTopRightTexture.render(mpRenderer, topRightTexturePosX, 0);

  // Inventory
  // --------------------------------------------------------------------------
  const auto inventoryStartPos = base::Vector{
    topRightTexturePosX + GameTraits::tileSize,
    2*GameTraits::tileSize};
  auto inventoryIter = playerModel.inventory().cbegin();
  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 2; ++col) {
      if (inventoryIter != playerModel.inventory().cend()) {
        const auto itemType = *inventoryIter++;
        const auto drawPos =
          inventoryStartPos + base::Vector{col, row} * GameTraits::tileSize*2;

        const auto textureIt = mInventoryTexturesByType.find(itemType);
        assert(textureIt != mInventoryTexturesByType.end());
        textureIt->second.render(mpRenderer, drawPos);
      }
    }
  }

  // Player state and remaining HUD elements
  // --------------------------------------------------------------------------
  drawScore(playerModel.score(), *mpStatusSpriteSheetRenderer);
  drawWeaponIcon(playerModel.weapon(), *mpStatusSpriteSheetRenderer);
  drawAmmoBar(
    playerModel.ammo(),
    playerModel.currentMaxAmmo(),
    *mpStatusSpriteSheetRenderer);
  drawHealthBar(playerModel);
  drawLevelNumber(mLevelNumber, *mpStatusSpriteSheetRenderer);
  drawCollectedLetters(playerModel);
  drawRadar();
}


void HudRenderer::drawHealthBar(const data::PlayerModel& playerModel) const {
  // Health slices start at col 20, row 4. The first 9 are for the "0 health"
  // animation

  // The model has a range of 1-9 for health, but the HUD shows only 8
  // slices, with a special animation for having 1 point of health.
  const auto numFullSlices = playerModel.health() - 1;
  if (numFullSlices > 0) {
    for (int i=0; i<NUM_HEALTH_SLICES; ++i) {
      const auto sliceIndex = i < numFullSlices ? 9 : 10;
      mpStatusSpriteSheetRenderer->renderTileSlice(
        sliceIndex + 20 + 4*40,
        base::Vector{24 + i, GameTraits::mapViewPortSize.height + 1});
    }
  } else {
    const auto animationOffset = mElapsedFrames;

    for (int i=0; i<NUM_HEALTH_SLICES; ++i) {
      const auto sliceIndex = (i + animationOffset) % 9;
      mpStatusSpriteSheetRenderer->renderTileSlice(
        sliceIndex + 20 + 4*40,
        base::Vector{24 + i, GameTraits::mapViewPortSize.height + 1});
    }
  }
}


void HudRenderer::drawCollectedLetters(
  const data::PlayerModel& playerModel
) const {
  for (const auto letter : playerModel.collectedLetters()) {
    const auto it = mCollectedLetterIndicatorsByType.find(letter);
    assert(it != mCollectedLetterIndicatorsByType.end());
    it->second.mTexture.render(mpRenderer, it->second.mPxPosition);
  }
}


void HudRenderer::drawRadar() const {
  {
    auto binder =
      renderer::RenderTargetTexture::Binder{mRadarSurface, mpRenderer};
    auto stateSaver = renderer::setupDefaultState(mpRenderer);

    mpRenderer->clear({0, 0, 0, 0});

    const auto blinkColorIndex =
      mElapsedFrames % NUM_RADAR_BLINK_STEPS + RADAR_BLINK_START_COLOR_INDEX;
    const auto blinkColor = loader::INGAME_PALETTE[blinkColorIndex];
    mpRenderer->drawPoint({RADAR_SIZE_PX/2, RADAR_SIZE_PX/2 + 1}, blinkColor);
  }

  mRadarSurface.render(mpRenderer, RADAR_POS_X, RADAR_POS_Y);
}

}
