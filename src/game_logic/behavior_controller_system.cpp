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

#include "engine/base_components.hpp"
#include "engine/physical_components.hpp"
#include "game_logic/behavior_controller.hpp"

#include "global_level_events.hpp"


namespace rigel { namespace game_logic {

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
  mDependencies.mpEvents->subscribe<events::ShootableDamaged>(*this);
  mDependencies.mpEvents->subscribe<events::ShootableKilled>(*this);
  mDependencies.mpEvents->subscribe<engine::events::CollidedWithWorld>(*this);
  mDependencies.mpEvents->subscribe<rigel::events::EarthQuakeBegin>(*this);
  mDependencies.mpEvents->subscribe<rigel::events::EarthQuakeEnd>(*this);
}


void BehaviorControllerSystem::update(entityx::EntityManager& es) {
  using engine::components::Active;
  using game_logic::components::BehaviorController;

  es.each<BehaviorController, Active>([this](
    entityx::Entity entity,
    BehaviorController& controller,
    const Active& active
  ) {
    controller.update(
      mDependencies,
      mGlobalState,
      active.mIsOnScreen,
      entity);
  });

  mPerFrameState.mIsOddFrame = !mPerFrameState.mIsOddFrame;
}


void BehaviorControllerSystem::receive(const events::ShootableDamaged& event) {
  using engine::components::Active;
  using game_logic::components::BehaviorController;

  auto entity = event.mEntity;
  if (
    entity.has_component<BehaviorController>() &&
    entity.has_component<Active>()
  ) {
    entity.component<BehaviorController>()->onHit(
      mDependencies,
      mGlobalState,
      event.mInflictorVelocity,
      entity);
  }
}


void BehaviorControllerSystem::receive(const events::ShootableKilled& event) {
  using engine::components::Active;
  using game_logic::components::BehaviorController;

  auto entity = event.mEntity;
  if (
    entity.has_component<BehaviorController>() &&
    entity.has_component<Active>()
  ) {
    entity.component<BehaviorController>()->onKilled(
      mDependencies,
      mGlobalState,
      event.mInflictorVelocity,
      entity);
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
      entity);
  }
}


void BehaviorControllerSystem::receive(const rigel::events::EarthQuakeBegin&) {
  mPerFrameState.mIsEarthShaking = true;
}


void BehaviorControllerSystem::receive(const rigel::events::EarthQuakeEnd&) {
  mPerFrameState.mIsEarthShaking = false;
}

}}
