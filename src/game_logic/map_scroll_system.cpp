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

#include "map_scroll_system.hpp"

#include "base/math_tools.hpp"
#include "data/game_traits.hpp"
#include "data/map.hpp"
#include "game_logic/player/components.hpp"

namespace ex = entityx;


namespace rigel { namespace game_logic {

namespace {

using player::PlayerState;
using namespace engine::components;
using namespace game_logic::components;


const base::Rect<int> DefaultDeadZone{
  {11, 2},
  {
    data::GameTraits::mapViewPortWidthTiles - 22,
    data::GameTraits::mapViewPortHeightTiles - 3,
  }
};


const base::Rect<int> ClimbingDeadZone{
  {11, 7},
  {
    data::GameTraits::mapViewPortWidthTiles - 22,
    data::GameTraits::mapViewPortHeightTiles - 14,
  }
};


base::Rect<int> scrollDeadZoneForState(const PlayerState state) {
  switch (state) {
    case PlayerState::ClimbingLadder:
      return ClimbingDeadZone;

    default:
      return DefaultDeadZone;
  }
}

}


MapScrollSystem::MapScrollSystem(
  base::Vector* pScrollOffset,
  entityx::Entity player,
  const data::map::Map& map
)
  : mPlayer(player)
  , mpScrollOffset(pScrollOffset)
  , mMaxScrollOffset(base::Extents{
    static_cast<int>(map.width() - data::GameTraits::mapViewPortWidthTiles),
    static_cast<int>(map.height() - data::GameTraits::mapViewPortHeightTiles)})
{
}


void MapScrollSystem::update() {
  updateManualScrolling();
  updateScrollOffset();
}


void MapScrollSystem::updateManualScrolling() {
  const auto& state = *mPlayer.component<PlayerControlled>();

  if (state.mIsLookingDown) {
    mpScrollOffset->y += 2;
  }
  if (state.mIsLookingUp) {
    mpScrollOffset->y -= 2;
  }
}


void MapScrollSystem::updateScrollOffset() {
  const auto& state = *mPlayer.component<PlayerControlled>();
  const auto& originalPlayerBounds = *mPlayer.component<BoundingBox>();
  const auto& playerPosition = *mPlayer.component<WorldPosition>();

  auto playerBounds = originalPlayerBounds;
  playerBounds.topLeft =
    (playerPosition - base::Vector{0, playerBounds.size.height - 1});

  auto worldSpaceDeadZone = scrollDeadZoneForState(state.mState);
  worldSpaceDeadZone.topLeft += *mpScrollOffset;

  // horizontal
  const auto offsetLeft =
    std::max(0, worldSpaceDeadZone.topLeft.x -  playerPosition.x);
  const auto offsetRight =
    std::min(0,
      worldSpaceDeadZone.bottomRight().x - playerBounds.bottomRight().x);
  const auto offsetX = -offsetLeft - offsetRight;

  // vertical
  const auto offsetTop =
    std::max(0, worldSpaceDeadZone.top() - playerBounds.top());
  const auto offsetBottom =
    std::min(0,
      worldSpaceDeadZone.bottom() - playerBounds.bottom());
  const auto offsetY = -offsetTop - offsetBottom;

  // Update and clamp
  *mpScrollOffset += base::Vector(offsetX, offsetY);

  mpScrollOffset->x =
    base::clamp(mpScrollOffset->x, 0, mMaxScrollOffset.width);
  mpScrollOffset->y =
    base::clamp(mpScrollOffset->y, 0, mMaxScrollOffset.height);
}


void MapScrollSystem::centerViewOnPlayer() {
  *mpScrollOffset = {};
  updateScrollOffset();
}

}}
