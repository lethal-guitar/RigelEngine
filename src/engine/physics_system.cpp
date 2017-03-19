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

#include "base/warnings.hpp"
#include "data/map.hpp"
#include "engine/base_components.hpp"
#include "engine/entity_tools.hpp"

RIGEL_DISABLE_WARNINGS
#include <boost/algorithm/cxx11/any_of.hpp>
RIGEL_RESTORE_WARNINGS

namespace ba = boost::algorithm;
namespace ex = entityx;


namespace rigel { namespace engine {

using namespace std;

using components::BoundingBox;
using components::Physical;
using components::SolidBody;
using components::WorldPosition;
using data::map::CollisionData;


BoundingBox toWorldSpace(
  const BoundingBox& bbox,
  const base::Vector& entityPosition
) {
  return bbox + base::Vector(
    entityPosition.x,
    entityPosition.y - (bbox.size.height - 1));
}


PhysicsSystem::PhysicsSystem(const data::map::Map* pMap)
  : mpMap(pMap)
{
}


void PhysicsSystem::update(
  ex::EntityManager& es,
  ex::EventManager& events,
  ex::TimeDelta dt
) {
  using components::CollidedWithWorld;

  mSolidBodies.clear();
  es.each<SolidBody, WorldPosition, BoundingBox>(
    [this](
      ex::Entity entity,
      const SolidBody&,
      const WorldPosition& pos,
      const BoundingBox& bbox
    ) {
      mSolidBodies.emplace_back(
        std::make_tuple(entity, toWorldSpace(bbox, pos)));
    });

  es.each<Physical, WorldPosition, BoundingBox, components::Active>(
    [this](
      ex::Entity entity,
      Physical& physical,
      WorldPosition& position,
      const BoundingBox& collisionRect,
      const components::Active&
    ) {
      const auto originalPosition = position;

      const auto movementX = static_cast<int16_t>(physical.mVelocity.x);
      if (movementX != 0) {
        position= applyHorizontalMovement(
          entity,
          toWorldSpace(collisionRect, position),
          position,
          movementX,
          physical.mCanStepUpStairs);
      }

      // Cache new world space BBox after applying horizontal movement
      // for the next steps
      const auto bbox = toWorldSpace(collisionRect, position);

      // Apply Gravity after horizontal movement, but before vertical
      // movement. This is so that if the horizontal movement results in the
      // entity floating in the air, we want to drop down already in the same
      // frame where we applied the horizontal movement. Changing the velocity
      // here will automatically move the entity down when doing the vertical
      // movement.
      if (physical.mGravityAffected) {
        physical.mVelocity.y = applyGravity(bbox, physical.mVelocity.y);
      }

      const auto movementY = static_cast<std::int16_t>(physical.mVelocity.y);
      if (movementY != 0) {
        std::tie(position, physical.mVelocity.y) = applyVerticalMovement(
          entity,
          bbox,
          position,
          physical.mVelocity.y,
          movementY,
          physical.mGravityAffected);
      }

      const auto collisionOccured =
        position != originalPosition + WorldPosition{movementX, movementY};
      setTag<CollidedWithWorld>(entity, collisionOccured);
    });
}


data::map::CollisionData PhysicsSystem::worldAt(
  const int x, const int y
) const {
  return mpMap->collisionData(x, y);
}


base::Vector PhysicsSystem::applyHorizontalMovement(
  entityx::Entity entity,
  const BoundingBox& bbox,
  const base::Vector& currentPosition,
  const int16_t movementX,
  const bool allowStairStepping
) const {
  base::Vector newPosition = currentPosition;
  newPosition.x += movementX;

  const auto movingRight = movementX > 0;
  const auto movementDirection = movingRight ? 1 : -1;
  const auto startX = currentPosition.x + movementDirection;
  const auto endX = newPosition.x + movementDirection;
  const auto boundingBoxOffsetX = bbox.left() - currentPosition.x;
  const auto xOffset =
    (movingRight ? bbox.size.width-1u : 0u) +
    boundingBoxOffsetX;
  auto hasCollision = [movingRight](const auto& cellData) {
    if (movingRight) {
      return cellData.isSolidLeft();
    } else {
      return cellData.isSolidRight();
    }
  };

  // Check each cell between old and new position along the movement
  // direction for collisions. If one is found, stop and use the cell before
  // that as new x value.
  for (auto row=bbox.topLeft.y; row<=bbox.bottomLeft().y; ++row) {
    for (auto col=startX; col != endX; col += movementDirection) {
      const auto x = col + xOffset;

      auto transformedBbox = bbox;
      transformedBbox.topLeft.x = col;
      const auto collidesWithSolidBody = hasSolidBodyCollision(
        entity, transformedBbox);

      const auto& enteredCell = worldAt(x, row);
      if (collidesWithSolidBody || hasCollision(enteredCell)) {
        bool mustResolveCollision = true;

        const auto atBottomRow = row == bbox.bottomLeft().y;
        if (atBottomRow && allowStairStepping) {
          // Collision happened at bottom row, check if we can step up
          // a stair
          const auto& stairStepUp = worldAt(x, row-1);
          if (stairStepUp.isClear() && enteredCell.isSolidTop()) {
            // Yes, step up
            newPosition.y -= 1;
            mustResolveCollision = false;
          }
        }

        if (mustResolveCollision) {
          return {
            static_cast<uint16_t>(col - movementDirection),
            newPosition.y
          };
        }
      }
    }
  }

  return newPosition;
}


float PhysicsSystem::applyGravity(
  const BoundingBox& bbox,
  const float currentVelocity
) {
  if (currentVelocity == 0.0f) {
    // Check if we are still on solid ground

    for (auto x=bbox.bottomLeft().x; x<bbox.bottomRight().x; ++x) {
      if (worldAt(x, bbox.bottomLeft().y+1).isSolidTop()) {
        // Standing on solid ground - all good
        return currentVelocity;
      }
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


std::tuple<base::Vector, float> PhysicsSystem::applyVerticalMovement(
  entityx::Entity entity,
  const BoundingBox& bbox,
  const base::Vector& currentPosition,
  const float currentVelocity,
  const int16_t movementY,
  const bool beginFallingOnHittingCeiling
) const {
  base::Vector newPosition = currentPosition;
  newPosition.y += movementY;

  const auto movingDown = movementY > 0;
  const auto movementDirection = movingDown ? 1 : -1;
  const auto startY = currentPosition.y + movementDirection;
  const auto endY = newPosition.y + movementDirection;
  const auto boundingBoxOffsetY = bbox.bottom() - currentPosition.y;
  const auto yOffset =
    (movingDown ? 0 : -(bbox.size.height - 1)) +
    boundingBoxOffsetY;
  auto hasCollision = [movingDown](const auto& cellData) {
    if (movingDown) {
      return cellData.isSolidTop();
    } else {
      return cellData.isSolidBottom();
    }
  };

  auto transformedBbox = bbox;
  for (auto row=startY; row != endY; row += movementDirection) {
    transformedBbox.topLeft.y += movementDirection;

    for (auto col=bbox.topLeft.x; col<=bbox.bottomRight().x; ++col) {
      const auto y = row + yOffset;
      const auto& enteredCell = worldAt(col, y);
      if (
        hasSolidBodyCollision(entity, transformedBbox) ||
        hasCollision(enteredCell)
      ) {
        newPosition.y = row - movementDirection;
        if (movingDown || !beginFallingOnHittingCeiling) {
          // For falling, we reset the Y velocity as soon as we hit the ground
          return make_tuple(newPosition, 0.0f);
        } else {
          // For jumping, we begin falling early when we hit the ceiling
          return make_tuple(newPosition, 1.0f);
        }
      }
    }
  }

  return make_tuple(newPosition, currentVelocity);
}


bool PhysicsSystem::hasSolidBodyCollision(
  entityx::Entity entity,
  const BoundingBox& bbox
) const {
  return ba::any_of(mSolidBodies, [&bbox, &entity](const auto& solidBodyInfo) {
    return
      entity != std::get<0>(solidBodyInfo) &&
      bbox.intersects(std::get<1>(solidBodyInfo));
  });
}

}}
