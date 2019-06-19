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


namespace rigel::game_logic {

struct Button {
  /** True if the button is currently pressed (down) */
  bool mIsPressed = false;

  /** True if there was at least one button-down event since the last update */
  bool mWasTriggered = false;
};


struct PlayerInput {
  bool mLeft = false;
  bool mRight = false;
  bool mUp = false;
  bool mDown = false;

  Button mInteract;
  Button mJump;
  Button mFire;

  void resetTriggeredStates() {
    mInteract.mWasTriggered = false;
    mJump.mWasTriggered = false;
    mFire.mWasTriggered = false;
  }
};

}
