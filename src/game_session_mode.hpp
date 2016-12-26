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

#include "base/boost_variant.hpp"
#include "ui/bonus_screen.hpp"

#include "game_mode.hpp"
#include "ingame_mode.hpp"


namespace rigel {

class GameSessionMode : public GameMode {
public:
  GameSessionMode(
    int episode,
    int level,
    data::Difficulty difficulty,
    Context context,
    boost::optional<base::Vector> playerPositionOverride = boost::none);

  void handleEvent(const SDL_Event& event) override;
  void updateAndRender(engine::TimeDelta dt) override;

private:
  template<typename StageT>
  void fadeToNewStage(StageT& stage);

private:
  using SessionStage = boost::variant<
    std::unique_ptr<IngameMode>,
    ui::BonusScreen
  >;

  SessionStage mCurrentStage;
  const int mEpisode;
  int mCurrentLevelNr;
  const data::Difficulty mDifficulty;
  Context mContext;
};

}
