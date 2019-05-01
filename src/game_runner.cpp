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

#include "game_logic/ingame_systems.hpp"


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

}


GameRunner::GameRunner(
  data::PlayerModel* pPlayerModel,
  const data::GameSessionId& sessionId,
  GameMode::Context context,
  const std::optional<base::Vector> playerPositionOverride,
  const bool showWelcomeMessage
)
  : mWorld(
      pPlayerModel,
      sessionId,
      context,
      playerPositionOverride,
      showWelcomeMessage)
{
  mStateStack.emplace(World{&mWorld});
}


void GameRunner::handleEvent(const SDL_Event& event) {
  if (mGameWasQuit) {
    return;
  }

  if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
    mGameWasQuit = true;
    return;
  }

  std::visit(
    [&event](auto& state) { state.handleEvent(event); },
    mStateStack.top());
}


void GameRunner::updateAndRender(engine::TimeDelta dt) {
  if (mGameWasQuit || levelFinished()) {
    return;
  }

  std::visit(
    [dt](auto& state) { state.updateAndRender(dt); },
    mStateStack.top());
}


void GameRunner::World::handleEvent(const SDL_Event& event) {
  handlePlayerInput(event);
  handleDebugKeys(event);
}


void GameRunner::World::updateAndRender(const engine::TimeDelta dt) {
  updateWorld(dt);
  mpWorld->render();

  if (mShowDebugText) {
    mpWorld->showDebugText();
  }

  mpWorld->processEndOfFrameActions();
}


void GameRunner::World::updateWorld(const engine::TimeDelta dt) {
  auto update = [this]() {
    mpWorld->updateGameLogic(mPlayerInput);
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
  }
}


void GameRunner::World::handlePlayerInput(const SDL_Event& event) {
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


void GameRunner::World::handleDebugKeys(const SDL_Event& event) {
  if (event.type != SDL_KEYDOWN || event.key.repeat != 0) {
    return;
  }

  auto& debuggingSystem = mpWorld->mpSystems->debuggingSystem();
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
  }
}

}
