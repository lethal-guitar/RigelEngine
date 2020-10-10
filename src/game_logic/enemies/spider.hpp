/* Copyright (C) 2018, Nikolai Wuttke. All rights reserved.
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

#include "base/warnings.hpp"
#include "engine/base_components.hpp"
#include "engine/physical_components.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/enemies/simple_walker.hpp"
#include "game_logic/player.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS


namespace rigel::game_logic {
  struct GlobalDependencies;
  struct GlobalState;
}


namespace rigel::game_logic::behaviors {

struct Spider {
  void update(
    GlobalDependencies& dependencies,
    GlobalState& state,
    bool isOnScreen,
    entityx::Entity entity);

  void onCollision(
    GlobalDependencies& dependencies,
    GlobalState& state,
    const engine::events::CollidedWithWorld& event,
    entityx::Entity entity);

  void walkOnFloor(entityx::Entity entity);

  enum class State {
    Uninitialized,
    OnCeiling,
    Falling,
    OnFloor,
    ClingingToPlayer
  };

  // TODO: Use variant pattern for states?
  behaviors::SimpleWalker mWalkerBehavior{nullptr};
  State mState = State::Uninitialized;
  engine::components::Orientation mPreviousPlayerOrientation;
  int mShakeOffProgress = 0;
  SpiderClingPosition mClingPosition;
};

}
