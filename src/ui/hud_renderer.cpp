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

#include "data/game_options.hpp"
#include "data/game_traits.hpp"
#include "data/unit_conversions.hpp"
#include "engine/sprite_factory.hpp"
#include "loader/palette.hpp"
#include "loader/resource_loader.hpp"

#include <cmath>
#include <string>


namespace rigel::ui
{

using namespace rigel::data;
using namespace rigel::engine;
using namespace rigel::renderer;


namespace
{

constexpr auto NUM_HEALTH_SLICES = 8;

constexpr auto RADAR_SIZE_PX = 32;
constexpr auto RADAR_CENTER_POS_X = 288;
constexpr auto RADAR_CENTER_POS_Y = 136;

constexpr auto RADAR_POS_X = RADAR_CENTER_POS_X - RADAR_SIZE_PX / 2 -
  data::GameTraits::inGameviewportOffset.x;
constexpr auto RADAR_POS_Y = RADAR_CENTER_POS_Y - RADAR_SIZE_PX / 2 -
  data::GameTraits::inGameviewportOffset.y;
constexpr auto RADAR_CENTER_OFFSET_RELATIVE =
  base::Vec2{RADAR_SIZE_PX / 2, RADAR_SIZE_PX / 2 + 1};

constexpr auto HUD_START_TOP_RIGHT =
  base::Vec2{data::GameTraits::mapviewportWidthTiles, 0};
constexpr auto HUD_START_BOTTOM_LEFT =
  base::Vec2{0, data::GameTraits::mapviewportHeightTiles};
constexpr auto HUD_START_BOTTOM_RIGHT = base::Vec2{
  HUD_START_BOTTOM_LEFT.x + 28,
  data::GameTraits::mapviewportHeightTiles};

constexpr auto INVENTORY_START_POS = base::Vec2{HUD_START_TOP_RIGHT.x + 1, 2};

// The letter collection indicator actors already contain an offset in the
// actor info that positions them correctly. Unfortunately, that offset is
// relative to the entire screen, but in our HUD renderer, everything is
// relative to the start of the map viewport, i.e. offset by {1, 1} tiles.  We
// have to account for that and render the indicators further up/left to negate
// this offset. On top of that, we need to offset one more to the left and one
// more up, because that's how the original game's coordinate system works - a
// coordinate for actor rendering actually refers to the actor's bottom-left,
// so we have to subtract one on the Y axis (each letter indicator is 2 tiles
// tall), and then we also have to subtract one on the X axis since the X
// coordinate refers to the tile after the actor's left-most tile for whatever
// reason. It's not clear why, the original executable's code literally has `x
// - 1` in the function that's used for drawing actors in the UI/HUD.  The
// in-game sprite drawing code doesn't have that behavior (but it does have the
// 'Y refers to bottom-most tile' part).  TODO: Maybe don't hardcode the height
// of the indicators?
constexpr auto LETTER_INDICATOR_POSITION = base::Vec2{-2, -2};

constexpr auto NUM_RADAR_BLINK_STEPS = 4;
constexpr auto RADAR_BLINK_START_COLOR_INDEX = 3;
const auto RADAR_DOT_COLOR = data::GameTraits::INGAME_PALETTE[15];


void drawNumbersBig(
  const int number,
  const int maxDigits,
  const base::Vec2& tlPosition,
  const TiledTexture& spriteSheet)
{
  auto remainingNumber = number;

  for (auto digitIndex = 0; digitIndex < maxDigits; ++digitIndex)
  {
    // Draw digits from right to left, until the number is fully drawn or
    // we run out of digits to draw.
    const auto tlPositionForDigit =
      tlPosition + base::Vec2{(maxDigits - 1 - digitIndex) * 2, 0};

    const auto digitValue = remainingNumber % 10;
    remainingNumber /= 10;

    // clang-format off
    spriteSheet.renderTileQuad(digitValue*2 + 7*40, tlPositionForDigit);
    // clang-format on

    if (remainingNumber == 0)
    {
      break;
    }
  }
}


void drawScore(const int score, const TiledTexture& spriteSheet)
{
  drawNumbersBig(
    score,
    7,
    base::Vec2{2, GameTraits::mapviewportSize.height + 1},
    spriteSheet);
}


void drawWeaponIcon(const WeaponType type, const TiledTexture& spriteSheet)
{
  const auto weaponIndex = static_cast<int>(type);
  // clang-format off
  spriteSheet.renderTileDoubleQuad(
    weaponIndex*4 + 4 + 5*40,
    base::Vec2{17, GameTraits::mapviewportSize.height + 1});
  // clang-format on
}


void drawAmmoBar(
  const int currentAmmo,
  const int maxAmmo,
  const TiledTexture& spriteSheet)
{
  // The sprite sheet has 17 bar sizes; index 0 is full, 16 is empty.
  // Starting at col 0, row 23. Each bar is 2 tiles high

  const auto quantizedAmmoCount = static_cast<int>(
    std::ceil(static_cast<float>(currentAmmo) / maxAmmo * 16.0f));

  const auto ammoBarIndex = 16 - quantizedAmmoCount;
  spriteSheet.renderTileSlice(
    ammoBarIndex + 23 * 40,
    base::Vec2{22, GameTraits::mapviewportSize.height + 1});
}


void drawLevelNumber(const int number, const TiledTexture& spriteSheet)
{
  drawNumbersBig(
    number,
    1,
    base::Vec2{
      GameTraits::mapviewportSize.width + 2,
      GameTraits::mapviewportSize.height},
    spriteSheet);
}

} // namespace


HudRenderer::HudRenderer(
  const int levelNumber,
  const data::GameOptions* pOptions,
  renderer::Renderer* pRenderer,
  engine::TiledTexture* pStatusSpriteSheet,
  const engine::SpriteFactory* pSpriteFactory)
  : mLevelNumber(levelNumber)
  , mpRenderer(pRenderer)
  , mpOptions(pOptions)
  , mpStatusSpriteSheetRenderer(pStatusSpriteSheet)
  , mpSpriteFactory(pSpriteFactory)
  , mRadarSurface(pRenderer, RADAR_SIZE_PX, RADAR_SIZE_PX)
{
}


void HudRenderer::updateAnimation()
{
  ++mElapsedFrames;
}


void HudRenderer::render(
  const data::PlayerModel& playerModel,
  const base::ArrayView<base::Vec2> radarPositions)
{
  // We group drawing into what texture is used to minimize the amount of
  // OpenGL state switches needed.

  // These use the actor sprite sheet texture.
  drawActorFrame(ActorID::HUD_frame_background, 0, HUD_START_TOP_RIGHT);
  drawActorFrame(ActorID::HUD_frame_background, 1, HUD_START_BOTTOM_LEFT);
  drawActorFrame(ActorID::HUD_frame_background, 2, HUD_START_BOTTOM_RIGHT);
  drawInventory(playerModel.inventory());
  drawCollectedLetters(playerModel);

  // These use the UI sprite sheet texture.
  drawScore(playerModel.score(), *mpStatusSpriteSheetRenderer);
  drawWeaponIcon(playerModel.weapon(), *mpStatusSpriteSheetRenderer);
  drawAmmoBar(
    playerModel.ammo(),
    playerModel.currentMaxAmmo(),
    *mpStatusSpriteSheetRenderer);
  drawHealthBar(playerModel);
  drawLevelNumber(mLevelNumber, *mpStatusSpriteSheetRenderer);
  drawRadar(radarPositions);
}


void HudRenderer::drawInventory(
  const std::vector<data::InventoryItemType>& inventory) const
{
  auto iItem = inventory.begin();
  for (int row = 0; row < 3; ++row)
  {
    for (int col = 0; col < 2; ++col)
    {
      if (iItem != inventory.end())
      {
        const auto itemType = *iItem++;
        const auto drawPos = INVENTORY_START_POS + base::Vec2{col * 2, row * 2};

        switch (itemType)
        {
          case InventoryItemType::CircuitBoard:
            drawActorFrame(ActorID::White_box_circuit_card, 0, drawPos);
            break;

          case InventoryItemType::BlueKey:
            drawActorFrame(ActorID::White_box_blue_key, 0, drawPos);
            break;

          case InventoryItemType::RapidFire:
            drawActorFrame(ActorID::Rapid_fire_icon, 0, drawPos);
            break;

          case InventoryItemType::SpecialHintGlobe:
            drawActorFrame(ActorID::Special_hint_globe_icon, 0, drawPos);
            break;

          case InventoryItemType::CloakingDevice:
            drawActorFrame(ActorID::Cloaking_device_icon, 0, drawPos);
            break;
        }
      }
    }
  }
}


void HudRenderer::drawHealthBar(const data::PlayerModel& playerModel) const
{
  // Health slices start at col 20, row 4. The first 9 are for the "0 health"
  // animation

  // The model has a range of 1-9 for health, but the HUD shows only 8
  // slices, with a special animation for having 1 point of health.
  const auto numFullSlices = playerModel.health() - 1;
  if (numFullSlices > 0)
  {
    for (int i = 0; i < NUM_HEALTH_SLICES; ++i)
    {
      const auto sliceIndex = i < numFullSlices ? 9 : 10;

      // clang-format off
      mpStatusSpriteSheetRenderer->renderTileSlice(
        sliceIndex + 20 + 4*40,
        base::Vec2{24 + i, GameTraits::mapviewportSize.height + 1});
      // clang-format on
    }
  }
  else
  {
    const auto animationOffset = mElapsedFrames;

    for (int i = 0; i < NUM_HEALTH_SLICES; ++i)
    {
      const auto sliceIndex = (i + animationOffset) % 9;

      // clang-format off
      mpStatusSpriteSheetRenderer->renderTileSlice(
        sliceIndex + 20 + 4*40,
        base::Vec2{24 + i, GameTraits::mapviewportSize.height + 1});
      // clang-format on
    }
  }
}


void HudRenderer::drawCollectedLetters(
  const data::PlayerModel& playerModel) const
{
  for (const auto letter : playerModel.collectedLetters())
  {
    // The draw position is the same for all cases, because each actor
    // includes a draw offset in its actor info that positions it correctly.
    switch (letter)
    {
      case CollectableLetterType::N:
        drawActorFrame(
          ActorID::Letter_collection_indicator_N, 0, LETTER_INDICATOR_POSITION);
        break;

      case CollectableLetterType::U:
        drawActorFrame(
          ActorID::Letter_collection_indicator_U, 0, LETTER_INDICATOR_POSITION);
        break;

      case CollectableLetterType::K:
        drawActorFrame(
          ActorID::Letter_collection_indicator_K, 0, LETTER_INDICATOR_POSITION);
        break;

      case CollectableLetterType::E:
        drawActorFrame(
          ActorID::Letter_collection_indicator_E, 0, LETTER_INDICATOR_POSITION);
        break;

      case CollectableLetterType::M:
        drawActorFrame(
          ActorID::Letter_collection_indicator_M, 0, LETTER_INDICATOR_POSITION);
        break;
    }
  }
}


void HudRenderer::drawRadar(const base::ArrayView<base::Vec2> positions) const
{
  auto drawDots = [&]() {
    for (const auto& position : positions)
    {
      const auto dotPosition = position + RADAR_CENTER_OFFSET_RELATIVE;
      mpRenderer->drawPoint(dotPosition, RADAR_DOT_COLOR);
    }

    const auto blinkColorIndex =
      mElapsedFrames % NUM_RADAR_BLINK_STEPS + RADAR_BLINK_START_COLOR_INDEX;
    const auto blinkColor = data::GameTraits::INGAME_PALETTE[blinkColorIndex];
    mpRenderer->drawPoint(RADAR_CENTER_OFFSET_RELATIVE, blinkColor);
  };


  if (mpOptions->mPerElementUpscalingEnabled)
  {
    {
      const auto saved = mRadarSurface.bindAndReset();
      mpRenderer->clear({0, 0, 0, 0});
      drawDots();
    }

    mRadarSurface.render(RADAR_POS_X, RADAR_POS_Y);
  }
  else
  {
    const auto saved = renderer::saveState(mpRenderer);
    mpRenderer->setGlobalTranslation(
      mpRenderer->globalTranslation() + base::Vec2{RADAR_POS_X, RADAR_POS_Y});

    drawDots();
  }
}


void HudRenderer::drawActorFrame(
  const ActorID id,
  const int frame,
  const base::Vec2& pos) const
{
  const auto& frameData = mpSpriteFactory->actorFrameData(id, frame);
  const auto destRect = base::Rect<int>{
    data::tileVectorToPixelVector(pos + frameData.mDrawOffset),
    data::tileExtentsToPixelExtents(frameData.mDimensions)};
  mpSpriteFactory->textureAtlas().draw(frameData.mImageId, destRect);
}

} // namespace rigel::ui
