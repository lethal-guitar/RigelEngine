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

#pragma once

#include <base/warnings.hpp>

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <variant>


namespace rigel::engine { class CollisionChecker; }
namespace rigel::engine::events { struct CollidedWithWorld; }
namespace rigel::game_logic {
  struct GlobalDependencies;
  struct GlobalState;
}


namespace rigel::game_logic::behaviors {

struct LivingStatue {
  struct Awakening {
    int mFramesElapsed = 0;
  };

  struct Waiting {
    int mFramesElapsed = 0;
  };

  struct Pouncing {
    int mFramesElapsed = 0;
  };

  struct Landing {};

  using State = std::variant<Awakening, Waiting, Pouncing, Landing>;

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

  void landOnGround(
    const GlobalDependencies& d,
    entityx::Entity entity);
  void ensureNotStuckInWall(
    const GlobalDependencies& d,
    entityx::Entity entity);
  void moveHorizontallyInAir(
    const GlobalDependencies& d,
    entityx::Entity entity);

  State mState;
};

}
