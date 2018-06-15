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

#include "item_container.hpp"

#include "data/sound_ids.hpp"
#include "engine/base_components.hpp"
#include "engine/collision_checker.hpp"
#include "engine/life_time_components.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/effect_components.hpp"
#include "game_logic/entity_factory.hpp"

#include "game_service_provider.hpp"


namespace rigel { namespace game_logic {

using engine::components::Active;
using engine::components::BoundingBox;
using engine::components::Sprite;
using engine::components::WorldPosition;
using game_logic::components::ItemContainer;


ItemContainerSystem::ItemContainerSystem(
  entityx::EntityManager* pEntityManager,
  entityx::EventManager& events
)
  : mpEntityManager(pEntityManager)
{
  events.subscribe<events::ShootableKilled>(*this);
}


void ItemContainerSystem::update(entityx::EntityManager& es) {
  for (auto& entity : mShotContainersQueue) {
    const auto& container = *entity.component<const ItemContainer>();

    auto contents = mpEntityManager->create();
    for (auto& component : container.mContainedComponents) {
      component.assignToEntity(contents);
    }

    contents.assign<Active>();
    contents.assign<WorldPosition>(*entity.component<WorldPosition>());

    entity.destroy();
  }
  mShotContainersQueue.clear();
}


void ItemContainerSystem::receive(const events::ShootableKilled& event) {
  auto entity = event.mEntity;
  if (entity.has_component<ItemContainer>()) {
    // We can't open up the item container immediately, but have to do it
    // in our update() function. This is because the container's contents
    // might be shootable, and this could cause them to be hit by the
    // same projectile as the one that opened the container. By deferring
    // opening the container to our update, the damage infliction update
    // will be finished, so this problem can't occur.
    entity.component<components::Shootable>()->mDestroyWhenKilled = false;
    mShotContainersQueue.emplace_back(entity);
  }
}


NapalmBombSystem::NapalmBombSystem(
  IGameServiceProvider* pServiceProvider,
  EntityFactory* pEntityFactory,
  engine::CollisionChecker* pCollisionChecker,
  entityx::EventManager& events
)
  : mpServiceProvider(pServiceProvider)
  , mpEntityFactory(pEntityFactory)
  , mpCollisionChecker(pCollisionChecker)
{
  events.subscribe<events::ShootableKilled>(*this);
}


void NapalmBombSystem::update(entityx::EntityManager& es) {
  using components::DestructionEffects;

  using State = components::NapalmBomb::State;
  es.each<components::NapalmBomb, WorldPosition, Sprite>(
    [this](
      entityx::Entity entity,
      components::NapalmBomb& state,
      const WorldPosition& position,
      Sprite& sprite
    ) {
      ++state.mFramesElapsed;

      switch (state.mState) {
        case State::Ticking:
          if (state.mFramesElapsed >= 25 && state.mFramesElapsed % 2 == 1) {
            sprite.flashWhite();
          }

          if (state.mFramesElapsed >= 31) {
            entity.component<DestructionEffects>()->mActivated = true;
            explode(entity);
          }
          break;

        case State::SpawningFires:
          if (state.mFramesElapsed > 10) {
            entity.destroy();
            return;
          }

          if (state.mFramesElapsed % 2 == 0) {
            const auto step = state.mFramesElapsed / 2;
            spawnFires(state, position, step);
          }
          break;
      }
    });
}


void NapalmBombSystem::receive(const events::ShootableKilled& event) {
  auto entity = event.mEntity;
  if (entity.has_component<components::NapalmBomb>()) {
    explode(entity);
  }
}


void NapalmBombSystem::explode(entityx::Entity entity) {
  const auto& position = *entity.component<WorldPosition>();
  auto& state = *entity.component<components::NapalmBomb>();

  mpServiceProvider->playSound(data::SoundId::Explosion);
  spawnFires(state, position, 0);

  state.mState = components::NapalmBomb::State::SpawningFires;
  state.mFramesElapsed = 0;
  entity.component<Sprite>()->mShow = false;
  entity.remove<engine::components::MovingBody>();
}


void NapalmBombSystem::spawnFires(
  components::NapalmBomb& state,
  const base::Vector& bombPosition,
  const int step
) {
  using namespace game_logic::components::parameter_aliases;

  auto spawnOneFire = [this](const base::Vector& position) {
    const auto canSpawn =
      mpCollisionChecker->isOnSolidGround(
        position,
        BoundingBox{{}, {2, 1}});

    if (canSpawn) {
      auto fire = createOneShotSprite(*mpEntityFactory, 65, position);
      fire.assign<components::PlayerDamaging>(Damage{1});
      fire.assign<components::DamageInflicting>(
        Damage{1},
        DestroyOnContact{false});
    }
    return canSpawn;
  };

  const auto basePosition = WorldPosition{step + 1, 0};
  if (state.mCanSpawnLeft) {
    state.mCanSpawnLeft = spawnOneFire(bombPosition + basePosition * -2);
  }

  if (state.mCanSpawnRight) {
    state.mCanSpawnRight = spawnOneFire(bombPosition + basePosition * 2);
  }
}

}}
