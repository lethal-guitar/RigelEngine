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

#include "game_session_mode.hpp"
#include "intro_demo_loop_mode.hpp"

#include "assets/resource_loader.hpp"
#include "data/game_session_data.hpp"
#include "frontend/game_service_provider.hpp"
#include "frontend/user_profile.hpp"
#include "ui/high_score_list.hpp"
#include "ui/menu_navigation.hpp"


namespace rigel
{

using namespace std;

using ui::DukeScriptRunner;


namespace
{


const std::array<data::Difficulty, 3> DIFFICULTY_MAPPING{
  data::Difficulty::Easy,
  data::Difficulty::Medium,
  data::Difficulty::Hard};


bool abortedByUser(const DukeScriptRunner::ExecutionResult& result)
{
  return result.mTerminationType ==
    DukeScriptRunner::ScriptTerminationType::AbortedByUser;
}

} // namespace


MenuMode::MenuMode(Context context)
  : mContext(context)
{
  mContext.mpServiceProvider->playMusic("DUKEIIA.IMF");
  runScript(mContext, "Main_Menu");
}


void MenuMode::handleEvent(const SDL_Event& event)
{
  if (mOptionsMenu)
  {
    mOptionsMenu->handleEvent(event);

    // Options menu blocks all input
    return;
  }

  if (mMenuState == MenuState::AskIfQuit && ui::isQuitConfirmButton(event))
  {
    mContext.mpServiceProvider->scheduleGameQuit();
    return;
  }

  if (mMenuState == MenuState::MainMenu)
  {
    const auto maybeIndex = mContext.mpScriptRunner->currentPageIndex();
    const auto optionsMenuSelected = maybeIndex && *maybeIndex == 2;
    if (
      optionsMenuSelected &&
      (ui::isConfirmButton(event) ||
       (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE)))
    {
      mOptionsMenu = ui::OptionsMenu{
        mContext.mpUserProfile,
        mContext.mpServiceProvider,
        mContext.mpRenderer,
        ui::OptionsMenu::Type::Main};
      return;
    }
  }

  mContext.mpScriptRunner->handleEvent(event);
}


std::unique_ptr<GameMode> MenuMode::updateAndRender(
  const engine::TimeDelta dt,
  const std::vector<SDL_Event>& events)
{
  for (const auto& event : events)
  {
    handleEvent(event);
  }

  if (mOptionsMenu)
  {
    mContext.mpScriptRunner->updateAndRender(0.0);
    mOptionsMenu->updateAndRender(dt);

    if (mOptionsMenu->isFinished())
    {
      mOptionsMenu = std::nullopt;
    }
    else
    {
      return {};
    }
  }

  mContext.mpScriptRunner->updateAndRender(dt);

  if (mMenuState == MenuState::ShowHiscores)
  {
    ui::drawHighScoreList(mContext, mChosenEpisode);
  }

  if (mContext.mpScriptRunner->hasFinishedExecution())
  {
    const auto result = mContext.mpScriptRunner->result();
    assert(result);

    using STT = ui::DukeScriptRunner::ScriptTerminationType;
    if (result->mTerminationType == STT::TimedOut)
    {
      return std::make_unique<IntroDemoLoopMode>(
        mContext, IntroDemoLoopMode::Type::Regular);
    }

    return navigateToNextMenu(*result);
  }

  return {};
}


void MenuMode::enterMainMenu()
{
  mChosenEpisode = 0;

  mMenuState = MenuState::MainMenu;
  runScript(mContext, "Main_Menu");
}


std::unique_ptr<GameMode> MenuMode::navigateToNextMenu(
  const ui::DukeScriptRunner::ExecutionResult& result)
{
  switch (mMenuState)
  {
    case MenuState::MainMenu:
      if (abortedByUser(result))
      {
        runScript(mContext, "Quit_Select");
        mMenuState = MenuState::AskIfQuit;
      }
      else
      {
        assert(result.mSelectedPage);

        switch (*result.mSelectedPage)
        {
          case 0:
            runScript(mContext, "Episode_Select");
            mMenuState = MenuState::SelectNewGameEpisode;
            break;

          case 1:
            runScript(mContext, "Restore_Game");
            mMenuState = MenuState::RestoreGame;
            break;

          case 3:
            if (mContext.mpServiceProvider->isSharewareVersion())
            {
              runScript(mContext, "Ordering_Info");
            }
            else
            {
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
      if (abortedByUser(result))
      {
        enterMainMenu();
      }
      else
      {
        assert(result.mSelectedPage);
        const auto chosenEpisode = *result.mSelectedPage;

        if (
          mContext.mpServiceProvider->isSharewareVersion() && chosenEpisode > 0)
        {
          runScript(mContext, "No_Can_Order");
          mMenuState = MenuState::EpisodeNotAvailableMessage;
        }
        else
        {
          mChosenEpisode = chosenEpisode;

          runScript(mContext, "Skill_Select");
          mMenuState = MenuState::SelectNewGameSkill;
        }
      }
      break;

    case MenuState::SelectNewGameSkill:
      if (abortedByUser(result))
      {
        runScript(mContext, "Episode_Select");
        mMenuState = MenuState::SelectNewGameEpisode;
      }
      else
      {
        assert(result.mSelectedPage);
        const auto chosenSkill = *result.mSelectedPage;
        if (chosenSkill < 0 || chosenSkill >= 3)
        {
          throw std::invalid_argument("Invalid skill index");
        }

        return std::make_unique<GameSessionMode>(
          data::GameSessionId{
            mChosenEpisode, 0, DIFFICULTY_MAPPING[chosenSkill]},
          mContext);
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
      if (abortedByUser(result))
      {
        enterMainMenu();
      }
      else
      {
        assert(result.mSelectedPage);
        const auto slotIndex = *result.mSelectedPage;
        const auto& slot = mContext.mpUserProfile->mSaveSlots[slotIndex];
        if (slot)
        {
          if (
            mContext.mpServiceProvider->isSharewareVersion() &&
            slot->mSessionId.needsRegisteredVersion())
          {
            runScript(mContext, "No_Can_Order");
            mMenuState = MenuState::NoSavedGameInSlotMessage;
          }
          else
          {
            return std::make_unique<GameSessionMode>(*slot, mContext);
          }
        }
        else
        {
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
      if (abortedByUser(result))
      {
        enterMainMenu();
      }
      else
      {
        assert(result.mSelectedPage);
        switch (*result.mSelectedPage)
        {
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
      if (abortedByUser(result))
      {
        enterMainMenu();
      }
      else
      {
        assert(result.mSelectedPage);
        const auto chosenEpisode = *result.mSelectedPage;

        if (
          mContext.mpServiceProvider->isSharewareVersion() && chosenEpisode > 0)
        {
          runScript(mContext, "No_Can_Order");
          mMenuState = MenuState::EpisodeNotAvailableMessageHighScores;
        }
        else
        {
          ui::setupHighScoreListDisplay(mContext, chosenEpisode);
          mChosenEpisode = chosenEpisode;
          mMenuState = MenuState::ShowHiscores;
        }
      }
      break;

    case MenuState::ShowHiscores:
      mContext.mpServiceProvider->fadeOutScreen();
      enterMainMenu();
      break;

    default:
      enterMainMenu();
      break;
  }

  return {};
}

} // namespace rigel
