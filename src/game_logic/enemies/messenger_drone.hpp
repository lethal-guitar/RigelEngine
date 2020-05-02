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

struct MessengerDrone {
  enum class Message {
    // Enum values are used to index into the message sequence array
    YourBrainIsOurs = 0,
    BringBackTheBrain = 1,
    LiveFromRigel = 2,
    Die = 3,
    CantEscape = 4
  };

  explicit MessengerDrone(const Message message)
    : mMessage(message)
  {
  }

  void update(
    GlobalDependencies& dependencies,
    GlobalState& state,
    bool isOnScreen,
    entityx::Entity entity);

  enum class State {
    AwaitActivation,
    FlyIn,
    ShowingMessage,
    FlyOut
  };

  State mState = State::AwaitActivation;
  engine::components::Orientation mOrientation =
    engine::components::Orientation::Left;
  Message mMessage;

  std::uint32_t mMessageStep = 0;
  int mElapsedFrames = 0;
};

}
