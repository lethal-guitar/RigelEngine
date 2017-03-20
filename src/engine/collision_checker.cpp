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

#include "collision_checker.hpp"

#include "data/map.hpp"
#include "engine/physical_components.hpp"


namespace rigel { namespace engine {

using namespace engine::components;


CollisionChecker::CollisionChecker(const data::map::Map* pMap)
  : mpMap(pMap)
{
}


bool CollisionChecker::walkEntity(entityx::Entity entity, const int amount) const {
  auto& position = *entity.component<WorldPosition>();
  const auto& bbox = *entity.component<BoundingBox>();

  const auto newPosition = position + base::Vector{amount, 0};

  const auto movingLeft = amount < 0;
  const auto xToTest = newPosition.x + (movingLeft ? 0 : bbox.size.width - 1);

  const auto stillOnSolidGround =
    mpMap->collisionData(xToTest, newPosition.y + 1).isSolidTop();

  // TODO: Unify this with the code in the physics system
  bool collidingWithWorld = false;
  for (int i = 0; i < bbox.size.height; ++i) {
    if (!mpMap->collisionData(xToTest, newPosition.y - i).isClear()) {
      collidingWithWorld = true;
      break;
    }
  }

  if (stillOnSolidGround && !collidingWithWorld) {
    position = newPosition;
    return true;
  }

  return false;
}


bool CollisionChecker::walkEntityOnCeiling(
  entityx::Entity entity,
  const int amount
) const {
  // TODO: Eliminate duplication with the regular walkEntity()
  auto& position = *entity.component<WorldPosition>();
  const auto& bbox = *entity.component<BoundingBox>();

  const auto newPosition = position + base::Vector{amount, 0};

  const auto movingLeft = amount < 0;
  const auto xToTest =
    newPosition.x +
    (movingLeft ? 0 : bbox.size.width - 1) +
    bbox.topLeft.x;
  const auto yToTest =
    newPosition.y +
    bbox.topLeft.y -
    (bbox.size.height - 1);

  const auto xOffset = bbox.size.width * amount;
  const auto offset = base::Vector{xOffset, 0};
  const auto stillOnCeiling = isTouchingCeiling(position + offset, bbox);

  // TODO: Unify this with the code in the physics system
  bool collidingWithWorld = false;
  for (int i = 0; i < bbox.size.height; ++i) {
    if (!mpMap->collisionData(xToTest, yToTest + i).isClear()) {
      collidingWithWorld = true;
      break;
    }
  }

  if (stillOnCeiling && !collidingWithWorld) {
    position = newPosition;
    return true;
  }

  return false;
}

bool CollisionChecker::isOnSolidGround(
  const WorldPosition& position,
  const BoundingBox& bbox
) const {
  const auto worldSpaceBbox = engine::toWorldSpace(bbox, position);

  const auto y = worldSpaceBbox.bottom() + 1;
  const auto startX = worldSpaceBbox.left();
  const auto endX = worldSpaceBbox.right();
  for (int x = startX; x <= endX; ++x) {
    if (mpMap->collisionData(x, y).isSolidTop()) {
      return true;
    }
  }

  return false;
}


bool CollisionChecker::isTouchingCeiling(
  const WorldPosition& position,
  const BoundingBox& bbox
) const {
  const auto worldSpaceBbox = engine::toWorldSpace(bbox, position);

  const auto y = worldSpaceBbox.top() - 1;
  const auto startX = worldSpaceBbox.left();
  const auto endX = worldSpaceBbox.right();
  for (int x = startX; x <= endX; ++x) {
    if (mpMap->collisionData(x, y).isSolidBottom()) {
      return true;
    }
  }

  return false;
}

}}
