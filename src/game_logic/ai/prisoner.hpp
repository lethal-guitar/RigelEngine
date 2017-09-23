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

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

namespace rigel { namespace engine {
  class RandomNumberGenerator;

  namespace components { struct Sprite; }
}}


namespace rigel { namespace game_logic { namespace ai {

namespace components {

struct Prisoner {
  enum class State {
    Idle,
    Grabbing,
    Dieing
  };

  explicit Prisoner(const bool isAggressive)
    : mIsAggressive(isAggressive)
  {
  }

  bool mIsAggressive;
  State mState = State::Idle;
  int mGrabStep = 0;
};

}


class PrisonerSystem : public entityx::System<PrisonerSystem> {
public:
  PrisonerSystem(
    entityx::Entity player,
    engine::RandomNumberGenerator* pRandomGenerator);

  void update(
    entityx::EntityManager& es,
    entityx::EventManager& events,
    entityx::TimeDelta dt) override;

  void onEntityHit(entityx::Entity entity);

private:
  void updateAggressivePrisoner(
    entityx::Entity entity,
    const engine::components::WorldPosition& position,
    components::Prisoner& state,
    engine::components::Sprite& sprite
  );

private:
  entityx::Entity mPlayer;
  engine::RandomNumberGenerator* mpRandomGenerator;
  bool mIsOddFrame = false;
};

}}}
