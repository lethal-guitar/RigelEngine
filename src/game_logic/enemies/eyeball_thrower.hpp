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

#include "game_logic/global_dependencies.hpp"

#include <variant>


namespace rigel::game_logic::behaviors {

namespace eyeball_thrower {

struct GettingUp {
  int mFramesElapsed = 0;
};

struct Walking {};

struct Attacking {
  int mFramesElapsed = 0;
};

using State = std::variant<GettingUp, Walking, Attacking>;

}


struct EyeballThrower {
  void update(
    GlobalDependencies& dependencies,
    GlobalState& state,
    bool isOnScreen,
    entityx::Entity entity);

  eyeball_thrower::State mState;

  // This stays persistent across state changes, therefore it's stored outside
  // of the state machine.
  int mFramesElapsedInWalkingState = 0;
};

}
