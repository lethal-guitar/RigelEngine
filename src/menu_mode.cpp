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

#include "menu_mode.hpp"

#include "data/game_session_data.hpp"
#include "loader/resource_loader.hpp"

#include "game_service_provider.hpp"


namespace rigel {

using namespace std;

using ui::DukeScriptRunner;


namespace {


const std::array<data::Difficulty, 3> DIFFICULTY_MAPPING{
  data::Difficulty::Easy,
  data::Difficulty::Medium,
  data::Difficulty::Hard
};


bool abortedByUser(const DukeScriptRunner::ExecutionResult& result) {
  return
    result.mTerminationType ==
    DukeScriptRunner::ScriptTerminationType::AbortedByUser;
}

}


MenuMode::MenuMode(Context context)
  : mpScriptRunner(context.mpScriptRunner)
  , mMainScripts(context.mpResources->loadScriptBundle("TEXT.MNI"))
  , mOptionsScripts(context.mpResources->loadScriptBundle("OPTIONS.MNI"))
  , mOrderingInfoScripts(context.mpResources->loadScriptBundle("ORDERTXT.MNI"))
  , mpServices(context.mpServiceProvider)
{
  mpServices->playMusic("DUKEIIA.IMF");
  mpScriptRunner->executeScript(mMainScripts["Main_Menu"]);
}


void MenuMode::handleEvent(const SDL_Event& event) {
  if (
    mMenuState == MenuState::AskIfQuit &&
    event.type == SDL_KEYDOWN &&
    event.key.keysym.sym == SDLK_y
  ) {
    mpServices->scheduleGameQuit();
    return;
  }

  mpScriptRunner->handleEvent(event);
}


void MenuMode::updateAndRender(engine::TimeDelta dt) {
  mpScriptRunner->updateAndRender(dt);

  if (mpScriptRunner->hasFinishedExecution()) {
    const auto result = mpScriptRunner->result();
    assert(result);

    navigateToNextMenu(*result);
  }
}


void MenuMode::enterMainMenu() {
  mChosenEpisodeForNewGame = 0;

  mMenuState = MenuState::MainMenu;
  mpScriptRunner->executeScript(mMainScripts["Main_Menu"]);
}


void MenuMode::navigateToNextMenu(
  const ui::DukeScriptRunner::ExecutionResult& result
) {
  switch (mMenuState) {
    case MenuState::MainMenu:
      if (abortedByUser(result)) {
        mpScriptRunner->executeScript(mMainScripts["Quit_Select"]);
        mMenuState = MenuState::AskIfQuit;
      } else {
        assert(result.mSelectedPage);

        switch (*result.mSelectedPage) {
          case 0:
            mpScriptRunner->executeScript(mMainScripts["Episode_Select"]);
            mMenuState = MenuState::SelectNewGameEpisode;
            break;

          case 1:
            mpScriptRunner->executeScript(mOptionsScripts["Restore_Game"]);
            mMenuState = MenuState::RestoreGame;
            break;

          case 2:
            mpScriptRunner->executeScript(mOptionsScripts["My_Options"]);
            mMenuState = MenuState::GameOptions;
            break;

          case 3:
            if (mpServices->isShareWareVersion()) {
              mpScriptRunner->executeScript(
                mOrderingInfoScripts["Ordering_Info"]);
            } else {
              mpScriptRunner->executeScript(mMainScripts["V4ORDER"]);
            }
            mMenuState = MenuState::OrderingInformation;
            break;

          case 4:
            mpScriptRunner->executeScript(mMainScripts["Both_S_I"]);
            mMenuState = MenuState::ChooseInstructionsOrStory;
            break;

          case 5:
            mpScriptRunner->executeScript(mMainScripts["Episode_Select"]);
            mMenuState = MenuState::SelectHighscoresEpisode;
            break;

          case 6:
            {
              auto showCreditsScript = mMainScripts["&Credits"];
              showCreditsScript.emplace_back(data::script::WaitForUserInput{});

              mpScriptRunner->executeScript(showCreditsScript);
              mMenuState = MenuState::ShowCredits;
            }
            break;

          case 7:
            mpScriptRunner->executeScript(mMainScripts["Quit_Select"]);
            mMenuState = MenuState::AskIfQuit;
            break;

          default:
            enterMainMenu();
            break;
        }
      }
      break;

    case MenuState::SelectNewGameEpisode:
      if (abortedByUser(result)) {
        enterMainMenu();
      } else {
        assert(result.mSelectedPage);
        const auto chosenEpisode = *result.mSelectedPage;

        if (mpServices->isShareWareVersion() && chosenEpisode > 0) {
          mpScriptRunner->executeScript(mMainScripts["No_Can_Order"]);
          mMenuState = MenuState::EpisodeNotAvailableMessage;
        } else {
          mChosenEpisodeForNewGame = chosenEpisode;

          mpScriptRunner->executeScript(mMainScripts["Skill_Select"]);
          mMenuState = MenuState::SelectNewGameSkill;
        }
      }
      break;

    case MenuState::SelectNewGameSkill:
      if (abortedByUser(result)) {
        mpScriptRunner->executeScript(mMainScripts["Episode_Select"]);
        mMenuState = MenuState::SelectNewGameEpisode;
      } else {
        assert(result.mSelectedPage);
        const auto chosenSkill = *result.mSelectedPage;
        if (chosenSkill < 0 || chosenSkill >= 3) {
          throw std::invalid_argument("Invalid skill index");
        }

        mpServices->scheduleNewGameStart(
          mChosenEpisodeForNewGame,
          DIFFICULTY_MAPPING[chosenSkill]);
        return;
      }
      break;

    case MenuState::EpisodeNotAvailableMessage:
      mpScriptRunner->executeScript(mMainScripts["Episode_Select"]);
      mMenuState = MenuState::SelectNewGameEpisode;
      break;

    case MenuState::GameOptions:
      if (abortedByUser(result)) {
        enterMainMenu();
      } else {
        assert(result.mSelectedPage);
        switch (*result.mSelectedPage) {
          case 4:
            mpScriptRunner->executeScript(mOptionsScripts["Key_Config"]);
            mMenuState = MenuState::KeyboardConfig;
            break;

          case 5:
            mpScriptRunner->executeScript(mOptionsScripts["&Calibrate"]);
            mMenuState = MenuState::JoystickCalibration;
            break;

          case 6:
            mpScriptRunner->executeScript(mOptionsScripts["Game_Speed"]);
            mMenuState = MenuState::GameSpeedConfig;
            break;
        }
      }
      break;

    case MenuState::KeyboardConfig:
    case MenuState::JoystickCalibration:
    case MenuState::GameSpeedConfig:
      mpScriptRunner->executeScript(mOptionsScripts["My_Options"]);
      mMenuState = MenuState::GameOptions;
      break;

    case MenuState::ChooseInstructionsOrStory:
      if (abortedByUser(result)) {
        enterMainMenu();
      } else {
        assert(result.mSelectedPage);
        switch (*result.mSelectedPage) {
          case 0:
            mpScriptRunner->executeScript(mMainScripts["&Instructions"]);
            mMenuState = MenuState::Instructions;
            break;

          case 1:
            mpScriptRunner->executeScript(mMainScripts["&Story"]);
            mMenuState = MenuState::Story;
            break;
        }
      }
      break;

    case MenuState::SelectHighscoresEpisode:
      if (abortedByUser(result)) {
        enterMainMenu();
      } else {
        assert(result.mSelectedPage);
        const auto chosenEpisode = *result.mSelectedPage;
        const auto hiscoreBackgroundScript =
          std::string("Volume") + std::to_string(chosenEpisode + 1);

        auto showHiScoreScript = mMainScripts[hiscoreBackgroundScript];
        showHiScoreScript.emplace_back(data::script::FadeIn{});
        showHiScoreScript.emplace_back(data::script::WaitForUserInput{});

        mpScriptRunner->executeScript(showHiScoreScript);
        mMenuState = MenuState::ShowHiscores;
      }
      break;

    default:
      enterMainMenu();
      break;
  }
}

}
