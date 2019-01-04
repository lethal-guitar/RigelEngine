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

#include "game_session_mode.hpp"

#include "base/match.hpp"

#include "game_service_provider.hpp"


namespace rigel {

GameSessionMode::GameSessionMode(
  const int episode,
  const int level,
  const data::Difficulty difficulty,
  Context context,
  std::optional<base::Vector> playerPositionOverride
)
  : mCurrentStage(std::make_unique<GameRunner>(
      &mPlayerModel,
      data::GameSessionId{episode, level, difficulty},
      context,
      playerPositionOverride,
      true /* show welcome message */))
  , mEpisode(episode)
  , mCurrentLevelNr(level)
  , mDifficulty(difficulty)
  , mContext(context)
{
}


void GameSessionMode::handleEvent(const SDL_Event& event) {
  // This is temporary - remove when in-game menu implemented
  if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
    mContext.mpServiceProvider->fadeOutScreen();
    mContext.mpServiceProvider->scheduleEnterMainMenu();
    return;
  }

  base::match(mCurrentStage,
    [&event](std::unique_ptr<GameRunner>& pIngameMode) {
      pIngameMode->handleEvent(event);
    },

    [](auto&) {}
    );
}


void GameSessionMode::updateAndRender(engine::TimeDelta dt) {
  base::match(mCurrentStage,
    [this, &dt](std::unique_ptr<GameRunner>& pIngameMode) {
      pIngameMode->updateAndRender(dt);

      if (pIngameMode->levelFinished()) {
        auto bonusScreen = ui::BonusScreen{mContext, {}, mPlayerModel.score()};
        fadeToNewStage(bonusScreen);
        mCurrentStage = std::move(bonusScreen);
      }
    },

    [this, &dt](ui::BonusScreen& bonusScreen) {
      bonusScreen.updateAndRender(dt);

      if (bonusScreen.finished()) {
        // TODO: Update player model with new score after bonus screen
        mPlayerModel.resetForNewLevel();

        auto pNextIngameMode = std::make_unique<GameRunner>(
          &mPlayerModel,
          data::GameSessionId{mEpisode, ++mCurrentLevelNr, mDifficulty},
          mContext);
        fadeToNewStage(*pNextIngameMode);
        mCurrentStage = std::move(pNextIngameMode);
      }
    });
}


template<typename StageT>
void GameSessionMode::fadeToNewStage(StageT& stage) {
  mContext.mpServiceProvider->fadeOutScreen();
  stage.updateAndRender(0);
  mContext.mpServiceProvider->fadeInScreen();
}

}
