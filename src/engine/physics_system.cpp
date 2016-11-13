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

#include <data/map.hpp>
#include <engine/base_components.hpp>


namespace ex = entityx;


namespace rigel { namespace engine {

using namespace std;

using components::Physical;
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


PhysicsSystem::PhysicsSystem(const data::map::LevelData& level)
  : mCollisionData(level.mMap.width(), level.mMap.height())
{
  for (int row=0; row<level.mMap.height(); ++row) {
    for (int col=0; col<level.mMap.width(); ++col) {
      auto collisionData1 =
        level.mTileSet.mAttributes.collisionData(
          level.mMap.tileAt(0, col, row));
      auto collisionData2 =
        level.mTileSet.mAttributes.collisionData(
          level.mMap.tileAt(1, col, row));
      mCollisionData.setValueAt(
        col, row, CollisionData{collisionData1, collisionData2});
    }
  }
}


void PhysicsSystem::update(
  ex::EntityManager& es,
  ex::EventManager& events,
  ex::TimeDelta dt
) {
  if (!updateAndCheckIfDesiredTicksElapsed(mTimeStepper, 2, dt)) {
    return;
  }

  es.each<Physical, WorldPosition>(
    [this](
      ex::Entity,
      Physical& physical,
      WorldPosition& position
    ) {
      const auto movementX = static_cast<int16_t>(physical.mVelocity.x);
      if (movementX != 0) {
        position = applyHorizontalMovement(
          toWorldSpace(physical.mCollisionRect, position),
          position,
          movementX);
      }

      // Cache new world space BBox after applying horizontal movement
      // for the next steps
      const auto bbox = toWorldSpace(physical.mCollisionRect, position);

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
          bbox,
          position,
          physical.mVelocity.y,
          movementY);
      }
    });
}


const data::map::CollisionData& PhysicsSystem::worldAt(
  const int x, const int y
) const {
  static const auto emptyCollisionData = CollisionData{};
  return mCollisionData.valueAtWithDefault(x, y, emptyCollisionData);
}


base::Vector PhysicsSystem::applyHorizontalMovement(
  const BoundingBox& bbox,
  const base::Vector& currentPosition,
  const int16_t movementX
) const {
  base::Vector newPosition = currentPosition;
  newPosition.x += movementX;

  const auto movingRight = movementX > 0;
  const auto movementDirection = movingRight ? 1 : -1;
  const auto startX = currentPosition.x + movementDirection;
  const auto endX = newPosition.x + movementDirection;
  const auto xOffset = movingRight ? bbox.size.width-1u : 0u;
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

      const auto& enteredCell = worldAt(x, row);
      if (hasCollision(enteredCell)) {
        bool mustResolveCollision = true;

        const auto atBottomRow = row == bbox.bottomLeft().y;
        if (atBottomRow) {
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
            newPosition.y};
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
  const BoundingBox& bbox,
  const base::Vector& currentPosition,
  const float currentVelocity,
  const int16_t movementY
) const {
  base::Vector newPosition = currentPosition;
  newPosition.y += movementY;

  const auto movingDown = movementY > 0;
  const auto movementDirection = movingDown ? 1 : -1;
  const auto startY = currentPosition.y + movementDirection;
  const auto endY = newPosition.y + movementDirection;
  const auto yOffset = movingDown ? 0 : -(bbox.size.height - 1);
  auto hasCollision = [movingDown](const auto& cellData) {
    if (movingDown) {
      return cellData.isSolidTop();
    } else {
      return cellData.isSolidBottom();
    }
  };

  for (auto row=startY; row != endY; row += movementDirection) {
    for (auto col=bbox.topLeft.x; col<=bbox.bottomRight().x; ++col) {
      const auto y = row + yOffset;

      const auto& enteredCell = worldAt(col, y);
      if (hasCollision(enteredCell)) {
        newPosition.y = row - movementDirection;
        if (movingDown) {
          // For falling, we reset the Y velocity as soon as we hit the ground
          return make_tuple(
            newPosition,
            0.0f);
        } else {
          // For jumping, we begin falling early when we hit the ceiling
          return make_tuple(
            newPosition,
            1.0f);
        }
      }
    }
  }

  return make_tuple(newPosition, currentVelocity);
}

}}
