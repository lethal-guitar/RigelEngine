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

#include "ingame_menu.hpp"

#include "base/match.hpp"
#include "common/game_service_provider.hpp"
#include "common/user_profile.hpp"
#include "loader/resource_loader.hpp"


namespace rigel::ui {

namespace {

constexpr auto SAVE_SLOT_NAME_ENTRY_POS_X = 14;
constexpr auto SAVE_SLOT_NAME_ENTRY_START_POS_Y = 6;
constexpr auto SAVE_SLOT_NAME_HEIGHT = 2;
constexpr auto MAX_SAVE_SLOT_NAME_LENGTH = 18;


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


bool isNonRepeatKeyDown(const SDL_Event& event) {
  return event.type == SDL_KEYDOWN && event.key.repeat == 0;
}


bool isConfirmButton(const SDL_Event& event) {
  const auto enterPressed = isNonRepeatKeyDown(event) &&
    (event.key.keysym.sym == SDLK_RETURN ||
     event.key.keysym.sym == SDLK_KP_ENTER);
  const auto buttonAPressed = event.type == SDL_CONTROLLERBUTTONDOWN &&
    event.cbutton.button == SDL_CONTROLLER_BUTTON_A;

  return enterPressed || buttonAPressed;
}


bool isCancelButton(const SDL_Event& event) {
  const auto escapePressed = isNonRepeatKeyDown(event) &&
    event.key.keysym.sym == SDLK_ESCAPE;
  const auto buttonBPressed = event.type == SDL_CONTROLLERBUTTONDOWN &&
    event.cbutton.button == SDL_CONTROLLER_BUTTON_B;

  return escapePressed || buttonBPressed;
}

}


void IngameMenu::ScriptedMenu::handleEvent(const SDL_Event& event) {
  if (!mEventHook(event)) {
    mpScriptRunner->handleEvent(event);
  }
}


void IngameMenu::ScriptedMenu::updateAndRender(const engine::TimeDelta dt) {
  mpScriptRunner->updateAndRender(dt);

  if (mpScriptRunner->hasFinishedExecution()) {
    mScriptFinishedHook(*mpScriptRunner->result());
  }
}


IngameMenu::SavedGameNameEntry::SavedGameNameEntry(
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


IngameMenu::IngameMenu(
  GameMode::Context context,
  const data::PlayerModel* pPlayerModel,
  const data::GameSessionId& sessionId
)
  : mContext(context)
  , mSavedGame(createSavedGame(sessionId, *pPlayerModel))
{
}


void IngameMenu::handleEvent(const SDL_Event& event) {
  if (mQuitRequested || mRequestedGameToLoad) {
    return;
  }

  if (mStateStack.empty()) {
    handleMenuEnterEvent(event);
  } else {
    // We want to process menu navigation and similar events in updateAndRender,
    // so we only add them to a queue here.
    mEventQueue.push_back(event);
  }
}


auto IngameMenu::updateAndRender(engine::TimeDelta dt) -> UpdateResult {
  mFadeoutNeeded = false;

  handleMenuActiveEvents();

  if (!mStateStack.empty()) {
    base::match(mStateStack.top(),
      [dt, this](SavedGameNameEntry& state) {
        mContext.mpScriptRunner->updateAndRender(dt);
        state.updateAndRender(dt);
      },

      [dt](auto& state) { state.updateAndRender(dt); });
  }

  if (mStateStack.empty()) {
    return mFadeoutNeeded
      ? UpdateResult::FinishedNeedsFadeout
      : UpdateResult::Finished;
  } else {
    return UpdateResult::StillActive;
  }
}


void IngameMenu::onRestoreGameMenuFinished(const ExecutionResult& result) {
  auto showErrorMessageScript = [this](const char* scriptName) {
    // When selecting a slot that can't be loaded, we show a message and
    // then return to the save slot selection menu.  The latter stays on the
    // stack, we push another menu state on top of the stack for showing the
    // message.
    enterScriptedMenu(
      scriptName,
      [this](const auto&) {
        leaveMenu();
        runScript(mContext, "Restore_Game");
      },
      noopEventHook,
      false, // isTransparent
      false); // shouldClearScriptCanvas
  };


  using STT = ui::DukeScriptRunner::ScriptTerminationType;

  if (result.mTerminationType == STT::AbortedByUser) {
    leaveMenu();
    mFadeoutNeeded = true;
  } else {
    const auto slotIndex = result.mSelectedPage;
    const auto& slot = mContext.mpUserProfile->mSaveSlots[*slotIndex];
    if (slot) {
      if (
        mContext.mpServiceProvider->isShareWareVersion() &&
        slot->mSessionId.needsRegisteredVersion()
      ) {
        showErrorMessageScript("No_Can_Order");
      } else {
        mRequestedGameToLoad = *slot;
      }
    } else {
      showErrorMessageScript("No_Game_Restore");
    }
  }
}


void IngameMenu::onSaveGameMenuFinished(const ExecutionResult& result) {
  using STT = ui::DukeScriptRunner::ScriptTerminationType;

  if (result.mTerminationType == STT::AbortedByUser) {
    leaveMenu();
    mFadeoutNeeded = true;
  } else {
    const auto slotIndex = *result.mSelectedPage;
    SDL_StartTextInput();
    mStateStack.push(SavedGameNameEntry{mContext, slotIndex});
  }
}


void IngameMenu::saveGame(const int slotIndex, std::string_view name) {
  auto savedGame = mSavedGame;
  savedGame.mName = name;

  mContext.mpUserProfile->mSaveSlots[slotIndex] = savedGame;
  mContext.mpUserProfile->saveToDisk();
}


void IngameMenu::handleMenuEnterEvent(const SDL_Event& event) {
  auto leaveMenuHook = [this](const ExecutionResult&) {
    leaveMenu();
  };

  auto leaveMenuWithFadeHook = [this](const ExecutionResult&) {
    leaveMenu();
    mFadeoutNeeded = true;
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
    if (
      (ev.type == SDL_KEYUP && ev.key.keysym.sym == SDLK_y) ||
      (ev.type == SDL_CONTROLLERBUTTONDOWN &&
       ev.cbutton.button == SDL_CONTROLLER_BUTTON_A)
    ) {
      mQuitRequested = true;
      return true;
    }

    return false;
  };


  if (!isNonRepeatKeyDown(event)) {
    return;
  }

  switch (event.key.keysym.sym) {
    case SDLK_ESCAPE:
      enterScriptedMenu(
        "2Quit_Select", leaveMenuHook, quitConfirmEventHook, true);
      break;

    case SDLK_F1:
      mStateStack.push(ui::OptionsMenu{
        mContext.mpUserProfile,
        mContext.mpServiceProvider,
        ui::OptionsMenu::Type::InGame});
      break;

    case SDLK_F2:
      enterScriptedMenu(
        "Save_Game",
        [this](const auto& result) { onSaveGameMenuFinished(result); });
      break;

    case SDLK_F3:
      enterScriptedMenu(
        "Restore_Game",
        [this](const auto& result) { onRestoreGameMenuFinished(result); });
      break;

    case SDLK_h:
      enterScriptedMenu("&Instructions", leaveMenuWithFadeHook);
      break;

    case SDLK_p:
      enterScriptedMenu("Paused", leaveMenuHook, noopEventHook, true);
      break;

    default:
      break;
  }
}


void IngameMenu::handleMenuActiveEvents() {
  auto handleSavedGameNameEntryEvent = [this](
    SavedGameNameEntry& state,
    const SDL_Event& event
  ) {
    auto leaveTextEntry = [&, this]() {
      SDL_StopTextInput();

      // Render one last time so we have something to fade out from
      mContext.mpScriptRunner->updateAndRender(0.0);
      state.updateAndRender(0.0);

      mStateStack.pop();
      mStateStack.pop();
      mFadeoutNeeded = true;
    };

    if (isConfirmButton(event)) {
      saveGame(state.mSlotIndex, state.mTextEntryWidget.text());
      leaveTextEntry();
    } else if (isCancelButton(event)) {
      leaveTextEntry();
    } else {
      state.mTextEntryWidget.handleEvent(event);
    }
  };


  for (const auto& event : mEventQueue) {
    if (mStateStack.empty()) {
      break;
    }

    base::match(mStateStack.top(),
      [&, this](SavedGameNameEntry& state) {
        handleSavedGameNameEntryEvent(state, event);
      },

      [&event](ScriptedMenu& state) {
        state.handleEvent(event);
      },

      [&, this](const ui::OptionsMenu& options) {
        if (isCancelButton(event) || options.isFinished()) {
          mStateStack.pop();
        }
      });
  }

  mEventQueue.clear();
}


template <typename ScriptEndHook, typename EventHook>
void IngameMenu::enterScriptedMenu(
  const char* scriptName,
  ScriptEndHook&& scriptEndedHook,
  EventHook&& eventHook,
  const bool isTransparent,
  const bool shouldClearScriptCanvas
) {
  if (shouldClearScriptCanvas) {
    mContext.mpScriptRunner->clearCanvas();
  }

  runScript(mContext, scriptName);
  mStateStack.push(ScriptedMenu{
    mContext.mpScriptRunner,
    std::forward<ScriptEndHook>(scriptEndedHook),
    std::forward<EventHook>(eventHook),
    isTransparent});
}


void IngameMenu::leaveMenu() {
  mStateStack.pop();
}

}
