/* Copyright (C) 2020, Nikolai Wuttke. All rights reserved.
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

#include "input_handler.hpp"


namespace rigel {

namespace {

constexpr auto ANALOG_STICK_DEADZONE_X = 10'000;
constexpr auto ANALOG_STICK_DEADZONE_Y = 24'000;
constexpr auto TRIGGER_THRESHOLD = 3'000;


game_logic::PlayerInput combinedInput(
  const game_logic::PlayerInput& baseInput,
  const base::Vector& analogStickVector
) {
  auto combined = baseInput;

  // "Overlay" analog stick movement on top of the digital d-pad movement.
  // This way, button presses and analog stick movements don't cancel each
  // other out.
  combined.mLeft |= analogStickVector.x < 0;
  combined.mRight |= analogStickVector.x > 0;
  combined.mUp |= analogStickVector.y < 0;
  combined.mDown |= analogStickVector.y > 0;

  return combined;
}

}



auto InputHandler::handleEvent(
  const SDL_Event& event,
  const bool playerInShip
) -> MenuCommand {
  const auto isKeyEvent = event.type == SDL_KEYDOWN || event.type == SDL_KEYUP;

  if (isKeyEvent && event.key.repeat == 0) {
    return handleKeyboardInput(event);
  } else {
    return handleControllerInput(event, playerInShip);
  }
}


void InputHandler::reset() {
  mPlayerInput = {};
}


game_logic::PlayerInput InputHandler::fetchInput() {
  const auto input = combinedInput(mPlayerInput, mAnalogStickVector);
  mPlayerInput.resetTriggeredStates();
  return input;
}


auto InputHandler::handleKeyboardInput(const SDL_Event& event) -> MenuCommand {
  auto updateButton = [](game_logic::Button& button, const bool isPressed) {
    button.mIsPressed = isPressed;
    if (isPressed) {
      button.mWasTriggered = true;
    }
  };


  const auto keyPressed = std::uint8_t{event.type == SDL_KEYDOWN};
  const auto keyCode = event.key.keysym.sym;

  if (keyCode == SDLK_UP) {
    mPlayerInput.mUp = keyPressed;
    updateButton(mPlayerInput.mInteract, keyPressed);
  } else if (keyCode == SDLK_DOWN) {
    mPlayerInput.mDown = keyPressed;
  } else if (keyCode == SDLK_LEFT) {
    mPlayerInput.mLeft = keyPressed;
  } else if (keyCode == SDLK_RIGHT) {
    mPlayerInput.mRight = keyPressed;
  } else if (keyCode == SDLK_LCTRL || keyCode == SDLK_RCTRL) {
    updateButton(mPlayerInput.mJump, keyPressed);
  } else if (keyCode == SDLK_LALT || keyCode == SDLK_RALT) {
    updateButton(mPlayerInput.mFire, keyPressed);
  } else if (keyCode == SDLK_F5) {
    if (keyPressed) {
      return MenuCommand::QuickSave;
    }
  } else if (keyCode == SDLK_F7) {
    if (keyPressed) {
      return MenuCommand::QuickLoad;
    }
  }

  return MenuCommand::None;
}


auto InputHandler::handleControllerInput(
  const SDL_Event& event,
  const bool playerInShip
) -> MenuCommand {
  switch (event.type) {
    case SDL_CONTROLLERAXISMOTION:
      switch (event.caxis.axis) {
        case SDL_CONTROLLER_AXIS_LEFTX:
        case SDL_CONTROLLER_AXIS_RIGHTX:
          mAnalogStickVector.x =
            base::applyThreshold(event.caxis.value, ANALOG_STICK_DEADZONE_X);
          break;

        case SDL_CONTROLLER_AXIS_LEFTY:
        case SDL_CONTROLLER_AXIS_RIGHTY:
          {
            // We want to avoid accidental crouching/looking up while the
            // player is walking, but still make it easy to move the ship
            // up/down while flying. Therefore, we use a different vertical
            // deadzone when not in the ship.
            const auto deadZone = playerInShip
              ? ANALOG_STICK_DEADZONE_X : ANALOG_STICK_DEADZONE_Y;

            const auto newY = base::applyThreshold(event.caxis.value, deadZone);
            if (mAnalogStickVector.y >= 0 && newY < 0) {
              mPlayerInput.mInteract.mWasTriggered = true;
            }
            mPlayerInput.mInteract.mIsPressed = newY < 0;
            mAnalogStickVector.y = newY;
          }
          break;

        case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
          {
            const auto triggerPressed = event.caxis.value > TRIGGER_THRESHOLD;

            auto& input = event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT
              ? mPlayerInput.mJump
              : mPlayerInput.mFire;
            if (!input.mIsPressed && triggerPressed) {
              input.mWasTriggered = true;
            }
            input.mIsPressed = triggerPressed;
          }
          break;

        default:
          break;
      }
      break;

    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP:
      {
        const auto buttonPressed = event.type == SDL_CONTROLLERBUTTONDOWN;

        switch (event.cbutton.button) {
          case SDL_CONTROLLER_BUTTON_DPAD_UP:
            mPlayerInput.mUp = buttonPressed;
            mPlayerInput.mInteract.mIsPressed = buttonPressed;
            if (buttonPressed) {
                mPlayerInput.mInteract.mWasTriggered = true;
            }
            break;

          case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            mPlayerInput.mDown = buttonPressed;
            break;

          case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            mPlayerInput.mLeft = buttonPressed;
            break;

          case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            mPlayerInput.mRight = buttonPressed;
            break;

          case SDL_CONTROLLER_BUTTON_A:
          case SDL_CONTROLLER_BUTTON_B:
          case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
            mPlayerInput.mJump.mIsPressed = buttonPressed;
            if (buttonPressed) {
                mPlayerInput.mJump.mWasTriggered = true;
            }
            break;

          case SDL_CONTROLLER_BUTTON_X:
          case SDL_CONTROLLER_BUTTON_Y:
          case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            mPlayerInput.mFire.mIsPressed = buttonPressed;
            if (buttonPressed) {
                mPlayerInput.mFire.mWasTriggered = true;
            }
            break;

          case SDL_CONTROLLER_BUTTON_BACK:
            if (buttonPressed) {
              return MenuCommand::QuickSave;
            }
            break;
        }
      }
      break;
  }

  return MenuCommand::None;
}

}
