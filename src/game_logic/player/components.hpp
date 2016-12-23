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
#include "engine/timing.hpp"

RIGEL_DISABLE_WARNINGS
#include <boost/optional.hpp>
RIGEL_RESTORE_WARNINGS


namespace rigel { namespace game_logic {

namespace player {

enum class Orientation {
  None,
  Left,
  Right
};


enum class PlayerState {
  Standing,
  Walking,
  Crouching,
  LookingUp,
  ClimbingLadder,
  Airborne,
  Dieing,
  Dead
};


}


namespace detail {

struct DeathAnimationState {
  engine::TimeStepper mStepper;
  int mElapsedFrames = 0;
};

}


struct PlayerInputState {
  bool mMovingLeft = false;
  bool mMovingRight = false;
  bool mMovingUp = false;
  bool mMovingDown = false;
  bool mJumping = false;
  bool mShooting = false;
};


namespace components {

struct PlayerControlled {
  player::Orientation mOrientation = player::Orientation::Left;
  player::PlayerState mState = player::PlayerState::Standing;

  boost::optional<engine::TimeDelta> mMercyFramesTimeElapsed;
  boost::optional<detail::DeathAnimationState> mDeathAnimationState;

  bool mIsLookingUp = false;
  bool mIsLookingDown = false;

  bool mPerformedInteraction = false;
  bool mPerformedJump = false;
  bool mShotFired = false;


  bool isPlayerDead() const {
    return
      mState == player::PlayerState::Dieing ||
      mState == player::PlayerState::Dead;
  }
};


enum class InteractableType {
  Teleporter
};


struct Interactable {
  explicit Interactable(const InteractableType type)
    : mType(type)
  {
  }

  InteractableType mType;
};

}

}}
