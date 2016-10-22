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

#include <engine/timing.hpp>
#include <loader/resource_bundle.hpp>
#include <ui/utils.hpp>


namespace rigel { namespace ui {

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
  const std::set<BonusNumber>& achievedBonuses,
  int scoreBeforeAddingBonuses
)
  : mScore(scoreBeforeAddingBonuses)
  , mpRenderer(context.mpRenderer)
  , mpServiceProvider(context.mpServiceProvider)
  , mBackgroundTexture(ui::fullScreenImageAsTexture(
      context.mpRenderer,
      *context.mpResources,
      "BONUSSCN.MNI"))
  , mTextRenderer(context.mpRenderer, *context.mpResources)
{
  context.mpServiceProvider->playMusic("OPNGATEA.IMF");

  engine::TimeDelta time = 0.0;
  if (!achievedBonuses.empty()) {
    time = setupBonusSummationSequence(achievedBonuses);
  } else {
    time = setupNoBonusSequence();
  }

  time += slowTicksToTime(FINAL_DELAY_TICKS);
  mEvents.emplace_back(Event{
    time,
    [this]() {
      mIsDone = true;
    }
  });
}


void BonusScreen::updateAndRender(engine::TimeDelta dt) {
  updateSequence(dt);

  mBackgroundTexture.render(mpRenderer, 0, 0);
  mTextRenderer.drawBonusScreenText(6, 8, "SCORE");
  mTextRenderer.drawBonusScreenText(6, 17, mRunningText);

  const auto scoreAsText = to_string(mScore);
  const auto scorePosX = static_cast<int>(34 - scoreAsText.size() * 2);
  mTextRenderer.drawBonusScreenText(scorePosX, 8, scoreAsText);
}


void BonusScreen::updateSequence(const engine::TimeDelta timeDelta) {
  if (mIsDone) {
    return;
  }

  mElapsedTime += timeDelta;

  if (mElapsedTime >= mEvents[mNextEvent].mTime) {
    mEvents[mNextEvent].mAction();
    ++mNextEvent;
  }
}


engine::TimeDelta BonusScreen::setupBonusSummationSequence(
  const std::set<BonusNumber>& achievedBonuses
) {
  auto time = slowTicksToTime(INITIAL_DELAY_TICKS);

  for (const auto bonus : achievedBonuses) {
    time += slowTicksToTime(100);

    for (const auto& text : BONUS_SLIDE_IN) {
      mEvents.emplace_back(Event{
        time,
        [this, text]() {
          mRunningText = text;
        }
      });

      time += slowTicksToTime(5);
    }

    mEvents.emplace_back(Event{
      time,
      [this, bonus]() {
        mRunningText = mRunningText + " " + to_string(bonus);
        mpServiceProvider->playSound(data::SoundId::BigExplosion);
      }
    });

    time += slowTicksToTime(190);
    mEvents.emplace_back(Event{
      time,
      [this]() {
        mRunningText = "  100000 PTS";
      }
    });
    time += slowTicksToTime(100);

    for (int iteration = 0; iteration < 100; ++iteration) {
      mEvents.emplace_back(Event{
        time,
        [this, iteration]() {
          mScore += 1000;
          mpServiceProvider->playSound(data::SoundId::DukeJumping);

          const auto newNumberAsText = to_string(99000 - iteration*1000);

          auto newText = string("  ");
          const auto placesToSkip = 6 - newNumberAsText.size();
          for (auto i = 0u; i < placesToSkip; ++i) {
            newText += " ";
          }
          newText += newNumberAsText;
          newText += " PTS";
          mRunningText = newText;
        }
      });

      time += slowTicksToTime(2);
    }

    mEvents.emplace_back(Event{
      time,
      [this]() {
        mRunningText = "       0 PTS";
        mpServiceProvider->playSound(data::SoundId::BigExplosion);
      }
    });

    time += slowTicksToTime(50);
  }

  return time;
}

engine::TimeDelta BonusScreen::setupNoBonusSequence() {
  auto time = slowTicksToTime(100 + INITIAL_DELAY_TICKS);

  for (int i = 0; i < 14; ++i) {
    mEvents.emplace_back(Event{
      time,
      [this, i]() {
        mRunningText = NO_BONUS_SLIDE_IN[i];
      }
    });
    time += slowTicksToTime(5);
  }

  mEvents.emplace_back(Event{
    time,
    [this]() {
      mpServiceProvider->playSound(data::SoundId::BigExplosion);
    }
  });
  time += slowTicksToTime(130);

  for (int i = 14; i < 20; ++i) {
    mEvents.emplace_back(Event{
      time,
      [this, i]() {
        mRunningText = NO_BONUS_SLIDE_IN[i];
      }
    });
    time += slowTicksToTime(10);
  }

  mEvents.emplace_back(Event{
    time,
    [this]() {
      mpServiceProvider->playSound(data::SoundId::BigExplosion);
    }
  });
  time += slowTicksToTime(130);

  for (int i = 20; i < 27; ++i) {
    mEvents.emplace_back(Event{
      time,
      [this, i]() {
        mRunningText = NO_BONUS_SLIDE_IN[i];
      }
    });
    time += slowTicksToTime(10);
  }

  time += slowTicksToTime(15);
  mEvents.emplace_back(Event{
    time,
    [this]() {
      mpServiceProvider->playSound(data::SoundId::BigExplosion);
    }
  });

  return time;
}

}}
