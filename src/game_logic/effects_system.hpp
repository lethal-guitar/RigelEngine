/* Copyright (C) 2017, Nikolai Wuttke. All rights reserved.
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

#include "base/spatial_types.hpp"
#include "base/warnings.hpp"
#include "game_logic/effect_components.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

namespace rigel { struct IGameServiceProvider; }
namespace rigel::engine {
  namespace events {
    struct CollidedWithWorld;
  }

  class RandomNumberGenerator;
  class ParticleSystem;
}
namespace rigel::game_logic::events {
  struct ShootableKilled;
}


namespace rigel::game_logic {

namespace components { struct DestructionEffects; }
class EntityFactory;


void triggerEffects(
  entityx::Entity entity, entityx::EntityManager& entityManager);


class EffectsSystem : public entityx::Receiver<EffectsSystem> {
public:
  EffectsSystem(
    IGameServiceProvider* pServiceProvider,
    engine::RandomNumberGenerator* pRandomGenerator,
    entityx::EntityManager* pEntityManager,
    EntityFactory* pEntityFactory,
    engine::ParticleSystem* pParticles,
    entityx::EventManager& events);

  void update(entityx::EntityManager& es);

  void receive(const events::ShootableKilled& event);
  void receive(const engine::events::CollidedWithWorld& event);

private:
  void triggerEffectsIfConditionMatches(
    entityx::Entity entity,
    components::DestructionEffects::TriggerCondition expectedCondition);

  void processEffectsAndAdvance(
    const base::Vector& position,
    components::DestructionEffects& effects);

  IGameServiceProvider* mpServiceProvider;
  engine::RandomNumberGenerator* mpRandomGenerator;
  entityx::EntityManager* mpEntityManager;
  EntityFactory* mpEntityFactory;
  engine::ParticleSystem* mpParticles;
};

}
