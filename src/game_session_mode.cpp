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
#include "data/saved_game.hpp"

#include "game_service_provider.hpp"


namespace rigel {

GameSessionMode::GameSessionMode(
  const data::GameSessionId& sessionId,
  Context context,
  std::optional<base::Vector> playerPositionOverride
)
  : mCurrentStage(std::make_unique<GameRunner>(
      &mPlayerModel,
      sessionId,
      context,
      playerPositionOverride,
      true /* show welcome message */))
  , mEpisode(sessionId.mEpisode)
  , mCurrentLevelNr(sessionId.mLevel)
  , mDifficulty(sessionId.mDifficulty)
  , mContext(context)
{
}


GameSessionMode::GameSessionMode(const data::SavedGame& save, Context context)
  : mPlayerModel(save)
  , mCurrentStage(std::make_unique<GameRunner>(
      &mPlayerModel,
      save.mSessionId,
      context,
      std::nullopt,
      true /* show welcome message */))
  , mEpisode(save.mSessionId.mEpisode)
  , mCurrentLevelNr(save.mSessionId.mLevel)
  , mDifficulty(save.mSessionId.mDifficulty)
  , mContext(context)
{
}


void GameSessionMode::handleEvent(const SDL_Event& event) {
  base::match(mCurrentStage,
    [&event, this](std::unique_ptr<GameRunner>& pIngameMode) {
      pIngameMode->handleEvent(event);

      if (pIngameMode->gameQuit()) {
        finishGameSession();
      }
    },

    [&event](ui::EpisodeEndSequence& endScreens) {
      endScreens.handleEvent(event);
    },

    [](auto&) {});
}


void GameSessionMode::updateAndRender(engine::TimeDelta dt) {
  base::match(mCurrentStage,
    [this, &dt](std::unique_ptr<GameRunner>& pIngameMode) {
      pIngameMode->updateAndRender(dt);

      if (pIngameMode->levelFinished()) {
        const auto achievedBonuses = pIngameMode->achievedBonuses();

        auto bonusScreen =
          ui::BonusScreen{mContext, achievedBonuses, mPlayerModel.score()};

        data::addBonusScore(mPlayerModel, achievedBonuses);

        if (data::isBossLevel(mCurrentLevelNr)) {
          mContext.mpServiceProvider->playMusic("NEVRENDA.IMF");

          auto endScreens = ui::EpisodeEndSequence{
            mContext, mEpisode, std::move(bonusScreen)};
          mContext.mpServiceProvider->fadeOutScreen();
          mCurrentStage = std::move(endScreens);
        } else {
          fadeToNewStage(bonusScreen);
          mCurrentStage = std::move(bonusScreen);
        }
      }
    },

    [this, &dt](ui::BonusScreen& bonusScreen) {
      bonusScreen.updateAndRender(dt);

      if (bonusScreen.finished()) {
        mPlayerModel.resetForNewLevel();

        auto pNextIngameMode = std::make_unique<GameRunner>(
          &mPlayerModel,
          data::GameSessionId{mEpisode, ++mCurrentLevelNr, mDifficulty},
          mContext);
        fadeToNewStage(*pNextIngameMode);
        mCurrentStage = std::move(pNextIngameMode);
      }
    },

    [this, &dt](ui::EpisodeEndSequence& endScreens) {
      endScreens.updateAndRender(dt);

      if (endScreens.finished()) {
        finishGameSession();
      }
    });
}


template<typename StageT>
void GameSessionMode::fadeToNewStage(StageT& stage) {
  mContext.mpServiceProvider->fadeOutScreen();
  stage.updateAndRender(0);
  mContext.mpServiceProvider->fadeInScreen();
}


void GameSessionMode::finishGameSession() {
  // TODO: Record and show hiscore
  mContext.mpServiceProvider->fadeOutScreen();
  mContext.mpServiceProvider->scheduleEnterMainMenu();
}

}
