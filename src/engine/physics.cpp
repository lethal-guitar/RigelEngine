/* Copyright (C) 2021, Nikolai Wuttke. All rights reserved.
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

#include "physics.hpp"

#include "engine/collision_checker.hpp"
#include "engine/movement.hpp"


namespace ex = entityx;

namespace rigel::engine
{

using components::BoundingBox;
using components::MovementSequence;
using components::MovingBody;
using components::SolidBody;
using components::WorldPosition;


namespace
{

base::Vec2f
  updateMovementSequence(entityx::Entity entity, const base::Vec2f& velocity)
{
  auto& sequence = *entity.component<MovementSequence>();
  if (sequence.mCurrentStep >= sequence.mVelocites.size())
  {
    if (sequence.mResetVelocityAfterSequence)
    {
      const auto resetVelocity =
        sequence.mEnableX ? base::Vec2f{} : base::Vec2f{velocity.x, 0.0f};
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

} // namespace


// TODO: This is implemented here, but declared in physical_components.hpp.
// It would be cleaner to have a matching .cpp file for that file.
BoundingBox
  toWorldSpace(const BoundingBox& bbox, const base::Vec2& entityPosition)
{
  return bbox +
    base::Vec2(entityPosition.x, entityPosition.y - (bbox.size.height - 1));
}


std::optional<PhysicsCollisionInfo> applyPhysics(
  const CollisionChecker& collisionChecker,
  const data::map::Map& map,
  entityx::Entity entity,
  components::MovingBody& body,
  components::WorldPosition& position,
  const components::BoundingBox& collisionRect)
{
  auto hasActiveSequence = [&]() {
    return entity.has_component<MovementSequence>();
  };

  if (hasActiveSequence())
  {
    body.mVelocity = updateMovementSequence(entity, body.mVelocity);
  }

  const auto originalVelocity = body.mVelocity;
  const auto originalPosition = position;

  const auto movementX = static_cast<std::int16_t>(body.mVelocity.x);
  moveHorizontally(collisionChecker, entity, movementX);

  // Cache new world space BBox after applying horizontal movement
  // for the next steps
  const auto bbox = toWorldSpace(collisionRect, position);

  if (body.mGravityAffected && !hasActiveSequence())
  {
    // Unstick objects from ground that ended up stuck inside on the previous
    // frame. This is needed for item's released from boxes in mid-air, which
    // can sometimes end up stuck in the ground.
    // It also makes sloped conveyor belts in N7 work.
    //
    // We need to temporarily move the object's position, instead of simply
    // doing a check at `position - {0, 1}`, because the entity might be a
    // solid body and we would detect collision with the entity itself if
    // we didn't adjust the position.
    --position.y;
    if (!collisionChecker.isOnSolidGround(position, collisionRect))
    {
      ++position.y;
    }

    body.mVelocity.y = applyGravity(collisionChecker, bbox, body.mVelocity.y);

    applyConveyorBeltMotion(collisionChecker, map, entity);
  }

  const auto movementY = static_cast<std::int16_t>(body.mVelocity.y);
  const auto result = moveVertically(collisionChecker, entity, movementY);
  if (result != MovementResult::Completed)
  {
    body.mVelocity.y = 0.0f;
  }

  const auto targetPosition =
    originalPosition + WorldPosition{movementX, movementY};

  if (body.mIgnoreCollisions)
  {
    position = targetPosition;
    body.mVelocity = originalVelocity;
  }

  const auto collisionOccured = position != targetPosition;
  if (collisionOccured)
  {
    const auto left = targetPosition.x != position.x && movementX < 0;
    const auto right = targetPosition.x != position.x && movementX > 0;
    const auto top = targetPosition.y != position.y && movementY < 0;
    const auto bottom = targetPosition.y != position.y && movementY > 0;

    return PhysicsCollisionInfo{left, right, top, bottom};
  }

  return {};
}


float applyGravity(
  const CollisionChecker& collisionChecker,
  const components::BoundingBox& bbox,
  float currentVelocity)
{
  if (currentVelocity == 0.0f)
  {
    if (collisionChecker.isOnSolidGround(bbox))
    {
      return currentVelocity;
    }

    // We are floating - begin falling
    return 0.5f;
  }
  else
  {
    // Apply gravity to falling object until terminal velocity reached
    if (currentVelocity < 2.0f)
    {
      return currentVelocity + 0.5f;
    }
    else
    {
      return 2.0f;
    }
  }
}

} // namespace rigel::engine
