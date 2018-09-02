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

#include "engine/collision_checker.hpp"
#include "engine/entity_tools.hpp"
#include "engine/movement.hpp"

namespace ex = entityx;


namespace rigel { namespace engine {

using namespace std;

using components::BoundingBox;
using components::MovementSequence;
using components::MovingBody;
using components::SolidBody;
using components::WorldPosition;


namespace {

base::Point<float> updateMovementSequence(
  entityx::Entity entity,
  const base::Point<float>& velocity
) {
  auto& sequence = *entity.component<MovementSequence>();
  if (sequence.mCurrentStep >= sequence.mVelocites.size()) {
    if (sequence.mResetVelocityAfterSequence) {
      const auto resetVelocity = sequence.mEnableX ?
        base::Point<float>{} :
        base::Point<float>{velocity.x, 0.0f};
      entity.remove<MovementSequence>();
      return resetVelocity;
    }

    entity.remove<MovementSequence>();
    return velocity;
  }

  auto newVelocity = sequence.mVelocites[sequence.mCurrentStep];
  ++sequence.mCurrentStep;

  return {sequence.mEnableX ? newVelocity.x : velocity.x, newVelocity.y};
}

}


// TODO: This is implemented here, but declared in physical_components.hpp.
// It would be cleaner to have a matching .cpp file for that file.
BoundingBox toWorldSpace(
  const BoundingBox& bbox,
  const base::Vector& entityPosition
) {
  return bbox + base::Vector(
    entityPosition.x,
    entityPosition.y - (bbox.size.height - 1));
}


PhysicsSystem::PhysicsSystem(
  const engine::CollisionChecker* pCollisionChecker,
  entityx::EventManager* pEvents
)
  : mpCollisionChecker(pCollisionChecker)
  , mpEvents(pEvents)
{
}


void PhysicsSystem::update(ex::EntityManager& es) {
  es.each<MovingBody, WorldPosition, BoundingBox, components::Active>(
    [this](
      ex::Entity entity,
      MovingBody& body,
      WorldPosition& position,
      const BoundingBox& collisionRect,
      const components::Active&
    ) {
      if (!body.mIsActive) {
        return;
      }

      const auto hasActiveSequence = entity.has_component<MovementSequence>();
      if (hasActiveSequence) {
        body.mVelocity = updateMovementSequence(entity, body.mVelocity);
      }

      const auto originalVelocity = body.mVelocity;
      const auto originalPosition = position;

      const auto movementX = static_cast<std::int16_t>(body.mVelocity.x);
      moveHorizontally(*mpCollisionChecker, entity, movementX);

      // Cache new world space BBox after applying horizontal movement
      // for the next steps
      const auto bbox = toWorldSpace(collisionRect, position);

      // Apply Gravity after horizontal movement, but before vertical
      // movement. This is so that if the horizontal movement results in the
      // entity floating in the air, we want to drop down already in the same
      // frame where we applied the horizontal movement. Changing the velocity
      // here will automatically move the entity down when doing the vertical
      // movement.
      if (body.mGravityAffected && !hasActiveSequence) {
        body.mVelocity.y = applyGravity(bbox, body.mVelocity.y);
      }

      const auto movementY = static_cast<std::int16_t>(body.mVelocity.y);
      const auto result =
        moveVertically(*mpCollisionChecker, entity, movementY);

      if (result != MovementResult::Completed) {
        const auto movingDown = movementY > 0;
        if (movingDown || !body.mGravityAffected) {
          // For falling, we reset the Y velocity as soon as we hit the ground
          body.mVelocity.y = 0.0f;
        } else {
          // For jumping, we begin falling early when we hit the ceiling
          body.mVelocity.y = 1.0f;
        }
      }

      const auto targetPosition =
        originalPosition + WorldPosition{movementX, movementY};
      const auto collisionOccured = position != targetPosition;
      setTag<components::CollidedWithWorld>(entity, collisionOccured);

      if (collisionOccured) {
        const auto left = targetPosition.x != position.x && movementX < 0;
        const auto right = targetPosition.x != position.x && movementX > 0;
        const auto top = targetPosition.y != position.y && movementY < 0;
        const auto bottom = targetPosition.y != position.y && movementY > 0;

        mpEvents->emit(events::CollidedWithWorld{
          entity, left, right, top, bottom});
      }

      if (body.mIgnoreCollisions) {
        position = targetPosition;
        body.mVelocity = originalVelocity;
      }
    });
}


float PhysicsSystem::applyGravity(
  const BoundingBox& bbox,
  const float currentVelocity
) {
  if (currentVelocity == 0.0f) {
    if (mpCollisionChecker->isOnSolidGround(bbox)) {
      return currentVelocity;
    }

    // We are floating - begin falling
    return 1.0f;
  } else {
    // Apply gravity to falling object until terminal velocity reached
    if (currentVelocity < 2.0f) {
      return currentVelocity + 0.56f;
    } else {
      return 2.0f;
    }
  }
}

}}
