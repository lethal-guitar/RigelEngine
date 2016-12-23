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

#include "data/game_traits.hpp"
#include "data/map.hpp"
#include "game_logic/player/components.hpp"
#include "utils/math_tools.hpp"

namespace ex = entityx;


namespace rigel { namespace game_logic {

namespace {

using player::PlayerState;
using namespace engine::components;
using namespace game_logic::components;


RIGEL_DISABLE_GLOBAL_CTORS_WARNING

const base::Rect<int> DefaultDeadZone{
  {11, 2},
  {
    data::GameTraits::mapViewPortWidthTiles - 23,
    data::GameTraits::mapViewPortHeightTiles - 3,
  }
};


const base::Rect<int> ClimbingDeadZone{
  {11, 7},
  {
    data::GameTraits::mapViewPortWidthTiles - 23,
    data::GameTraits::mapViewPortHeightTiles - 14,
  }
};

RIGEL_RESTORE_WARNINGS


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


void MapScrollSystem::update(
  entityx::EntityManager& es,
  entityx::EventManager& events,
  entityx::TimeDelta dt
) {
  const auto& state = *mPlayer.component<PlayerControlled>().get();
  const auto& bbox = *mPlayer.component<BoundingBox>().get();
  const auto& worldPosition = *mPlayer.component<WorldPosition>().get();

  updateScrollOffset(state, worldPosition, bbox, dt);
}


void MapScrollSystem::updateScrollOffset(
  const PlayerControlled& state,
  const WorldPosition& playerPosition,
  const BoundingBox& originalPlayerBounds,
  const ex::TimeDelta dt
) {
  if (updateAndCheckIfDesiredTicksElapsed(mTimeStepper, 2, dt)) {
    // We can just always update here, since the code below will clamp the
    // scroll offset properly
    if (state.mIsLookingDown) {
      mpScrollOffset->y += 2;
    }
    if (state.mIsLookingUp) {
      mpScrollOffset->y -= 2;
    }
  }

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
    utils::clamp(mpScrollOffset->x, 0, mMaxScrollOffset.width);
  mpScrollOffset->y =
    utils::clamp(mpScrollOffset->y, 0, mMaxScrollOffset.height);
}

}}
