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

#include "engine/physical_components.hpp"


namespace rigel { namespace engine {

namespace ex = entityx;

using data::map::CollisionData;
using data::map::SolidEdge;
using namespace engine::components;


CollisionChecker::CollisionChecker(const data::map::Map* pMap)
  : mpMap(pMap)
{
}


bool CollisionChecker::walkEntity(ex::Entity entity, const int amount) const {
  auto& position = *entity.component<WorldPosition>();
  const auto& bbox = *entity.component<BoundingBox>();

  const auto newPosition = position + base::Vector{amount, 0};
  const auto movingLeft = amount < 0;

  const auto xToTest = newPosition.x + (movingLeft ? 0 : bbox.size.width - 1);
  const auto stillOnSolidGround =
    isOnSolidGround({{xToTest, newPosition.y}, {1, 1}});

  const auto collidingWithWorld = movingLeft
    ? isTouchingLeftWall(position, bbox)
    : isTouchingRightWall(position, bbox);

  if (stillOnSolidGround && !collidingWithWorld) {
    position = newPosition;
    return true;
  }

  return false;
}


bool CollisionChecker::walkEntityOnCeiling(
  ex::Entity entity,
  const int amount
) const {
  // TODO: Eliminate duplication with the regular walkEntity()
  auto& position = *entity.component<WorldPosition>();
  const auto& bbox = *entity.component<BoundingBox>();

  const auto newPosition = position + base::Vector{amount, 0};
  const auto movingLeft = amount < 0;

  const auto xOffset = bbox.size.width * amount;
  const auto offset = base::Vector{xOffset, 0};
  const auto stillOnCeiling = isTouchingCeiling(position + offset, bbox);

  const auto collidingWithWorld = movingLeft
    ? isTouchingLeftWall(position, bbox)
    : isTouchingRightWall(position, bbox);

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
  return isOnSolidGround(worldSpaceBbox);
}


bool CollisionChecker::isTouchingCeiling(
  const WorldPosition& position,
  const BoundingBox& bbox
) const {
  const auto worldSpaceBbox = engine::toWorldSpace(bbox, position);
  return isTouchingCeiling(worldSpaceBbox);
}


bool CollisionChecker::isTouchingLeftWall(
  const WorldPosition& position,
  const BoundingBox& bbox
) const {
  const auto worldSpaceBbox = engine::toWorldSpace(bbox, position);
  return isTouchingLeftWall(worldSpaceBbox);
}


bool CollisionChecker::isTouchingRightWall(
  const WorldPosition& position,
  const BoundingBox& bbox
) const {
  const auto worldSpaceBbox = engine::toWorldSpace(bbox, position);
  return isTouchingRightWall(worldSpaceBbox);
}


bool CollisionChecker::testHorizontalSpan(
  const BoundingBox& bbox,
  const int y,
  const SolidEdge edge
) const {
  const auto startX = bbox.left();
  const auto endX = bbox.right();
  for (int x = startX; x <= endX; ++x) {
    if (mpMap->collisionData(x, y).isSolidOn(edge)) {
      return true;
    }
  }

  return false;
}


bool CollisionChecker::testVerticalSpan(
  const BoundingBox& bbox,
  const int x,
  const SolidEdge edge
) const {
  const auto startY = bbox.top();
  const auto endY = bbox.bottom();
  for (int y = startY; y <= endY; ++y) {
    if (mpMap->collisionData(x, y).isSolidOn(edge)) {
      return true;
    }
  }

  return false;
}


bool CollisionChecker::isTouchingCeiling(
  const BoundingBox& worldSpaceBbox
) const {
  const auto y = worldSpaceBbox.top() - 1;
  return testHorizontalSpan(worldSpaceBbox, y, SolidEdge::bottom());
}


bool CollisionChecker::isOnSolidGround(
  const BoundingBox& worldSpaceBbox
) const {
  const auto y = worldSpaceBbox.bottom() + 1;
  return testHorizontalSpan(worldSpaceBbox, y, SolidEdge::top());
}


bool CollisionChecker::isTouchingLeftWall(
  const BoundingBox& worldSpaceBbox
) const {
  const auto x = worldSpaceBbox.left() - 1;
  return testVerticalSpan(worldSpaceBbox, x, SolidEdge::right());
}


bool CollisionChecker::isTouchingRightWall(
  const BoundingBox& worldSpaceBbox
) const {
  const auto x = worldSpaceBbox.right() + 1;
  return testVerticalSpan(worldSpaceBbox, x, SolidEdge::left());
}

}}
