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

#pragma once

#include "base/spatial_types.hpp"
#include "base/warnings.hpp"
#include "common/game_mode.hpp"
#include "data/bonus.hpp"
#include "data/saved_game.hpp"
#include "game_logic/game_world.hpp"
#include "game_logic/input.hpp"
#include "ui/duke_script_runner.hpp"
#include "ui/menu_navigation_helper.hpp"
#include "ui/options_menu.hpp"
#include "ui/text_entry_widget.hpp"

RIGEL_DISABLE_WARNINGS
#include <SDL.h>
RIGEL_RESTORE_WARNINGS

#include <functional>
#include <optional>
#include <stack>
#include <variant>
#include <vector>


namespace rigel::ui {

class IngameMenu {
public:
  enum class UpdateResult {
    StillActive,
    Finished,
    FinishedNeedsFadeout
  };

  IngameMenu(
    GameMode::Context context,
    const data::PlayerModel* pPlayerModel,
    const data::GameSessionId& sessionId);

  void handleEvent(const SDL_Event& event);
  UpdateResult updateAndRender(engine::TimeDelta dt);

  bool quitRequested() const {
    return mQuitRequested;
  }

  std::optional<data::SavedGame> requestedGameToLoad() const {
    return mRequestedGameToLoad;
  }

  bool isActive() const {
    return !mStateStack.empty() || mMenuToEnter;
  }

private:
  using ExecutionResult = ui::DukeScriptRunner::ExecutionResult;

  struct TopLevelMenu {
    explicit TopLevelMenu(GameMode::Context context);

    TopLevelMenu(const TopLevelMenu&) = delete;
    TopLevelMenu& operator=(const TopLevelMenu&) = delete;

    void handleEvent(const SDL_Event& event);
    void updateAndRender(engine::TimeDelta dt);

    GameMode::Context mContext;
    loader::Palette16 mPalette;
    engine::TiledTexture mUiSpriteSheet;
    MenuElementRenderer mMenuElementRenderer;
    renderer::OwningTexture mMenuBackground;
    MenuNavigationHelper mNavigationHelper;
    engine::TimeDelta mElapsedTime = 0;
    int mSelectedIndex = 0;
  };

  struct ScriptedMenu {
    template <typename ScriptEndHook, typename EventHook>
    ScriptedMenu(
      ui::DukeScriptRunner* pScriptRunner,
      ScriptEndHook&& scriptEndHook,
      EventHook&& eventHook
    )
      : mScriptFinishedHook(std::forward<ScriptEndHook>(scriptEndHook))
      , mEventHook(std::forward<EventHook>(eventHook))
      , mpScriptRunner(pScriptRunner)
    {
    }

    void handleEvent(const SDL_Event& event);
    void updateAndRender(engine::TimeDelta dt);

    std::function<void(const ExecutionResult&)> mScriptFinishedHook;
    std::function<bool(const SDL_Event&)> mEventHook;
    ui::DukeScriptRunner* mpScriptRunner;
  };

  struct SavedGameNameEntry {
    SavedGameNameEntry(GameMode::Context context, const int slotIndex);

    void updateAndRender(engine::TimeDelta dt) {
      mTextEntryWidget.updateAndRender(dt);
    }

    ui::TextEntryWidget mTextEntryWidget;
    int mSlotIndex;
  };

  using State = std::variant<
    std::unique_ptr<TopLevelMenu>,
    ScriptedMenu,
    SavedGameNameEntry,
    ui::OptionsMenu>;

  enum class MenuType {
    TopLevel,
    ConfirmQuitInGame,
    ConfirmQuit,
    Options,
    SaveGame,
    LoadGame,
    Help,
    Pause
  };

  static bool noopEventHook(const SDL_Event&) { return false; }

  template <typename ScriptEndHook, typename EventHook = decltype(noopEventHook)>
  void enterScriptedMenu(
    const char* scriptName,
    ScriptEndHook&& scriptEndedHook,
    EventHook&& eventHook = noopEventHook,
    bool shouldClearScriptCanvas = true);
  void enterMenu(MenuType type);
  void leaveMenu();
  void fadeout();

  void onRestoreGameMenuFinished(const ExecutionResult& result);
  void onSaveGameMenuFinished(const ExecutionResult& result);
  void saveGame(int slotIndex, std::string_view name);
  void handleMenuEnterEvent(const SDL_Event& event);
  void handleMenuActiveEvents();

  GameMode::Context mContext;
  data::SavedGame mSavedGame;
  std::optional<data::SavedGame> mRequestedGameToLoad;
  std::stack<State, std::vector<State>> mStateStack;
  std::vector<SDL_Event> mEventQueue;
  std::optional<MenuType> mMenuToEnter;
  TopLevelMenu* mpTopLevelMenu = nullptr;
  bool mQuitRequested = false;
  bool mFadeoutNeeded = false;
};

}
