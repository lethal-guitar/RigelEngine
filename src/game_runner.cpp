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

#include "base/match.hpp"
#include "common/game_service_provider.hpp"
#include "common/user_profile.hpp"
#include "game_logic/ingame_systems.hpp"
#include "loader/resource_loader.hpp"


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

constexpr auto SAVE_SLOT_NAME_ENTRY_POS_X = 14;
constexpr auto SAVE_SLOT_NAME_ENTRY_START_POS_Y = 6;
constexpr auto SAVE_SLOT_NAME_HEIGHT = 2;

constexpr auto MAX_SAVE_SLOT_NAME_LENGTH = 18;


bool isNonRepeatKeyDown(const SDL_Event& event) {
  return event.type == SDL_KEYDOWN && event.key.repeat == 0;
}


auto createSavedGame(
  const data::GameSessionId& sessionId,
  const data::PlayerModel& playerModel
) {
  return data::SavedGame{
    sessionId,
    playerModel.tutorialMessages(),
    "", // will be filled in on saving
    playerModel.weapon(),
    playerModel.ammo(),
    playerModel.score()
  };
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
  , mSavedGame(createSavedGame(sessionId, *pPlayerModel))
  , mWorld(
      pPlayerModel,
      sessionId,
      context,
      playerPositionOverride,
      showWelcomeMessage)
{
  mStateStack.emplace(World{&mWorld, &mContext.mpUserProfile->mOptions});
}


void GameRunner::handleEvent(const SDL_Event& event) {
  auto handleSavedGameNameEntryEvent = [&, this](
    SavedGameNameEntry& state
  ) {
    auto leaveTextEntry = [this]() {
      SDL_StopTextInput();
      mStateStack.pop();
      mStateStack.pop();
      fadeToWorld();
    };

    if (isNonRepeatKeyDown(event)) {
      switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
          leaveTextEntry();
          return;

        case SDLK_RETURN:
        case SDLK_KP_ENTER:
          saveGame(state.mSlotIndex, state.mTextEntryWidget.text());
          leaveTextEntry();
          return;

        default:
          break;
      }
    }

    state.mTextEntryWidget.handleEvent(event);
  };


  if (mGameWasQuit) {
    return;
  }

  base::match(mStateStack.top(),
    [&event, this](World& state) {
      if (!handleMenuEnterEvent(event)) {
        state.handleEvent(event);
      }
    },

    [&, this](SavedGameNameEntry& state) {
      handleSavedGameNameEntryEvent(state);
    },

    [&event](Menu& state) {
      state.handleEvent(event);
    },

    [&, this](const ui::OptionsMenu& options) {
      const auto escapePressed = isNonRepeatKeyDown(event) &&
        event.key.keysym.sym == SDLK_ESCAPE;
      if (escapePressed || options.isFinished()) {
        mStateStack.pop();
      }
    });
}


void GameRunner::updateAndRender(engine::TimeDelta dt) {
  if (mGameWasQuit || levelFinished()) {
    return;
  }

  base::match(mStateStack.top(),
    [dt, this](ui::OptionsMenu& state) {
      state.updateAndRender(dt);
      mWorld.render(0.0f);
    },

    [dt](auto& state) { state.updateAndRender(dt); });
}


bool GameRunner::handleMenuEnterEvent(const SDL_Event& event) {
  auto leaveMenuHook = [this](const ExecutionResult&) {
    leaveMenu();
  };

  auto leaveMenuWithFadeHook = [this](const ExecutionResult&) {
    leaveMenu();
    fadeToWorld();
  };

  auto quitConfirmEventHook = [this](const SDL_Event& ev) {
    // The user needs to press Y in order to confirm quitting the game, but we
    // want the confirmation to happen when the key is released, not when it's
    // pressed. This is because the "a new high score" screen may appear after
    // quitting the game, and if we were to quit on key down, it's very likely
    // for the key to still be pressed while the new screen appears. This in
    // turn would lead to an undesired letter Y being entered into the high
    // score name entry field, because the text input system would see the key
    // being released and treated as an input.
    //
    // Therefore, we quit on key up. Nevertheless, we still need to prevent the
    // key down event from reaching the script runner, as it would cancel out
    // the quit confirmation dialog otherwise.
    if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_y) {
      return true;
    }
    if (ev.type == SDL_KEYUP && ev.key.keysym.sym == SDLK_y) {
      mGameWasQuit = ev.type == SDL_KEYUP;
      return true;
    }

    return false;
  };


  if (!isNonRepeatKeyDown(event)) {
    return false;
  }

  switch (event.key.keysym.sym) {
    case SDLK_ESCAPE:
      enterMenu("2Quit_Select", leaveMenuHook, quitConfirmEventHook);
      break;

    case SDLK_F1:
      if (auto pWorld = std::get_if<World>(&mStateStack.top())) {
        pWorld->mPlayerInput = {};
      }
      mStateStack.push(ui::OptionsMenu{&mContext.mpUserProfile->mOptions});
      break;

    case SDLK_F2:
      enterMenu(
        "Save_Game",
        [this](const auto& result) { onSaveGameMenuFinished(result); });
      break;

    case SDLK_F3:
      enterMenu(
        "Restore_Game",
        [this](const auto& result) { onRestoreGameMenuFinished(result); });
      break;

    case SDLK_h:
      enterMenu("&Instructions", leaveMenuWithFadeHook);
      break;

    case SDLK_p:
      enterMenu("Paused", leaveMenuHook);
      break;

    default:
      return false;
  }

  return true;
}


template <typename ScriptEndHook, typename EventHook>
void GameRunner::enterMenu(
  const char* scriptName,
  ScriptEndHook&& scriptEndedHook,
  EventHook&& eventHook
) {
  if (auto pWorld = std::get_if<World>(&mStateStack.top())) {
    pWorld->mPlayerInput = {};
  }

  runScript(mContext, scriptName);
  mStateStack.push(Menu{
    mContext.mpScriptRunner,
    std::forward<ScriptEndHook>(scriptEndedHook),
    std::forward<EventHook>(eventHook)});
}


void GameRunner::leaveMenu() {
  mStateStack.pop();
}


void GameRunner::fadeToWorld() {
  mContext.mpServiceProvider->fadeOutScreen();
  mWorld.render(0.0f);
  mContext.mpServiceProvider->fadeInScreen();
}


void GameRunner::onRestoreGameMenuFinished(const ExecutionResult& result) {
  using STT = ui::DukeScriptRunner::ScriptTerminationType;

  if (result.mTerminationType == STT::AbortedByUser) {
    leaveMenu();
    fadeToWorld();
  } else {
    const auto slotIndex = result.mSelectedPage;
    const auto& slot = mContext.mpUserProfile->mSaveSlots[*slotIndex];
    if (slot) {
      mContext.mpServiceProvider->scheduleStartFromSavedGame(*slot);
    } else {
      // When selecting an empty slot, we show a message ("no game in this
      // slot") and then return to the save slot selection menu.
      // The latter stays on the stack, we push another menu state on top of
      // the stack for showing the message.
      enterMenu(
        "No_Game_Restore",
        [this](const auto&) {
          leaveMenu();
          runScript(mContext, "Restore_Game");
        });
    }
  }
}


void GameRunner::onSaveGameMenuFinished(const ExecutionResult& result) {
  using STT = ui::DukeScriptRunner::ScriptTerminationType;

  if (result.mTerminationType == STT::AbortedByUser) {
    leaveMenu();
    fadeToWorld();
  } else {
    // HACK: This is to make the menu selection indicator (spinning arrow)
    // disappear.
    // TODO: Find a more obvious/clear way to do this. Ideally, the script
    // runner would do this as part of the updateAndRender() call that runs the
    // script's final instruction.
    mContext.mpScriptRunner->updateAndRender(0.0);

    const auto slotIndex = *result.mSelectedPage;
    SDL_StartTextInput();
    mStateStack.push(SavedGameNameEntry{mContext, slotIndex});
  }
}


void GameRunner::saveGame(const int slotIndex, std::string_view name) {
  auto savedGame = mSavedGame;
  savedGame.mName = name;

  mContext.mpUserProfile->mSaveSlots[slotIndex] = savedGame;
  mContext.mpUserProfile->saveToDisk();
}


void GameRunner::World::handleEvent(const SDL_Event& event) {
  handlePlayerInput(event);
  handleDebugKeys(event);
}


void GameRunner::World::updateAndRender(const engine::TimeDelta dt) {
  updateWorld(dt);

  const auto interpolation = mpOptions->mMotionSmoothing
    ? static_cast<float>(mAccumulatedTime / GAME_LOGIC_UPDATE_DELAY)
    : 0.0f;
  mpWorld->render(interpolation);

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

    mpWorld->mpSystems->updateBackdropAutoScrolling(dt);
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
  if (!isNonRepeatKeyDown(event)) {
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


GameRunner::SavedGameNameEntry::SavedGameNameEntry(
  GameMode::Context context,
  const int slotIndex
)
  : mTextEntryWidget(
      context.mpUiRenderer,
      SAVE_SLOT_NAME_ENTRY_POS_X,
      SAVE_SLOT_NAME_ENTRY_START_POS_Y + slotIndex * SAVE_SLOT_NAME_HEIGHT,
      MAX_SAVE_SLOT_NAME_LENGTH,
      ui::TextEntryWidget::Style::BigText)
  , mSlotIndex(slotIndex)
{
}

}
