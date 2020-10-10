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

#include "game_runner.hpp"

#include "base/math_tools.hpp"
#include "common/game_service_provider.hpp"
#include "ui/utils.hpp"

#include <sstream>


namespace rigel {

namespace {

// Update game logic at 15 FPS. This is not exactly the speed at which the
// game runs on period-appropriate hardware, but it's very close, and it nicely
// fits into 60 FPS, giving us 4 render frames for 1 logic update.
//
// On a 486 with a fast graphics card, the game runs at roughly 15.5 FPS, with
// a slower (non-VLB) graphics card, it's roughly 14 FPS. On a fast 386 (40 MHz),
// it's roughly 13 FPS. With 15 FPS, the feel should therefore be very close to
// playing the game on a 486 at the default game speed setting.
constexpr auto GAME_LOGIC_UPDATE_DELAY = 1.0/15.0;

// Controller handling stuff.
// TODO: This should move into its own file at some point.
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


GameRunner::GameRunner(
  data::PlayerModel* pPlayerModel,
  const data::GameSessionId& sessionId,
  GameMode::Context context,
  const std::optional<base::Vector> playerPositionOverride,
  const bool showWelcomeMessage
)
  : mContext(context)
  , mMenu(context, pPlayerModel, sessionId)
  , mWorld(
      pPlayerModel,
      sessionId,
      context,
      playerPositionOverride,
      showWelcomeMessage)
{
}


void GameRunner::handleEvent(const SDL_Event& event) {
  if (gameQuit() || requestedGameToLoad()) {
    return;
  }

  mMenu.handleEvent(event);
  if (mMenu.isActive()) {
    // The menu overrides game event handling when it is active, therefore
    // stop here
    return;
  }

  handlePlayerKeyboardInput(event);
  handlePlayerGameControllerInput(event);

  const auto debugModeEnabled =
    mContext.mpServiceProvider->commandLineOptions().mDebugModeEnabled;
  if (debugModeEnabled) {
    handleDebugKeys(event);
  }
}


void GameRunner::updateAndRender(engine::TimeDelta dt) {
  if (gameQuit() || levelFinished() || requestedGameToLoad()) {
    // TODO: This is a workaround to make the fadeout on quitting work.
    // Would be good to find a better way to do this.
    mWorld.render();
    return;
  }

  if (updateMenu(dt)) {
    return;
  }

  updateWorld(dt);
  mWorld.render();
  renderDebugText();
  mWorld.processEndOfFrameActions();
}


void GameRunner::updateWorld(const engine::TimeDelta dt) {
  auto update = [this]() {
    mWorld.updateGameLogic(combinedInput(mPlayerInput, mAnalogStickVector));
    mPlayerInput.resetTriggeredStates();
  };


  if (mSingleStepping) {
    if (mDoNextSingleStep) {
      update();
      mDoNextSingleStep = false;
    }
  } else {
    mAccumulatedTime += dt;
    for (;
      mAccumulatedTime >= GAME_LOGIC_UPDATE_DELAY;
      mAccumulatedTime -= GAME_LOGIC_UPDATE_DELAY
    ) {
      update();
    }

    mWorld.mpState->mRenderingSystem.updateBackdropAutoScrolling(dt);
  }
}


bool GameRunner::updateMenu(const engine::TimeDelta dt) {
  if (mMenu.isActive()) {
    mPlayerInput = {};

    if (mMenu.isTransparent()) {
      mWorld.render();
    }

    const auto result = mMenu.updateAndRender(dt);

    if (result == ui::IngameMenu::UpdateResult::FinishedNeedsFadeout) {
      mContext.mpServiceProvider->fadeOutScreen();
      mWorld.render();
      mContext.mpServiceProvider->fadeInScreen();
    }

    return true;
  }

  return false;
}


void GameRunner::handlePlayerKeyboardInput(const SDL_Event& event) {
  const auto isKeyEvent = event.type == SDL_KEYDOWN || event.type == SDL_KEYUP;
  if (!isKeyEvent || event.key.repeat != 0) {
    return;
  }

  const auto keyPressed = std::uint8_t{event.type == SDL_KEYDOWN};
  switch (event.key.keysym.sym) {
    // TODO: DRY this up a little?
    case SDLK_UP:
      mPlayerInput.mUp = keyPressed;
      mPlayerInput.mInteract.mIsPressed = keyPressed;
      if (keyPressed) {
        mPlayerInput.mInteract.mWasTriggered = true;
      }
      break;

    case SDLK_DOWN:
      mPlayerInput.mDown = keyPressed;
      break;

    case SDLK_LEFT:
      mPlayerInput.mLeft = keyPressed;
      break;

    case SDLK_RIGHT:
      mPlayerInput.mRight = keyPressed;
      break;

    case SDLK_LCTRL:
    case SDLK_RCTRL:
      mPlayerInput.mJump.mIsPressed = keyPressed;
      if (keyPressed) {
        mPlayerInput.mJump.mWasTriggered = true;
      }
      break;

    case SDLK_LALT:
    case SDLK_RALT:
      mPlayerInput.mFire.mIsPressed = keyPressed;
      if (keyPressed) {
        mPlayerInput.mFire.mWasTriggered = true;
      }
      break;
  }
}


void GameRunner::handlePlayerGameControllerInput(const SDL_Event& event) {
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
            const auto deadZone =
              mWorld.mpState->mPlayer.stateIs<game_logic::InShip>()
              ? ANALOG_STICK_DEADZONE_X
              : ANALOG_STICK_DEADZONE_Y;

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
        }
      }
      break;
  }
}


void GameRunner::handleDebugKeys(const SDL_Event& event) {
  if (event.type != SDL_KEYDOWN || event.key.repeat != 0) {
    return;
  }

  auto& debuggingSystem = mWorld.mpState->mDebuggingSystem;
  switch (event.key.keysym.sym) {
    case SDLK_b:
      debuggingSystem.toggleBoundingBoxDisplay();
      break;

    case SDLK_c:
      debuggingSystem.toggleWorldCollisionDataDisplay();
      break;

    case SDLK_d:
      mShowDebugText = !mShowDebugText;
      break;

    case SDLK_g:
      debuggingSystem.toggleGridDisplay();
      break;

    case SDLK_s:
      mSingleStepping = !mSingleStepping;
      break;

    case SDLK_SPACE:
      if (mSingleStepping) {
        mDoNextSingleStep = true;
      }
      break;

    case SDLK_F10:
      {
        auto& player = mWorld.mpState->mPlayer;
        player.mGodModeOn = !player.mGodModeOn;
      }
      break;

    case SDLK_F11:
      mLevelFinishedByDebugKey = true;
      break;
  }
}


void GameRunner::renderDebugText() {
  std::stringstream debugText;

  if (mWorld.mpState->mPlayer.mGodModeOn) {
    debugText << "GOD MODE on\n";
  }

  if (mShowDebugText) {
    mWorld.printDebugText(debugText);
  }

  ui::drawText(debugText.str(), 0, 32, {255, 255, 255, 255});
}

}
