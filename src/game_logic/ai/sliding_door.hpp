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

#include "base/warnings.hpp"
#include "engine/base_components.hpp"
#include "engine/renderer.hpp"
#include "engine/visual_components.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

namespace rigel { struct IGameServiceProvider; }


namespace rigel { namespace game_logic { namespace ai {

namespace components {

struct HorizontalSlidingDoor {
  enum class State {
    Closed = 0,
    HalfOpen = 1,
    Open = 2,
  };

  State mState = State::Closed;
  bool mPlayerWasInRange = false;
  entityx::Entity mCollisionHelper;
};


struct VerticalSlidingDoor {
  enum class State {
    Closed,
    Opening,
    Open,
    Closing
  };

  State mState = State::Closed;
  bool mPlayerWasInRange = false;
  int mSlideStep = 0;
};

}


class SlidingDoorSystem {
public:
  SlidingDoorSystem(
    entityx::Entity playerEntity,
    IGameServiceProvider* pServiceProvider);

  void update(entityx::EntityManager& es);

private:
  template<typename StateT>
  void updateSoundGeneration(const bool inRange, StateT& state);

private:
  entityx::Entity mPlayerEntity;
  IGameServiceProvider* mpServiceProvider;
};

}


void renderVerticalSlidingDoor(
  engine::Renderer* pRenderer,
  entityx::Entity,
  const engine::components::Sprite& sprite,
  const base::Vector& positionInScreenSpace);

}}
