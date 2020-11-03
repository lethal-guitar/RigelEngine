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

#include "behavior_controller_system.hpp"

#include "common/global.hpp"
#include "common/game_service_provider.hpp"
#include "data/player_model.hpp"
#include "data/sound_ids.hpp"
#include "engine/base_components.hpp"
#include "engine/physical_components.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/behavior_controller.hpp"
#include "game_logic/compatibility_mode.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic {

using engine::components::Active;
using engine::components::BoundingBox;
using engine::components::CollidedWithWorld;
using engine::components::MovingBody;
using engine::components::Sprite;
using engine::components::WorldPosition;
using game_logic::components::BehaviorController;
using game_logic::components::DamageInflicting;
using game_logic::components::Shootable;
using game_logic::components::SlotIndex;

namespace {

auto extractVelocity(entityx::Entity entity) {
  return entity.has_component<MovingBody>()
    ? entity.component<MovingBody>()->mVelocity
    : base::Point<float>{};
}

}


BehaviorControllerSystem::BehaviorControllerSystem(
  GlobalDependencies dependencies,
  Player* pPlayer,
  const base::Vector* pCameraPosition,
  data::map::Map* pMap
)
  : mDependencies(dependencies)
  , mGlobalState(
      pPlayer,
      pCameraPosition,
      pMap,
      &mPerFrameState)
{
  mDependencies.mpEvents->subscribe<engine::events::CollidedWithWorld>(*this);
}


void BehaviorControllerSystem::update(
  entityx::EntityManager& es,
  const PerFrameState& s
) {
  std::vector<std::tuple<entityx::Entity, BoundingBox>> inflictors;
  es.each<DamageInflicting, WorldPosition, BoundingBox>(
    [&](
      entityx::Entity entity,
      const DamageInflicting&,
      const WorldPosition& position,
      const BoundingBox& bbox
    ) {
      inflictors.push_back({entity, engine::toWorldSpace(bbox, position)});
    });

  auto sortIndex = [](entityx::Entity entity) {
    if (entity.has_component<SlotIndex>()) {
      return entity.component<SlotIndex>()->mIndex;
    }

    return -1;
  };

  std::stable_sort(std::begin(inflictors), std::end(inflictors),
    [&](const auto& lhs, const auto& rhs) {
      return sortIndex(std::get<0>(lhs)) < sortIndex(std::get<0>(rhs));
    });

  mPerFrameState = s;

  es.each<BehaviorController, Active>([&](
    entityx::Entity entity,
    BehaviorController& controller,
    const Active& active
  ) {
    controller.update(
      mDependencies,
      mGlobalState,
      active.mIsOnScreen,
      entity);

    if (
      entity &&
      entity.has_component<Shootable>() &&
      !entity.component<Shootable>()->mInvincible
    ) {
      updateDamageInfliction(entity, controller, inflictors);
    }
  });
}


void BehaviorControllerSystem::updateDamageInfliction(
  entityx::Entity shootableEntity,
  BehaviorController& controller,
  std::vector<std::tuple<entityx::Entity, BoundingBox>>& inflictors
) {
  for (auto& [inflictorEntity, inflictorBbox] : inflictors) {
    const auto shootableBbox = engine::toWorldSpace(
      *shootableEntity.component<BoundingBox>(),
      *shootableEntity.component<WorldPosition>());

    if (shootableBbox.intersects(inflictorBbox)) {
      inflictDamage(inflictorEntity, shootableEntity, controller);
      break;
    }
  }
}


void BehaviorControllerSystem::inflictDamage(
  entityx::Entity inflictorEntity,
  entityx::Entity shootableEntity,
  BehaviorController& controller
) {
  auto& damage = *inflictorEntity.component<DamageInflicting>();
  auto& shootable = *shootableEntity.component<Shootable>();
  const auto inflictorVelocity = extractVelocity(inflictorEntity);

  damage.mHasCausedDamage = true;

  shootable.mHealth -= damage.mAmount;
  if (shootable.mHealth <= 0) {
    controller.onKilled(
      mDependencies,
      mGlobalState,
      inflictorVelocity,
      shootableEntity);
    mDependencies.mpEvents->emit(
      events::ShootableKilled{shootableEntity, inflictorVelocity});
    // Event listeners mustn't remove the shootable component
    assert(shootableEntity.has_component<Shootable>());

    mGlobalState.mpPlayer->model().giveScore(shootable.mGivenScore);

    if (shootable.mDestroyWhenKilled) {
      shootableEntity.destroy();
    } else {
      shootableEntity.remove<Shootable>();
    }
  } else {
    controller.onHit(
      mDependencies,
      mGlobalState,
      inflictorVelocity,
      shootableEntity);
    mDependencies.mpEvents->emit(
      events::ShootableDamaged{shootableEntity, inflictorVelocity});

    if (shootable.mEnableHitFeedback) {
      mDependencies.mpServiceProvider->playSound(data::SoundId::EnemyHit);

      if (shootableEntity.has_component<Sprite>()) {
        shootableEntity.component<Sprite>()->flashWhite();
      }
    }
  }
}


void BehaviorControllerSystem::receive(
  const engine::events::CollidedWithWorld& event
) {
  using engine::components::Active;
  using game_logic::components::BehaviorController;

  auto entity = event.mEntity;
  if (
    entity.has_component<BehaviorController>() &&
    entity.has_component<Active>()
  ) {
    entity.component<BehaviorController>()->onCollision(
      mDependencies,
      mGlobalState,
      event,
      entity);
  }
}

}
