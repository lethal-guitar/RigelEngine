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

#include <catch.hpp>

#include <game_logic/player/state_machine.hpp>


using namespace std;

using namespace rigel;
using namespace game_logic;

using player::Orientation;
using player::State;
using player::WALK_START_DELAY;


TEST_CASE("Player state machine updates according to input") {
  player::StateComponent playerState;
  player::StateMachine stateMachine(&playerState);


  SECTION("State updates correctly in grounded states") {
    playerState.mState = State::Standing;
    playerState.mOrientation = Orientation::Right;

    player::InputState inputs;


    SECTION("State is unchanged when no inputs given") {
      stateMachine.update(20.0, player::InputState{});

      REQUIRE(playerState.mState == State::Standing);
      REQUIRE(playerState.mOrientation == Orientation::Right);
    }


    SECTION("Orientation is changed immediately when horizontal input given") {
      inputs.mMovingLeft = true;

      stateMachine.update(0.0, inputs);

      REQUIRE(playerState.mOrientation == Orientation::Left);


      inputs.mMovingLeft = false;
      inputs.mMovingRight = true;

      stateMachine.update(0.0, inputs);

      REQUIRE(playerState.mOrientation == Orientation::Right);
    }


    SECTION("Pressing up button immediately goes to 'aim upwards' mode") {
      inputs.mMovingUp = true;

      stateMachine.update(0.0, inputs);

      REQUIRE(playerState.mState == State::LookingUp);
    }


    SECTION("Pressing down button immediately goes to 'crouch' mode") {
      inputs.mMovingDown = true;

      stateMachine.update(0.0, inputs);

      REQUIRE(playerState.mState == State::Crouching);
    }


    SECTION("Conflicting vertical inputs are ignored") {
      inputs.mMovingDown = true;
      inputs.mMovingUp = true;

      stateMachine.update(0.0, inputs);

      REQUIRE(playerState.mState == State::Standing);
    }


    SECTION("Holding horizontal direction goes to walk mode after short delay") {
      inputs.mMovingRight = true;

      stateMachine.update(WALK_START_DELAY / 2.0, inputs);
      REQUIRE(playerState.mState == State::Standing);

      stateMachine.update(WALK_START_DELAY / 2.0, inputs);
      REQUIRE(playerState.mState == State::Walking);


      SECTION("Letting go of horizontal direction immediately goes back to standing mode") {
        inputs.mMovingRight = false;

        stateMachine.update(0.0, inputs);
        REQUIRE(playerState.mState == State::Standing);


        SECTION("Consecutive horizontal input awaits delay again") {
          inputs.mMovingRight = true;

          stateMachine.update(0.0, inputs);
          REQUIRE(playerState.mState == State::Standing);

          stateMachine.update(WALK_START_DELAY * 1.1, inputs);
          REQUIRE(playerState.mState == State::Walking);
        }
      }


      SECTION("Vertical inputs temporarily override walk state") {
        inputs.mMovingUp = true;

        stateMachine.update(0.0, inputs);
        REQUIRE(playerState.mState == State::LookingUp);

        inputs.mMovingUp = false;
        inputs.mMovingDown = true;

        stateMachine.update(0.0, inputs);
        REQUIRE(playerState.mState == State::Crouching);

        inputs.mMovingDown = false;

        stateMachine.update(0.0, inputs);
        REQUIRE(playerState.mState == State::Walking);
      }
    }


    SECTION("Conflicting horizontal inputs are ignored") {
      inputs.mMovingLeft = inputs.mMovingRight = true;

      stateMachine.update(WALK_START_DELAY * 4, inputs);

      REQUIRE(playerState.mState == State::Standing);
    }
  }
}
