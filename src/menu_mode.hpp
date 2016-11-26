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

#include "data/duke_script.hpp"
#include "loader/duke_script_loader.hpp"
#include "ui/duke_script_runner.hpp"

#include "game_mode.hpp"


namespace rigel {

class MenuMode : public GameMode {
public:
  explicit MenuMode(Context context);

  void handleEvent(const SDL_Event& event) override;
  void updateAndRender(engine::TimeDelta dt) override;

private:
  enum class MenuState {
    AskIfQuit,
    ChooseInstructionsOrStory,
    EpisodeNotAvailableMessage,
    GameOptions,
    GameSpeedConfig,
    Instructions,
    JoystickCalibration,
    KeyboardConfig,
    MainMenu,
    OrderingInformation,
    RestoreGame,
    SelectHighscoresEpisode,
    SelectNewGameEpisode,
    SelectNewGameSkill,
    ShowCredits,
    ShowHiscores,
    Story
  };

  void enterMainMenu();
  void navigateToNextMenu(const ui::DukeScriptRunner::ExecutionResult& result);

private:
  ui::DukeScriptRunner mScriptRunner;

  loader::ScriptBundle mMainScripts;
  loader::ScriptBundle mOptionsScripts;
  loader::ScriptBundle mOrderingInfoScripts;

  MenuState mMenuState = MenuState::MainMenu;
  int mChosenEpisodeForNewGame = 0;

  IGameServiceProvider* mpServices;
};

}
