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

#include "movement.hpp"

#include "engine/collision_checker.hpp"

namespace rigel { namespace engine {

using namespace engine::components;


namespace ex = entityx;

bool walk(
  const CollisionChecker& collisionChecker,
  ex::Entity entity,
  const components::Orientation orientation
) {
  auto& position = *entity.component<WorldPosition>();
  const auto& bbox = *entity.component<BoundingBox>();

  const auto amount = orientation::toMovement(orientation);
  const auto newPosition = position + base::Vector{amount, 0};
  const auto movingLeft = amount < 0;

  const auto xToTest = newPosition.x + (movingLeft ? 0 : bbox.size.width - 1);
  const auto stillOnSolidGround =
    collisionChecker.isOnSolidGround({{xToTest, newPosition.y}, {1, 1}});

  const auto collidingWithWorld = movingLeft
    ? collisionChecker.isTouchingLeftWall(position, bbox)
    : collisionChecker.isTouchingRightWall(position, bbox);

  if (stillOnSolidGround && !collidingWithWorld) {
    position = newPosition;
    return true;
  }

  return false;
}


bool walkOnCeiling(
  const CollisionChecker& collisionChecker,
  ex::Entity entity,
  const components::Orientation orientation
) {
  // TODO: Eliminate duplication with the regular walk()
  auto& position = *entity.component<WorldPosition>();
  const auto& bbox = *entity.component<BoundingBox>();

  const auto amount = orientation::toMovement(orientation);
  const auto newPosition = position + base::Vector{amount, 0};
  const auto movingLeft = amount < 0;

  const auto xOffset = bbox.size.width * amount;
  const auto offset = base::Vector{xOffset, 0};
  const auto stillOnCeiling =
    collisionChecker.isTouchingCeiling(position + offset, bbox);

  const auto collidingWithWorld = movingLeft
    ? collisionChecker.isTouchingLeftWall(position, bbox)
    : collisionChecker.isTouchingRightWall(position, bbox);

  if (stillOnCeiling && !collidingWithWorld) {
    position = newPosition;
    return true;
  }

  return false;
}

}}
