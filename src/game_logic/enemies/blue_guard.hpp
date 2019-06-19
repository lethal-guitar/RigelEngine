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

namespace rigel {

struct IGameServiceProvider;
namespace engine { class CollisionChecker; }
namespace engine { class RandomNumberGenerator; }
namespace engine::components { struct Sprite; }
namespace game_logic {
  class EntityFactory;
  class Player;
}
namespace game_logic::events { struct ShootableDamaged; }

}


namespace rigel::game_logic::ai {

namespace components {

struct BlueGuard {
  static BlueGuard typingOnTerminal() {
    BlueGuard instance;
    instance.mTypingOnTerminal = true;
    return instance;
  }

  static BlueGuard patrolling(const engine::components::Orientation orientation) {
    BlueGuard instance;
    instance.mOrientation = orientation;
    return instance;
  }

  engine::components::Orientation mOrientation =
    engine::components::Orientation::Left;
  int mStanceChangeCountdown = 0;
  int mStepsWalked = 0;
  bool mTypingOnTerminal = false;
  bool mOneStepWalkedSinceTypingStop = true;
  bool mIsCrouched = false;
};

}

class BlueGuardSystem : public entityx::Receiver<BlueGuardSystem> {
public:
  BlueGuardSystem(
    const Player* pPlayer,
    engine::CollisionChecker* pCollisionChecker,
    EntityFactory* pEntityFactory,
    IGameServiceProvider* pServiceProvider,
    engine::RandomNumberGenerator* pRandomGenerator,
    entityx::EventManager& events);

  void update(entityx::EntityManager& es);
  void receive(const events::ShootableDamaged& event);

private:
  void stopTyping(
    components::BlueGuard& state,
    engine::components::Sprite& sprite,
    engine::components::WorldPosition& position);

  void updateGuard(
    entityx::Entity guardEntity,
    components::BlueGuard& state,
    engine::components::Sprite& sprite,
    engine::components::WorldPosition& position);

private:
  const Player* mpPlayer;
  engine::CollisionChecker* mpCollisionChecker;
  EntityFactory* mpEntityFactory;
  IGameServiceProvider* mpServiceProvider;
  engine::RandomNumberGenerator* mpRandomGenerator;

  bool mIsOddFrame = false;
};

}
