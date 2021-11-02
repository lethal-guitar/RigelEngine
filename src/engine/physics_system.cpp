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

#include "physics_system.hpp"

#include "engine/entity_tools.hpp"
#include "engine/physics.hpp"


namespace ex = entityx;


namespace rigel::engine
{

using components::BoundingBox;
using components::MovingBody;
using components::WorldPosition;


PhysicsSystem::PhysicsSystem(
  const engine::CollisionChecker* pCollisionChecker,
  const data::map::Map* pMap,
  entityx::EventManager* pEvents)
  : mpCollisionChecker(pCollisionChecker)
  , mpMap(pMap)
  , mpEvents(pEvents)
{
  mpEvents->subscribe<ex::ComponentAddedEvent<MovingBody>>(*this);
  mpEvents->subscribe<ex::ComponentRemovedEvent<MovingBody>>(*this);
}


void PhysicsSystem::update(ex::EntityManager& es)
{
  es.each<MovingBody, WorldPosition, BoundingBox, components::Active>(
    [this](
      ex::Entity entity,
      MovingBody& body,
      WorldPosition& position,
      const BoundingBox& collisionRect,
      const components::Active&) {
      applyPhysics(entity, body, position, collisionRect);
    });
}


void PhysicsSystem::updatePhase1(ex::EntityManager& es)
{
  update(es);
  mShouldCollectForPhase2 = true;
}


void PhysicsSystem::updatePhase2(ex::EntityManager& es)
{
  for (auto entity : mPhysicsObjectsForPhase2)
  {
    assert(entity.has_component<MovingBody>());
    const auto hasRequiredComponents = entity.has_component<WorldPosition>() &&
      entity.has_component<BoundingBox>() &&
      entity.has_component<components::Active>();

    if (hasRequiredComponents)
    {
      applyPhysics(
        entity,
        *entity.component<MovingBody>(),
        *entity.component<WorldPosition>(),
        *entity.component<BoundingBox>());
    }
  }

  mPhysicsObjectsForPhase2.clear();
  mShouldCollectForPhase2 = false;
}


void PhysicsSystem::applyPhysics(
  ex::Entity entity,
  MovingBody& body,
  WorldPosition& position,
  const BoundingBox& collisionRect)
{
  if (!body.mIsActive)
  {
    return;
  }

  const auto result = engine::applyPhysics(
    *mpCollisionChecker, *mpMap, entity, body, position, collisionRect);

  setTag<components::CollidedWithWorld>(entity, result.has_value());

  if (result)
  {
    mpEvents->emit(events::CollidedWithWorld{
      entity, result->mLeft, result->mRight, result->mTop, result->mBottom});
  }
}


void PhysicsSystem::receive(
  const entityx::ComponentAddedEvent<components::MovingBody>& event)
{
  if (!mShouldCollectForPhase2)
  {
    return;
  }

  mPhysicsObjectsForPhase2.push_back(event.entity);
}


void PhysicsSystem::receive(
  const entityx::ComponentRemovedEvent<components::MovingBody>& event)
{
  using namespace std;

  if (!mShouldCollectForPhase2)
  {
    return;
  }

  const auto it = find_if(
    begin(mPhysicsObjectsForPhase2),
    end(mPhysicsObjectsForPhase2),
    [&event](const auto& entity) { return entity == event.entity; });

  if (it != end(mPhysicsObjectsForPhase2))
  {
    mPhysicsObjectsForPhase2.erase(it);
  }
}

} // namespace rigel::engine
