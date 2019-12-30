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

#include "base/warnings.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

namespace rigel::engine { class RandomNumberGenerator; }
namespace rigel::game_logic {
  struct GlobalDependencies;
  struct GlobalState;
}


namespace rigel::game_logic::behaviors {

struct WallWalker {
  enum class Direction {
    Up,
    Down,
    Left,
    Right
  };

  explicit WallWalker(engine::RandomNumberGenerator& rng);

  void update(
    GlobalDependencies& dependencies,
    GlobalState& state,
    bool isOnScreen,
    entityx::Entity entity);

  Direction mDirection;
  int mFramesUntilDirectionSwitch = 20;
  int mMovementToggle = 0;
  bool mShouldSkipThisFrame = false;
};

}
