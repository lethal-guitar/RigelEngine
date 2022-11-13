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

#include "base/math_utils.hpp"
#include "frontend/game_service_provider.hpp"
#include "frontend/user_profile.hpp"
#include "game_logic/game_world.hpp"
#include "ui/utils.hpp"

#include <sstream>


namespace rigel
{

GameRunner::GameRunner(
  data::PlayerModel* pPlayerModel,
  const data::GameSessionId& sessionId,
  GameMode::Context context,
  const std::optional<base::Vec2> playerPositionOverride,
  const bool showWelcomeMessage)
  : mContext(context)
  , mpWorld(std::make_unique<game_logic::GameWorld>(
      pPlayerModel,
      sessionId,
      context,
      playerPositionOverride,
      showWelcomeMessage))
  , mInputHandler(&context.mpUserProfile->mOptions)
  , mMenu(context, pPlayerModel, mpWorld.get(), sessionId)
{
}


void GameRunner::handleEvent(const SDL_Event& event)
{
  if (gameQuit() || requestedGameToLoad())
  {
    return;
  }

  mMenu.handleEvent(event);
  if (mMenu.isActive())
  {
    // The menu overrides game event handling when it is active, therefore
    // stop here
    return;
  }

  const auto menuCommand =
    mInputHandler.handleEvent(event, mpWorld->isPlayerInShip());

  switch (menuCommand)
  {
    case InputHandler::MenuCommand::QuickSave:
      mpWorld->quickSave();
      break;

    case InputHandler::MenuCommand::QuickLoad:
      mpWorld->quickLoad();
      break;

    default:
      break;
  }

  const auto debugModeEnabled =
    mContext.mpServiceProvider->commandLineOptions().mDebugModeEnabled;
  if (debugModeEnabled)
  {
    handleDebugKeys(event);
  }
}


void GameRunner::updateAndRender(engine::TimeDelta dt)
{
  if (gameQuit() || levelFinished() || requestedGameToLoad())
  {
    // TODO: This is a workaround to make the fadeout on quitting work.
    // Would be good to find a better way to do this.
    mpWorld->render(interpolationFactor(dt));
    return;
  }

  if (updateMenu(dt))
  {
    return;
  }

  updateWorld(dt);
  mpWorld->render(interpolationFactor(dt));

  renderDebugText();
  mpWorld->processEndOfFrameActions();
}


bool GameRunner::needsPerElementUpscaling() const
{
  return mpWorld->needsPerElementUpscaling();
}


float GameRunner::interpolationFactor(const engine::TimeDelta dt) const
{
  return mContext.mpUserProfile->mOptions.mMotionSmoothing
    ? static_cast<float>(mAccumulatedTime / game_logic::GAME_LOGIC_UPDATE_DELAY)
    : 1.0f;
}


void GameRunner::updateWorld(const engine::TimeDelta dt)
{
  auto update = [this]() {
    mpWorld->updateGameLogic(mInputHandler.fetchInput());
  };


  if (mSingleStepping)
  {
    if (mDoNextSingleStep)
    {
      update();
      mDoNextSingleStep = false;
    }
  }
  else
  {
    mAccumulatedTime += dt;
    for (; mAccumulatedTime >= game_logic::GAME_LOGIC_UPDATE_DELAY;
         mAccumulatedTime -= game_logic::GAME_LOGIC_UPDATE_DELAY)
    {
      update();
    }

    mpWorld->updateBackdropAutoScrolling(dt);
  }
}


bool GameRunner::updateMenu(const engine::TimeDelta dt)
{
  if (mMenu.isActive())
  {
    mInputHandler.reset();

    if (mMenu.isTransparent())
    {
      mpWorld->render(interpolationFactor(dt));
    }

    const auto result = mMenu.updateAndRender(dt);

    if (result == ui::IngameMenu::UpdateResult::FinishedNeedsFadeout)
    {
      mContext.mpServiceProvider->fadeOutScreen();
      mpWorld->render(interpolationFactor(dt));
      mContext.mpServiceProvider->fadeInScreen();
    }

    return true;
  }

  return false;
}


void GameRunner::handleDebugKeys(const SDL_Event& event)
{
  if (event.type != SDL_KEYDOWN || event.key.repeat != 0)
  {
    return;
  }

  switch (event.key.keysym.sym)
  {
    case SDLK_b:
      mpWorld->debugToggleBoundingBoxDisplay();
      break;

    case SDLK_c:
      mpWorld->debugToggleWorldCollisionDataDisplay();
      break;

    case SDLK_d:
      mShowDebugText = !mShowDebugText;
      break;

    case SDLK_g:
      mpWorld->debugToggleGridDisplay();
      break;

    case SDLK_s:
      mSingleStepping = !mSingleStepping;
      break;

    case SDLK_SPACE:
      if (mSingleStepping)
      {
        mDoNextSingleStep = true;
      }
      break;

    case SDLK_F10:
      mpWorld->toggleGodMode();
      break;

    case SDLK_F11:
      mLevelFinishedByDebugKey = true;
      break;
  }
}


void GameRunner::renderDebugText()
{
  std::stringstream debugText;

  if (mpWorld->isGodModeOn())
  {
    debugText << "GOD MODE on\n";
  }

  if (mShowDebugText)
  {
    mpWorld->printDebugText(debugText);
  }

  ui::drawText(debugText.str(), 0, 32, {255, 255, 255, 255});
}


bool GameRunner::levelFinished() const
{
  return mpWorld->levelFinished() || mLevelFinishedByDebugKey;
}


bool GameRunner::gameQuit() const
{
  return mMenu.quitRequested();
}


std::optional<data::SavedGame> GameRunner::requestedGameToLoad() const
{
  return mMenu.requestedGameToLoad();
}


std::set<data::Bonus> GameRunner::achievedBonuses() const
{
  return mpWorld->achievedBonuses();
}

} // namespace rigel
