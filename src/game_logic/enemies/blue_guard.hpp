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

#include "base/warnings.hpp"
#include "engine/base_components.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS


namespace rigel::game_logic {
  struct GlobalDependencies;
  struct GlobalState;
}


namespace rigel::game_logic::behaviors {

struct BlueGuard {
  static BlueGuard typingOnTerminal() {
    BlueGuard instance;
    instance.mTypingOnTerminal = true;
    return instance;
  }

  static BlueGuard patrolling(const engine::components::Orientation orientation) {
    BlueGuard instance;
    instance.mOrientation = orientation;
    return instance;
  }

  void update(
    GlobalDependencies& dependencies,
    GlobalState& state,
    bool isOnScreen,
    entityx::Entity entity);

  void onHit(
    GlobalDependencies& dependencies,
    GlobalState& state,
    const base::Point<float>& inflictorVelocity,
    entityx::Entity entity);

  void stopTyping(GlobalState& state, entityx::Entity entity);

  engine::components::Orientation mOrientation =
    engine::components::Orientation::Left;
  int mStanceChangeCountdown = 0;
  int mStepsWalked = 0;
  bool mTypingOnTerminal = false;
  bool mIsCrouched = false;
  bool mTypingInterruptedByAttack = false;
};

}
