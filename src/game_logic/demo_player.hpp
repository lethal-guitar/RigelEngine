/* Copyright (C) 2020, Nikolai Wuttke. All rights reserved.
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

#include "common/game_mode.hpp"
#include "data/player_model.hpp"
#include "engine/timing.hpp"
#include "game_logic/input.hpp"

#include <memory>
#include <vector>


namespace rigel::game_logic {

class GameWorld;


struct DemoInput {
  PlayerInput mInput;
  bool mNextLevel;
};


class DemoPlayer {
public:
  explicit DemoPlayer(GameMode::Context context);

  void updateAndRender(engine::TimeDelta dt);

  bool isFinished() const;

private:
  GameMode::Context mContext;
  data::PlayerModel mPlayerModel;

  std::vector<DemoInput> mFrames;
  std::size_t mCurrentFrameIndex = 1;
  std::size_t mLevelIndex = 0;
  engine::TimeDelta mElapsedTime = 0;

  std::unique_ptr<GameWorld> mpWorld;
};

}
