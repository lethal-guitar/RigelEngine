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

#include "common/game_service_provider.hpp"
#include "common/user_profile.hpp"
#include "data/game_session_data.hpp"
#include "loader/resource_loader.hpp"
#include "ui/high_score_list.hpp"


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
  : mContext(context)
{
  mContext.mpServiceProvider->playMusic("DUKEIIA.IMF");
  runScript(mContext,"Main_Menu");
}


void MenuMode::handleEvent(const SDL_Event& event) {
  if (mOptionsMenu) {
    if (
      event.type == SDL_KEYDOWN &&
      event.key.keysym.sym == SDLK_ESCAPE
    ) {
      mOptionsMenu = std::nullopt;
    }
    return;
  }

  if (
    mMenuState == MenuState::AskIfQuit &&
    event.type == SDL_KEYDOWN &&
    event.key.keysym.sym == SDLK_y
  ) {
    mContext.mpServiceProvider->scheduleGameQuit();
    return;
  }

  if (mMenuState == MenuState::MainMenu) {
    const auto maybeIndex = mContext.mpScriptRunner->currentPageIndex();
    const auto optionsMenuSelected = maybeIndex && *maybeIndex == 2;
    if (
      optionsMenuSelected &&
      event.type == SDL_KEYDOWN &&
      (event.key.keysym.sym == SDLK_RETURN ||
       event.key.keysym.sym == SDLK_KP_ENTER ||
       event.key.keysym.sym == SDLK_SPACE)
    ) {
      mOptionsMenu = ui::OptionsMenu{&mContext.mpUserProfile->mOptions};
      return;
    }
  }

  mContext.mpScriptRunner->handleEvent(event);
}


void MenuMode::updateAndRender(engine::TimeDelta dt) {
  if (mOptionsMenu) {
    mOptionsMenu->updateAndRender(dt);
    if (mOptionsMenu->isFinished()) {
      mOptionsMenu = std::nullopt;
    } else {
      return;
    }
  }

  mContext.mpScriptRunner->updateAndRender(dt);

  if (mContext.mpScriptRunner->hasFinishedExecution()) {
    const auto result = mContext.mpScriptRunner->result();
    assert(result);

    navigateToNextMenu(*result);
  }
}


void MenuMode::enterMainMenu() {
  mChosenEpisodeForNewGame = 0;

  mMenuState = MenuState::MainMenu;
  runScript(mContext, "Main_Menu");
}


void MenuMode::navigateToNextMenu(
  const ui::DukeScriptRunner::ExecutionResult& result
) {
  switch (mMenuState) {
    case MenuState::MainMenu:
      if (abortedByUser(result)) {
        runScript(mContext, "Quit_Select");
        mMenuState = MenuState::AskIfQuit;
      } else {
        assert(result.mSelectedPage);

        switch (*result.mSelectedPage) {
          case 0:
            runScript(mContext, "Episode_Select");
            mMenuState = MenuState::SelectNewGameEpisode;
            break;

          case 1:
            runScript(mContext, "Restore_Game");
            mMenuState = MenuState::RestoreGame;
            break;

          case 3:
            if (mContext.mpServiceProvider->isShareWareVersion()) {
              runScript(mContext, "Ordering_Info");
            } else {
              runScript(mContext, "V4ORDER");
            }
            mMenuState = MenuState::OrderingInformation;
            break;

          case 4:
            runScript(mContext, "Both_S_I");
            mMenuState = MenuState::ChooseInstructionsOrStory;
            break;

          case 5:
            runScript(mContext, "Episode_Select");
            mMenuState = MenuState::SelectHighscoresEpisode;
            break;

          case 6:
            {
              auto showCreditsScript = mContext.mpScripts->at("&Credits");
              showCreditsScript.emplace_back(data::script::WaitForUserInput{});

              mContext.mpScriptRunner->executeScript(showCreditsScript);
              mMenuState = MenuState::ShowCredits;
            }
            break;

          case 7:
            runScript(mContext, "Quit_Select");
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

        if (mContext.mpServiceProvider->isShareWareVersion() && chosenEpisode > 0) {
          runScript(mContext, "No_Can_Order");
          mMenuState = MenuState::EpisodeNotAvailableMessage;
        } else {
          mChosenEpisodeForNewGame = chosenEpisode;

          runScript(mContext, "Skill_Select");
          mMenuState = MenuState::SelectNewGameSkill;
        }
      }
      break;

    case MenuState::SelectNewGameSkill:
      if (abortedByUser(result)) {
        runScript(mContext, "Episode_Select");
        mMenuState = MenuState::SelectNewGameEpisode;
      } else {
        assert(result.mSelectedPage);
        const auto chosenSkill = *result.mSelectedPage;
        if (chosenSkill < 0 || chosenSkill >= 3) {
          throw std::invalid_argument("Invalid skill index");
        }

        mContext.mpServiceProvider->scheduleNewGameStart(
          mChosenEpisodeForNewGame,
          DIFFICULTY_MAPPING[chosenSkill]);
        return;
      }
      break;

    case MenuState::EpisodeNotAvailableMessage:
      runScript(mContext, "Episode_Select");
      mMenuState = MenuState::SelectNewGameEpisode;
      break;

    case MenuState::EpisodeNotAvailableMessageHighScores:
      runScript(mContext, "Episode_Select");
      mMenuState = MenuState::SelectHighscoresEpisode;
      break;

    case MenuState::RestoreGame:
      if (abortedByUser(result)) {
        enterMainMenu();
      } else {
        assert(result.mSelectedPage);
        const auto slotIndex = *result.mSelectedPage;
        const auto& slot = mContext.mpUserProfile->mSaveSlots[slotIndex];
        if (slot) {
          mContext.mpServiceProvider->scheduleStartFromSavedGame(*slot);
        } else {
          runScript(mContext, "No_Game_Restore");
          mMenuState = MenuState::NoSavedGameInSlotMessage;
        }
      }
      break;

    case MenuState::NoSavedGameInSlotMessage:
      runScript(mContext, "Restore_Game");
      mMenuState = MenuState::RestoreGame;
      break;

    case MenuState::ChooseInstructionsOrStory:
      if (abortedByUser(result)) {
        enterMainMenu();
      } else {
        assert(result.mSelectedPage);
        switch (*result.mSelectedPage) {
          case 0:
            runScript(mContext, "&Instructions");
            mMenuState = MenuState::Instructions;
            break;

          case 1:
            runScript(mContext, "&Story");
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

        if (mContext.mpServiceProvider->isShareWareVersion() && chosenEpisode > 0) {
          runScript(mContext, "No_Can_Order");
          mMenuState = MenuState::EpisodeNotAvailableMessageHighScores;
        } else {
          ui::setupHighScoreListDisplay(mContext, chosenEpisode);
          mMenuState = MenuState::ShowHiscores;
        }
      }
      break;

    default:
      enterMainMenu();
      break;
  }
}

}
