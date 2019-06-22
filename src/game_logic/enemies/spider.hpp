/* Copyright (C) 2018, Nikolai Wuttke. All rights reserved.
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
#include "engine/physical_components.hpp"
#include "game_logic/player.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

namespace rigel {

namespace engine { class CollisionChecker; }
namespace engine { class RandomNumberGenerator; }
namespace game_logic { struct IEntityFactory; }

}


namespace rigel::game_logic::ai {

namespace components {

// TODO: Use variant pattern for states
struct Spider {
  enum class State {
    Uninitialized,
    OnCeiling,
    Falling,
    OnFloor,
    ClingingToPlayer
  };

  State mState = State::Uninitialized;
  engine::components::Orientation mPreviousPlayerOrientation;
  int mShakeOffProgress = 0;
  SpiderClingPosition mClingPosition;
};

}


class SpiderSystem : public entityx::Receiver<SpiderSystem> {
public:
  SpiderSystem(
    Player* pPlayer,
    engine::CollisionChecker* pCollisionChecker,
    engine::RandomNumberGenerator* pRandomGenerator,
    IEntityFactory* pEntityFactory,
    entityx::EventManager& events);

  void update(entityx::EntityManager& es);
  void receive(const engine::events::CollidedWithWorld& event);

private:
  Player* mpPlayer;
  engine::CollisionChecker* mpCollisionChecker;
  engine::RandomNumberGenerator* mpRandomGenerator;
  IEntityFactory* mpEntityFactory;

  bool mIsOddFrame = false;
};

}
