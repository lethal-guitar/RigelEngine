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
#include "ui/utils.hpp"

#include <sstream>


namespace rigel::ui {

namespace {

constexpr auto MENU_START_POS_X = 11;
constexpr auto MENU_START_POS_Y = 6;
constexpr auto MENU_ITEM_HEIGHT = 2;
constexpr auto MENU_SELECTION_INDICATOR_POS_X = 8;
constexpr auto MENU_ITEM_COLOR = 2;
constexpr auto MENU_ITEM_COLOR_SELECTED = 3;

constexpr auto SAVE_SLOT_NAME_ENTRY_POS_X = 14;
constexpr auto SAVE_SLOT_NAME_ENTRY_START_POS_Y = MENU_START_POS_Y;
constexpr auto SAVE_SLOT_NAME_HEIGHT = MENU_ITEM_HEIGHT;
constexpr auto MAX_SAVE_SLOT_NAME_LENGTH = 18;

constexpr auto TOP_LEVEL_MENU_ITEMS = std::array{
  "Save Game",
  "Restore Game",
  "Options",
  "Help",
  "Quit Game"
};

constexpr int itemIndex(const std::string_view item) {
  int result = 0;
  for (const auto candidate : TOP_LEVEL_MENU_ITEMS) {
    if (item == candidate) {
      return result;
    }

    ++result;
  }

  return -1;
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


std::string makePrefillName(const data::SavedGame& savedGame) {
  std::stringstream stream;
  stream
    << "Ep " << savedGame.mSessionId.mEpisode + 1 << ", "
    << "Lv " << savedGame.mSessionId.mLevel + 1 << ", ";

  switch (savedGame.mSessionId.mDifficulty) {
    case data::Difficulty::Easy: stream << "Easy"; break;
    case data::Difficulty::Medium: stream << "Medium"; break;
    case data::Difficulty::Hard: stream << "Hard"; break;
  }

  return stream.str();
}

}


IngameMenu::TopLevelMenu::TopLevelMenu(GameMode::Context context)
  : mContext(context)
  , mPalette(context.mpResources->loadPaletteFromFullScreenImage("MESSAGE.MNI"))
  , mUiSpriteSheet(makeUiSpriteSheet(
    context.mpRenderer, *context.mpResources, mPalette))
  , mMenuElementRenderer(
      &mUiSpriteSheet, context.mpRenderer, *context.mpResources)
  , mMenuBackground(fullScreenImageAsTexture(
    context.mpRenderer, *context.mpResources, "MESSAGE.MNI"))
{
}


void IngameMenu::TopLevelMenu::handleEvent(const SDL_Event& event) {
  auto selectNext = [this]() {
    ++mSelectedIndex;
    if (mSelectedIndex >= int(TOP_LEVEL_MENU_ITEMS.size())) {
      mSelectedIndex = 0;
    }

    mContext.mpServiceProvider->playSound(data::SoundId::MenuSelect);
  };

  auto selectPrevious = [this]() {
    --mSelectedIndex;
    if (mSelectedIndex < 0) {
      mSelectedIndex = int(TOP_LEVEL_MENU_ITEMS.size()) - 1;
    }

    mContext.mpServiceProvider->playSound(data::SoundId::MenuSelect);
  };


  switch (mNavigationHelper.convert(event)) {
    case NavigationEvent::NavigateUp:
      selectPrevious();
      break;

    case NavigationEvent::NavigateDown:
      selectNext();
      break;

    default:
      break;
  }
}


void IngameMenu::TopLevelMenu::updateAndRender(const engine::TimeDelta dt) {
  mContext.mpRenderer->clear();
  mMenuBackground.render(mContext.mpRenderer, 0, 0);

  auto index = 0;
  for (const auto item : TOP_LEVEL_MENU_ITEMS) {
    const auto colorIndex = index == mSelectedIndex
      ? MENU_ITEM_COLOR_SELECTED
      : MENU_ITEM_COLOR;
    mMenuElementRenderer.drawBigText(
      MENU_START_POS_X,
      MENU_START_POS_Y + index * MENU_ITEM_HEIGHT,
      item,
      mPalette[colorIndex]);
    ++index;
  }

  mElapsedTime += dt;
  mMenuElementRenderer.drawSelectionIndicator(
    MENU_SELECTION_INDICATOR_POS_X,
    MENU_START_POS_Y + mSelectedIndex * MENU_ITEM_HEIGHT,
    mElapsedTime);
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
  const int slotIndex,
  const std::string_view initialName
)
  : mTextEntryWidget(
      context.mpUiRenderer,
      SAVE_SLOT_NAME_ENTRY_POS_X,
      SAVE_SLOT_NAME_ENTRY_START_POS_Y + slotIndex * SAVE_SLOT_NAME_HEIGHT,
      MAX_SAVE_SLOT_NAME_LENGTH,
      ui::TextEntryWidget::Style::BigText,
      initialName)
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


bool IngameMenu::isTransparent() const {
  if (mStateStack.empty()) {
    return true;
  }

  if (mpTopLevelMenu) {
    return false;
  }

  return base::match(mStateStack.top(),
    [](const ScriptedMenu& state) { return state.mIsTransparent; },
    [](const ui::OptionsMenu&) { return true; },
    [](const auto&) { return false; });
}


void IngameMenu::handleEvent(const SDL_Event& event) {
  if (mQuitRequested || mRequestedGameToLoad) {
    return;
  }

  if (!isActive()) {
    handleMenuEnterEvent(event);
  } else {
    // We want to process menu navigation and similar events in updateAndRender,
    // so we only add them to a queue here.
    mEventQueue.push_back(event);
  }
}


auto IngameMenu::updateAndRender(engine::TimeDelta dt) -> UpdateResult {
  if (mMenuToEnter) {
    enterMenu(*mMenuToEnter);
    mMenuToEnter.reset();
  }

  mFadeoutNeeded = false;

  handleMenuActiveEvents();

  if (mpTopLevelMenu && mStateStack.size() > 1) {
    mpTopLevelMenu->updateAndRender(0.0);
  }

  if (!mStateStack.empty()) {
    base::match(mStateStack.top(),
      [dt, this](SavedGameNameEntry& state) {
        mContext.mpScriptRunner->updateAndRender(dt);
        state.updateAndRender(dt);
      },

      [dt](std::unique_ptr<TopLevelMenu>& pState) {
        pState->updateAndRender(dt);
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
    fadeout();
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


void IngameMenu::saveGame(const int slotIndex, std::string_view name) {
  auto savedGame = mSavedGame;
  savedGame.mName = name;

  mContext.mpUserProfile->mSaveSlots[slotIndex] = savedGame;
  mContext.mpUserProfile->saveToDisk();
}


void IngameMenu::handleMenuEnterEvent(const SDL_Event& event) {
  if (
    event.type == SDL_CONTROLLERBUTTONDOWN &&
    event.cbutton.button == SDL_CONTROLLER_BUTTON_START
  ) {
    mMenuToEnter = MenuType::TopLevel;
    return;
  }

  if (!isNonRepeatKeyDown(event)) {
    return;
  }

  switch (event.key.keysym.sym) {
    case SDLK_ESCAPE:
      mMenuToEnter = MenuType::ConfirmQuitInGame;
      break;

    case SDLK_F1:
      mMenuToEnter = MenuType::Options;
      break;

    case SDLK_F2:
      mMenuToEnter = MenuType::SaveGame;
      break;

    case SDLK_F3:
      mMenuToEnter = MenuType::LoadGame;
      break;

    case SDLK_h:
      mMenuToEnter = MenuType::Help;
      break;

    case SDLK_p:
      mMenuToEnter = MenuType::Pause;
      break;

    default:
      break;
  }
}


void IngameMenu::enterMenu(const MenuType type) {
  auto leaveMenuHook = [this](const ExecutionResult&) {
    leaveMenu();
  };

  auto leaveMenuWithFadeHook = [this](const ExecutionResult&) {
    leaveMenu();
    fadeout();
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

  auto saveSlotSelectionEventHook = [this](const SDL_Event& event) {
    if (isConfirmButton(event)) {
      const auto enteredViaGamepad =
        event.type == SDL_CONTROLLERBUTTONDOWN;

      const auto slotIndex = *mContext.mpScriptRunner->currentPageIndex();
      SDL_StartTextInput();
      mStateStack.push(SavedGameNameEntry{
        mContext,
        slotIndex,
        enteredViaGamepad ? makePrefillName(mSavedGame) : ""});
      return true;
    }

    return false;
  };

  auto onSaveSlotSelectionFinished = [this](const ExecutionResult& result) {
    using STT = ui::DukeScriptRunner::ScriptTerminationType;
    if (result.mTerminationType == STT::AbortedByUser) {
      leaveMenu();
      fadeout();
    }
  };


  switch (type) {
    case MenuType::ConfirmQuitInGame:
      enterScriptedMenu(
        "2Quit_Select", leaveMenuHook, quitConfirmEventHook, true);
      break;

    case MenuType::ConfirmQuit:
      enterScriptedMenu("Quit_Select", leaveMenuHook, quitConfirmEventHook);
      break;

    case MenuType::Options:
      mStateStack.push(ui::OptionsMenu{
        mContext.mpUserProfile,
        mContext.mpServiceProvider,
        ui::OptionsMenu::Type::InGame});
      break;

    case MenuType::SaveGame:
      enterScriptedMenu(
        "Save_Game", onSaveSlotSelectionFinished, saveSlotSelectionEventHook);
      break;

    case MenuType::LoadGame:
      enterScriptedMenu(
        "Restore_Game",
        [this](const auto& result) { onRestoreGameMenuFinished(result); });
      break;

    case MenuType::Help:
      enterScriptedMenu("&Instructions", leaveMenuWithFadeHook);
      break;

    case MenuType::Pause:
      enterScriptedMenu("Paused", leaveMenuHook, noopEventHook, true);
      break;

    case MenuType::TopLevel:
      {
        auto pMenu = std::make_unique<TopLevelMenu>(mContext);
        mContext.mpServiceProvider->fadeOutScreen();
        pMenu->updateAndRender(0.0);
        mContext.mpServiceProvider->fadeInScreen();

        mpTopLevelMenu = pMenu.get();
        mStateStack.push(std::move(pMenu));
      }
      break;
  }
}


void IngameMenu::handleMenuActiveEvents() {
  auto handleSavedGameNameEntryEvent = [this](
    SavedGameNameEntry& state,
    const SDL_Event& event
  ) {
    auto leaveSaveGameMenu = [&, this]() {
      SDL_StopTextInput();

      // Render one last time so we have something to fade out from
      mContext.mpScriptRunner->updateAndRender(0.0);
      state.updateAndRender(0.0);

      mStateStack.pop();
      mStateStack.pop();
    };

    if (isConfirmButton(event)) {
      saveGame(state.mSlotIndex, state.mTextEntryWidget.text());
      leaveSaveGameMenu();
      if (mpTopLevelMenu) {
        mpTopLevelMenu = nullptr;
        mStateStack.pop();
      }

      fadeout();
    } else if (isCancelButton(event)) {
      SDL_StopTextInput();
      mStateStack.pop();
    } else {
      state.mTextEntryWidget.handleEvent(event);
    }
  };


  for (const auto& event : mEventQueue) {
    if (mStateStack.empty()) {
      break;
    }

    base::match(mStateStack.top(),
      [&](std::unique_ptr<TopLevelMenu>& pState) {
        auto& state = *pState;
        if (isConfirmButton(event)) {
          switch (state.mSelectedIndex) {
            case itemIndex("Save Game"):
              enterMenu(MenuType::SaveGame);
              break;

            case itemIndex("Restore Game"):
              enterMenu(MenuType::LoadGame);
              break;

            case itemIndex("Options"):
              enterMenu(MenuType::Options);
              break;

            case itemIndex("Help"):
              enterMenu(MenuType::Help);
              break;

            case itemIndex("Quit Game"):
              enterMenu(MenuType::ConfirmQuit);
              break;
          }
        } else if (isCancelButton(event)) {
          state.updateAndRender(0.0);
          mpTopLevelMenu = nullptr;
          mStateStack.pop();
          fadeout();
        } else {
          state.handleEvent(event);
        }
      },

      [&, this](SavedGameNameEntry& state) {
        handleSavedGameNameEntryEvent(state, event);
      },

      [&event](ScriptedMenu& state) {
        state.handleEvent(event);
      },

      [&](const ui::OptionsMenu&) {
        // handled by Dear ImGui
      });
  }

  mEventQueue.clear();

  // Handle options menu being closed
  if (
    !mStateStack.empty() &&
    std::holds_alternative<ui::OptionsMenu>(mStateStack.top()) &&
    std::get<ui::OptionsMenu>(mStateStack.top()).isFinished()
  ) {
    mStateStack.pop();
  }
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


void IngameMenu::fadeout() {
  if (mpTopLevelMenu) {
    mContext.mpServiceProvider->fadeOutScreen();
    mpTopLevelMenu->updateAndRender(0.0);
    mContext.mpServiceProvider->fadeInScreen();
  } else {
    mFadeoutNeeded = true;
  }
}

}
