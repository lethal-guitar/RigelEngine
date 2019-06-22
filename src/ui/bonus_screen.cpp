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

#include "bonus_screen.hpp"

#include "common/game_service_provider.hpp"
#include "engine/timing.hpp"
#include "loader/resource_loader.hpp"
#include "ui/utils.hpp"


namespace rigel::ui {

using namespace std;
using engine::slowTicksToTime;


namespace {

const auto INITIAL_DELAY_TICKS = 60;
const auto FINAL_DELAY_TICKS = 425;


const std::array<const char*, 6> BONUS_SLIDE_IN{
  "S",
  "ONUS",
  " BONUS",
  "ET BONUS",
  "CRET BONUS",
  "SECRET BONUS"};

const std::array<const char*, 27> NO_BONUS_SLIDE_IN{
  "            ==",
  "          ====",
  "        ======",
  "      ========",
  "    ==========",
  "  ============",
  "==============",
  "  ============",
  "   N==========",
  "   NO ========",
  "   NO BO======",
  "   NO BONU====",
  "   NO BONUS!==",
  "   NO BONUS!  ",
  " NO BONUS!  BE",
  "O BONUS! BETTE",
  "BONUS! BETTER ",
  "NUS! BETTER LU",
  "S! BETTER LUCK",
  " BETTER LUCK! ",
  "ETTER LUCK!  N",
  "TER LUCK!  NEX",
  "R LUCK!  NEXT ",
  "LUCK!  NEXT TI",
  "CK!  NEXT TIME",
  "!  NEXT TIME! ",
  "  NEXT TIME!  "};

}


BonusScreen::BonusScreen(
  GameMode::Context context,
  const std::set<data::Bonus>& achievedBonuses,
  int scoreBeforeAddingBonuses
)
  : mState(scoreBeforeAddingBonuses)
  , mpRenderer(context.mpRenderer)
  , mBackgroundTexture(ui::fullScreenImageAsTexture(
      context.mpRenderer,
      *context.mpResources,
      "BONUSSCN.MNI"))
  , mpTextRenderer(context.mpUiRenderer)
{
  context.mpServiceProvider->playMusic("OPNGATEA.IMF");

  engine::TimeDelta time = 0.0;
  if (!achievedBonuses.empty()) {
    time =
      setupBonusSummationSequence(achievedBonuses, context.mpServiceProvider);
  } else {
    time = setupNoBonusSequence(context.mpServiceProvider);
  }

  time += slowTicksToTime(FINAL_DELAY_TICKS);
  mEvents.emplace_back(Event{
    time,
    [](State& state) {
      state.mIsDone = true;
    }
  });
}


void BonusScreen::updateAndRender(engine::TimeDelta dt) {
  updateSequence(dt);

  mBackgroundTexture.render(mpRenderer, 0, 0);
  mpTextRenderer->drawBonusScreenText(6, 8, "SCORE");
  mpTextRenderer->drawBonusScreenText(6, 17, mState.mRunningText);

  const auto scoreAsText = to_string(mState.mScore);
  const auto scorePosX = static_cast<int>(34 - scoreAsText.size() * 2);
  mpTextRenderer->drawBonusScreenText(scorePosX, 8, scoreAsText);
}


void BonusScreen::updateSequence(const engine::TimeDelta timeDelta) {
  if (mState.mIsDone) {
    return;
  }

  mElapsedTime += timeDelta;

  if (mElapsedTime >= mEvents[mNextEvent].mTime) {
    mEvents[mNextEvent].mAction(mState);
    ++mNextEvent;
  }
}


engine::TimeDelta BonusScreen::setupBonusSummationSequence(
  const std::set<data::Bonus>& achievedBonuses,
  IGameServiceProvider* pServiceProvider
) {
  auto time = slowTicksToTime(INITIAL_DELAY_TICKS);

  for (const auto bonus : achievedBonuses) {
    time += slowTicksToTime(100);

    for (const auto& text : BONUS_SLIDE_IN) {
      mEvents.emplace_back(Event{
        time,
        [text](State& state) {
          state.mRunningText = text;
        }
      });

      time += slowTicksToTime(5);
    }

    mEvents.emplace_back(Event{
      time,
      [pServiceProvider, bonus](State& state) {
        state.mRunningText += " " + to_string(asNumber(bonus));
        pServiceProvider->playSound(data::SoundId::BigExplosion);
      }
    });

    time += slowTicksToTime(190);
    mEvents.emplace_back(Event{
      time,
      [](State& state) {
        state.mRunningText = "  100000 PTS";
      }
    });
    time += slowTicksToTime(100);

    for (int iteration = 0; iteration < 100; ++iteration) {
      mEvents.emplace_back(Event{
        time,
        [pServiceProvider, iteration](State& state) {
          state.mScore += 1000;
          pServiceProvider->playSound(data::SoundId::DukeJumping);

          const auto newNumberAsText = to_string(99000 - iteration*1000);

          auto newText = string("  ");
          const auto placesToSkip = 6 - newNumberAsText.size();
          for (auto i = 0u; i < placesToSkip; ++i) {
            newText += " ";
          }
          newText += newNumberAsText;
          newText += " PTS";
          state.mRunningText = newText;
        }
      });

      time += slowTicksToTime(2);
    }

    mEvents.emplace_back(Event{
      time,
      [pServiceProvider](State& state) {
        state.mRunningText = "       0 PTS";
        pServiceProvider->playSound(data::SoundId::BigExplosion);
      }
    });

    time += slowTicksToTime(50);
  }

  return time;
}

engine::TimeDelta BonusScreen::setupNoBonusSequence(
  IGameServiceProvider* pServiceProvider
) {
  auto time = slowTicksToTime(100 + INITIAL_DELAY_TICKS);

  for (int i = 0; i < 14; ++i) {
    mEvents.emplace_back(Event{
      time,
      [i](State& state) {
        state.mRunningText = NO_BONUS_SLIDE_IN[i];
      }
    });
    time += slowTicksToTime(5);
  }

  mEvents.emplace_back(Event{
    time,
    [pServiceProvider](State&) {
      pServiceProvider->playSound(data::SoundId::BigExplosion);
    }
  });
  time += slowTicksToTime(130);

  for (int i = 14; i < 20; ++i) {
    mEvents.emplace_back(Event{
      time,
      [i](State& state) {
        state.mRunningText = NO_BONUS_SLIDE_IN[i];
      }
    });
    time += slowTicksToTime(10);
  }

  mEvents.emplace_back(Event{
    time,
    [pServiceProvider](State&) {
      pServiceProvider->playSound(data::SoundId::BigExplosion);
    }
  });
  time += slowTicksToTime(130);

  for (int i = 20; i < 27; ++i) {
    mEvents.emplace_back(Event{
      time,
      [i](State& state) {
        state.mRunningText = NO_BONUS_SLIDE_IN[i];
      }
    });
    time += slowTicksToTime(10);
  }

  time += slowTicksToTime(15);
  mEvents.emplace_back(Event{
    time,
    [pServiceProvider](State&) {
      pServiceProvider->playSound(data::SoundId::BigExplosion);
    }
  });

  return time;
}

}
