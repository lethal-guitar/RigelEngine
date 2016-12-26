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

#include "base/warnings.hpp"

RIGEL_DISABLE_WARNINGS
#include <atria/variant/match_boost.hpp>
RIGEL_RESTORE_WARNINGS


namespace rigel {

GameSessionMode::GameSessionMode(
  const int episode,
  const int level,
  const data::Difficulty difficulty,
  Context context,
  boost::optional<base::Vector> playerPositionOverride
)
  : mCurrentStage(std::make_unique<IngameMode>(
      episode, level, difficulty, context, playerPositionOverride))
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

  atria::variant::match(mCurrentStage,
    [&event](std::unique_ptr<IngameMode>& pIngameMode) {
      pIngameMode->handleEvent(event);
    },

    [](auto&) {}
    );
}


void GameSessionMode::updateAndRender(engine::TimeDelta dt) {
  atria::variant::match(mCurrentStage,
    [this, &dt](std::unique_ptr<IngameMode>& pIngameMode) {
      pIngameMode->updateAndRender(dt);

      if (pIngameMode->levelFinished()) {
        auto bonusScreen = ui::BonusScreen{mContext, {}, 0};
        fadeToNewStage(bonusScreen);
        mCurrentStage = std::move(bonusScreen);
      }
    },

    [this, &dt](ui::BonusScreen& bonusScreen) {
      bonusScreen.updateAndRender(dt);

      if (bonusScreen.finished()) {
        auto pNextIngameMode = std::make_unique<IngameMode>(
          mEpisode, ++mCurrentLevelNr, mDifficulty, mContext);
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
