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
#include "engine/physical_components.hpp"
#include "game_logic/player.hpp"

namespace ex = entityx;


namespace rigel { namespace game_logic {

namespace {

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


base::Rect<int> scrollDeadZone(const Player& player) {
  if (player.stateIs<ClimbingLadder>()) {
    return ClimbingDeadZone;
  }

  return DefaultDeadZone;
}


base::Vector offsetToDeadZone(
  const Player& player,
  const base::Vector& cameraPosition
) {
  const auto playerBounds = player.worldSpaceCollisionBox();

  auto worldSpaceDeadZone = scrollDeadZone(player);
  worldSpaceDeadZone.topLeft += cameraPosition;

  // horizontal
  const auto offsetLeft =
    std::max(0, worldSpaceDeadZone.topLeft.x -  playerBounds.topLeft.x);
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

  return {offsetX, offsetY};
}

}


MapScrollSystem::MapScrollSystem(
  base::Vector* pScrollOffset,
  const Player* pPlayer,
  const data::map::Map& map
)
  : mpPlayer(pPlayer)
  , mpScrollOffset(pScrollOffset)
  , mMaxScrollOffset(base::Extents{
    static_cast<int>(map.width() - data::GameTraits::mapViewPortWidthTiles),
    static_cast<int>(map.height() - data::GameTraits::mapViewPortHeightTiles)})
{
}


void MapScrollSystem::update(const PlayerInput& input) {
  updateManualScrolling(input);
  updateScrollOffset();
}


void MapScrollSystem::updateManualScrolling(const PlayerInput& input) {
  if (mpPlayer->stateIs<OnGround>() || mpPlayer->stateIs<OnPipe>()) {
    if (input.mDown) {
      mpScrollOffset->y += 2;
    }
    if (input.mUp) {
      mpScrollOffset->y -= 2;
    }
  }
}


void MapScrollSystem::updateScrollOffset() {
  const auto [offsetX, offsetY] = offsetToDeadZone(*mpPlayer, *mpScrollOffset);

  setPosition(*mpScrollOffset + base::Vector(offsetX, offsetY));
}


void MapScrollSystem::setPosition(const base::Vector position) {
  mpScrollOffset->x = base::clamp(position.x, 0, mMaxScrollOffset.width);
  mpScrollOffset->y = base::clamp(position.y, 0, mMaxScrollOffset.height);
}


void MapScrollSystem::centerViewOnPlayer() {
  *mpScrollOffset = {};
  updateScrollOffset();
}

}}
