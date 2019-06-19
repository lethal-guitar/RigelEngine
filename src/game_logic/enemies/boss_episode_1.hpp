/* Copyright (C) 2019, Nikolai Wuttke. All rights reserved.
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

#include "engine/base_components.hpp"
#include "game_logic/global_dependencies.hpp"

#include <variant>


namespace rigel::engine::events {
  struct CollidedWithWorld;
}


namespace rigel::game_logic {

namespace boss_episode_1 {

struct AwaitingActivation {};

struct SlammingDown {};

struct RisingBackUp {};

struct FlyingLeftOnUpperLevel {};

struct FlyingRightDroppingBombs {};

struct MovingDownOnRightSide {
  int mFramesElapsed = 0;
};

struct FlyingLeftOnLowerLevel {};

struct MovingUpOnLeftSide {};

struct ZigZagging {
  int mFramesElapsed = 0;
  engine::components::Orientation mOrientation =
    engine::components::Orientation::Left;
};

struct Dieing {
  int mFramesElapsed = 0;
};


using State = std::variant<
  AwaitingActivation,
  SlammingDown,
  RisingBackUp,
  FlyingLeftOnUpperLevel,
  FlyingRightDroppingBombs,
  MovingDownOnRightSide,
  FlyingLeftOnLowerLevel,
  MovingUpOnLeftSide,
  ZigZagging,
  Dieing>;

}


namespace behaviors {

struct BossEpisode1 {
  void update(
    GlobalDependencies& dependencies,
    GlobalState& state,
    bool isOnScreen,
    entityx::Entity entity);

  void onCollision(
    GlobalDependencies& d,
    GlobalState&,
    const engine::events::CollidedWithWorld&,
    entityx::Entity entity);

  void onKilled(
    GlobalDependencies& dependencies,
    GlobalState& state,
    const base::Point<float>& inflictorVelocity,
    entityx::Entity entity);

  boss_episode_1::State mState;
  int mStartingAltitude = 0;
};

}}

