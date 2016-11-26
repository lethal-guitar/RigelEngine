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
#include "loader/resource_loader.hpp"

#include <cmath>
#include <string>


namespace rigel { namespace ui {

using namespace rigel::data;
using namespace rigel::engine;
using sdl_utils::OwningTexture;


namespace {

const auto HUD_BACKGROUND_ACTOR_ID = 77;
const auto NUM_HEALTH_SLICES = 8;


void drawNumbersBig(
  const int number,
  const int maxDigits,
  const base::Vector& tlPosition,
  const TileRenderer& spriteSheet
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


void drawScore(const int score, const TileRenderer& spriteSheet) {
  drawNumbersBig(
    score,
    7,
    base::Vector{2, GameTraits::mapViewPortSize.height + 1},
    spriteSheet);
}


void drawWeaponIcon(const WeaponType type, const TileRenderer& spriteSheet) {
  const auto weaponIndex = static_cast<int>(type);
  spriteSheet.renderTileDoubleQuad(
    weaponIndex*4 + 4 + 5*40,
    base::Vector{17, GameTraits::mapViewPortSize.height + 1});
}


void drawAmmoBar(
  const int currentAmmo,
  const int maxAmmo,
  const TileRenderer& spriteSheet
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


void drawLevelNumber(const int number, const TileRenderer& spriteSheet) {
  drawNumbersBig(
    number,
    1,
    base::Vector{
      GameTraits::mapViewPortSize.width + 1,
      GameTraits::mapViewPortSize.height},
    spriteSheet);
}


sdl_utils::OwningTexture actorToTexture(
  SDL_Renderer* pRenderer,
  const loader::ActorData& data
) {
  return sdl_utils::OwningTexture(pRenderer, data.mFrames[0].mFrameImage);
}


}


HudRenderer::InventoryItemTextureMap HudRenderer::makeInventoryItemTextureMap(
  SDL_Renderer* pRenderer,
  const loader::ActorImagePackage& imagePack
) {
  InventoryItemTextureMap map;

  map.emplace(
    InventoryItemType::CircuitBoard,
    actorToTexture(pRenderer, imagePack.loadActor(37)));
  map.emplace(
    InventoryItemType::BlueKey,
    actorToTexture(pRenderer, imagePack.loadActor(121)));
  map.emplace(
    InventoryItemType::RapidFire,
    actorToTexture(pRenderer, imagePack.loadActor(52)));
  map.emplace(
    InventoryItemType::SpecialHintGlobe,
    actorToTexture(pRenderer, imagePack.loadActor(238)));
  map.emplace(
    InventoryItemType::CloakingDevice,
    actorToTexture(pRenderer, imagePack.loadActor(211)));
  return map;
}


HudRenderer::CollectedLetterIndicatorMap
HudRenderer::makeCollectedLetterTextureMap(
  SDL_Renderer* pRenderer,
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
      actorToTexture(pRenderer, imagePack.loadActor(290)),
      letterDrawStart});
  map.emplace(
    CollectableLetterType::U,
    CollectedLetterIndicator{
      actorToTexture(pRenderer, imagePack.loadActor(291)),
      letterDrawStart});
  map.emplace(
    CollectableLetterType::K,
    CollectedLetterIndicator{
      actorToTexture(pRenderer, imagePack.loadActor(292)),
      letterDrawStart + letterSize * 1});
  map.emplace(
    CollectableLetterType::E,
    CollectedLetterIndicator{
      actorToTexture(pRenderer, imagePack.loadActor(293)),
      letterDrawStart + letterSize * 2});
  map.emplace(
    CollectableLetterType::M,
    CollectedLetterIndicator{
      actorToTexture(pRenderer, imagePack.loadActor(294)),
      letterDrawStart + letterSize * 3});
  return map;
}


HudRenderer::HudRenderer(
  data::PlayerModel* pPlayerModel,
  const int levelNumber,
  SDL_Renderer* pRenderer,
  const loader::ResourceLoader& bundle
)
  : HudRenderer(
      pPlayerModel,
      levelNumber,
      pRenderer,
      bundle.mActorImagePackage.loadActor(HUD_BACKGROUND_ACTOR_ID),
      makeInventoryItemTextureMap(pRenderer, bundle.mActorImagePackage),
      makeCollectedLetterTextureMap(pRenderer, bundle.mActorImagePackage),
      bundle.loadTiledFullscreenImage("STATUS.MNI"))
{
}


HudRenderer::HudRenderer(
  data::PlayerModel* pPlayerModel,
  const int levelNumber,
  SDL_Renderer* pRenderer,
  const loader::ActorData& actorData,
  InventoryItemTextureMap&& inventoryItemTextures,
  CollectedLetterIndicatorMap&& collectedLetterTextures,
  const data::Image& statusSpriteSheetImage
)
  : mpPlayerModel(pPlayerModel)
  , mLevelNumber(levelNumber)
  , mpRenderer(pRenderer)
  , mTopRightTexture(mpRenderer, actorData.mFrames[0].mFrameImage)
  , mBottomLeftTexture(mpRenderer, actorData.mFrames[1].mFrameImage)
  , mBottomRightTexture(mpRenderer, actorData.mFrames[2].mFrameImage)
  , mInventoryTexturesByType(std::move(inventoryItemTextures))
  , mCollectedLetterIndicatorsByType(std::move(collectedLetterTextures))
  , mStatusSpriteSheetRenderer(
      sdl_utils::OwningTexture(pRenderer, statusSpriteSheetImage),
      pRenderer)
{
}


void HudRenderer::render() {
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
  auto inventoryIter = mpPlayerModel->mInventory.cbegin();
  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 2; ++col) {
      if (inventoryIter != mpPlayerModel->mInventory.cend()) {
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
  drawScore(mpPlayerModel->mScore, mStatusSpriteSheetRenderer);
  drawWeaponIcon(mpPlayerModel->mWeapon, mStatusSpriteSheetRenderer);
  drawAmmoBar(
    mpPlayerModel->mAmmo,
    mpPlayerModel->currentMaxAmmo(),
    mStatusSpriteSheetRenderer);
  drawHealthBar();
  drawLevelNumber(mLevelNumber, mStatusSpriteSheetRenderer);
  drawCollectedLetters();
}


void HudRenderer::update(engine::TimeDelta dt) {
  mTimeStepper.update(dt);
}


void HudRenderer::drawHealthBar() const {
  // Health slices start at col 20, row 4. The first 9 are for the "0 health"
  // animation

  // The model has a range of 1-9 for health, but the HUD shows only 8
  // slices, with a special animation for having 1 point of health.
  const auto numFullSlices = mpPlayerModel->mHealth - 1;
  if (numFullSlices > 0) {
    for (int i=0; i<NUM_HEALTH_SLICES; ++i) {
      const auto sliceIndex = i < numFullSlices ? 9 : 10;
      mStatusSpriteSheetRenderer.renderTileSlice(
        sliceIndex + 20 + 4*40,
        base::Vector{24 + i, GameTraits::mapViewPortSize.height + 1});
    }
  } else {
    // TODO: Determine actual animation timing
    const auto animationOffset = mTimeStepper.elapsedTicks() / 2;

    for (int i=0; i<NUM_HEALTH_SLICES; ++i) {
      const auto sliceIndex = (i + animationOffset) % 9;
      mStatusSpriteSheetRenderer.renderTileSlice(
        sliceIndex + 20 + 4*40,
        base::Vector{24 + i, GameTraits::mapViewPortSize.height + 1});
    }
  }
}


void HudRenderer::drawCollectedLetters() const {
  for (const auto letter : mpPlayerModel->mCollectedLetters) {
    const auto it = mCollectedLetterIndicatorsByType.find(letter);
    assert(it != mCollectedLetterIndicatorsByType.end());

    it->second.mTexture.render(
      mpRenderer,
      it->second.mPxPosition);
  }
}


}}
