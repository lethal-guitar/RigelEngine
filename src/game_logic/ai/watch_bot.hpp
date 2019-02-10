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

namespace rigel { namespace engine { namespace events {
  struct CollidedWithWorld;
}}}


namespace rigel { namespace game_logic { namespace behaviors {

namespace watch_bot {

struct Jumping {
  Jumping() = default;
  explicit Jumping(const engine::components::Orientation orientation)
    : mOrientation(orientation)
  {
  }

  int mFramesElapsed = 0;
  engine::components::Orientation mOrientation =
    engine::components::Orientation::Left;
};

struct Falling {
  engine::components::Orientation mOrientation;
};

struct OnGround {
  int mFramesElapsed = 0;
};

struct LookingAround {
  explicit LookingAround(const engine::components::Orientation orientation)
    : mOrientation(orientation)
  {
  }

  int mFramesElapsed = 0;
  engine::components::Orientation mOrientation;
};


using State = std::variant<Jumping, Falling, OnGround, LookingAround>;

}


struct WatchBot {
  void update(
    GlobalDependencies& dependencies,
    GlobalState& state,
    bool isOnScreen,
    entityx::Entity entity);

  void onCollision(
    GlobalDependencies& dependencies,
    GlobalState& state,
    const engine::events::CollidedWithWorld& event,
    entityx::Entity entity);

  void land(entityx::Entity entity, GlobalDependencies& dependencies);

  watch_bot::State mState;
};


struct WatchBotCarrier {
  enum class State {
    ApproachingPlayer,
    ReleasingPayload
  };

  void update(
    GlobalDependencies& dependencies,
    GlobalState& state,
    bool isOnScreen,
    entityx::Entity entity);

  entityx::Entity mPayload;
  State mState = State::ApproachingPlayer;
  int mFramesElapsed = 0;
};

}}}
