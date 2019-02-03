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

#include "data/bonus.hpp"
#include "engine/timing.hpp"
#include "ui/menu_element_renderer.hpp"
#include "game_mode.hpp"

#include <functional>
#include <set>


namespace rigel { namespace ui {

class BonusScreen {
public:
  BonusScreen(
    GameMode::Context context,
    const std::set<data::Bonus>& achievedBonuses,
    int scoreBeforeAddingBonuses);

  void updateAndRender(engine::TimeDelta dt);

  bool finished() const {
    return mState.mIsDone;
  }

private:
  struct State {
    explicit State(const int score)
      : mScore(score)
    {
    }

    int mScore;
    std::string mRunningText;
    bool mIsDone = false;
  };

  struct Event {
    engine::TimeDelta mTime;
    std::function<void(State&)> mAction;
  };

  engine::TimeDelta setupBonusSummationSequence(
    const std::set<data::Bonus>& achievedBonuses,
    IGameServiceProvider* pServiceProvider);
  engine::TimeDelta setupNoBonusSequence(
    IGameServiceProvider* pServiceProvider);
  void updateSequence(engine::TimeDelta timeDelta);

private:
  State mState;

  engine::TimeDelta mElapsedTime = 0.0;
  std::vector<Event> mEvents;
  std::size_t mNextEvent = 0;

  engine::Renderer* mpRenderer;
  engine::OwningTexture mBackgroundTexture;
  ui::MenuElementRenderer mTextRenderer;
};

}}
