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

#include "base/spatial_types.hpp"
#include "base/warnings.hpp"
#include "common/game_mode.hpp"
#include "data/bonus.hpp"
#include "data/saved_game.hpp"
#include "game_logic/game_world.hpp"
#include "game_logic/input.hpp"
#include "loader/duke_script_loader.hpp"
#include "ui/duke_script_runner.hpp"
#include "ui/options_menu.hpp"
#include "ui/text_entry_widget.hpp"

RIGEL_DISABLE_WARNINGS
#include <SDL.h>
RIGEL_RESTORE_WARNINGS

#include <functional>
#include <stack>
#include <variant>


namespace rigel {

class GameRunner : public entityx::Receiver<GameRunner> {
public:
  GameRunner(
    data::PlayerModel* pPlayerModel,
    const data::GameSessionId& sessionId,
    GameMode::Context context,
    std::optional<base::Vector> playerPositionOverride = std::nullopt,
    bool showWelcomeMessage = false);

  void handleEvent(const SDL_Event& event);
  void updateAndRender(engine::TimeDelta dt);

  bool levelFinished() const;
  bool gameQuit() const;
  std::set<data::Bonus> achievedBonuses() const;

private:
  using ExecutionResult = ui::DukeScriptRunner::ExecutionResult;

  struct World {
    explicit World(game_logic::GameWorld* pWorld)
      : mpWorld(pWorld)
    {
    }

    void handleEvent(const SDL_Event& event);
    void updateAndRender(engine::TimeDelta dt);

    void updateWorld(engine::TimeDelta dt);
    void handlePlayerInput(const SDL_Event& event);
    void handleDebugKeys(const SDL_Event& event);

    game_logic::GameWorld* mpWorld;
    game_logic::PlayerInput mPlayerInput;
    engine::TimeDelta mAccumulatedTime = 0.0;
    bool mShowDebugText = false;
    bool mSingleStepping = false;
    bool mDoNextSingleStep = false;
  };

  struct Menu {
    template <typename ScriptEndHook, typename EventHook>
    Menu(
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

    void updateAndRender(engine::TimeDelta dt);

    ui::TextEntryWidget mTextEntryWidget;
    int mSlotIndex;
  };

  using State = std::variant<World, Menu, SavedGameNameEntry, ui::OptionsMenu>;

  static bool noopEventHook(const SDL_Event&) { return false; }

  bool handleMenuEnterEvent(const SDL_Event& event);

  template <typename ScriptEndHook, typename EventHook = decltype(noopEventHook)>
  void enterMenu(
    const char* scriptName,
    ScriptEndHook&& scriptEndedHook,
    EventHook&& eventHook = noopEventHook);
  void leaveMenu();
  void fadeToWorld();

  void onRestoreGameMenuFinished(const ExecutionResult& result);
  void onSaveGameMenuFinished(const ExecutionResult& result);
  void saveGame(int slotIndex, std::string_view name);

  GameMode::Context mContext;
  data::SavedGame mSavedGame;
  game_logic::GameWorld mWorld;
  std::stack<State, std::vector<State>> mStateStack;
  bool mGameWasQuit = false;
};


inline bool GameRunner::levelFinished() const {
  return mWorld.levelFinished();
}


inline bool GameRunner::gameQuit() const {
  return mGameWasQuit;
}


inline std::set<data::Bonus> GameRunner::achievedBonuses() const {
  return mWorld.achievedBonuses();
}


inline void GameRunner::Menu::handleEvent(const SDL_Event& event) {
  if (!mEventHook(event)) {
    mpScriptRunner->handleEvent(event);
  }
}


inline void GameRunner::Menu::updateAndRender(const engine::TimeDelta dt) {
  mpScriptRunner->updateAndRender(dt);

  if (mpScriptRunner->hasFinishedExecution()) {
    mScriptFinishedHook(*mpScriptRunner->result());
  }
}


inline void GameRunner::SavedGameNameEntry::updateAndRender(
  const engine::TimeDelta dt
) {
  mTextEntryWidget.updateAndRender(dt);
}

}
