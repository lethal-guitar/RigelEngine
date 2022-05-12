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

#pragma once

#include "base/array_view.hpp"
#include "data/actor_ids.hpp"
#include "data/player_model.hpp"
#include "engine/tiled_texture.hpp"
#include "renderer/texture.hpp"

#include <cstdint>
#include <unordered_map>


namespace rigel
{

namespace data
{
struct GameOptions;
class Image;
} // namespace data

namespace engine
{
class SpriteFactory;
}


namespace ui
{

inline bool isVisibleOnRadar(const base::Vec2& position)
{
  // clang-format off
  return
    position.x >= -16 && position.x < 16 &&
    position.y >= -16 && position.y < 16;
  // clang-format on
}


constexpr auto HUD_HEIGHT_BOTTOM = 4;
constexpr auto HUD_WIDTH_RIGHT = 6;
constexpr auto HUD_WIDTH_TOTAL = HUD_WIDTH_RIGHT + 32;


class HudRenderer
{
public:
  HudRenderer(
    int levelNumber,
    const data::GameOptions* pOptions,
    renderer::Renderer* pRenderer,
    engine::TiledTexture* pStatusSpriteSheetRenderer,
    const engine::SpriteFactory* pSpriteFactory);

  void updateAnimation();

  void renderClassicHud(
    const data::PlayerModel& playerModel,
    base::ArrayView<base::Vec2> radarPositions);

  void renderWidescreenHud(
    int viewportWidth,
    const data::PlayerModel& playerModel,
    base::ArrayView<base::Vec2> radarPositions);

private:
  void drawLeftSideExtension(int viewportWidth) const;
  void
    drawInventory(const std::vector<data::InventoryItemType>& inventory) const;
  void drawHealthBar(const data::PlayerModel& playerModel) const;
  void drawCollectedLetters(const data::PlayerModel& playerModel) const;
  void drawRadar(base::ArrayView<base::Vec2> positions) const;
  void drawActorFrame(data::ActorID id, int frame, const base::Vec2& pos) const;

  const int mLevelNumber;
  renderer::Renderer* mpRenderer;
  const data::GameOptions* mpOptions;

  std::uint32_t mElapsedFrames = 0;

  engine::TiledTexture* mpStatusSpriteSheetRenderer;
  const engine::SpriteFactory* mpSpriteFactory;
  mutable renderer::RenderTargetTexture mRadarSurface;
};

} // namespace ui
} // namespace rigel
