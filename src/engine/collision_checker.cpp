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

#include <algorithm>


namespace rigel::engine {

namespace ex = entityx;

using data::map::CollisionData;
using data::map::SolidEdge;
using namespace engine::components;


CollisionChecker::CollisionChecker(
  const data::map::Map* pMap,
  ex::EntityManager& entities,
  ex::EventManager& eventManager
)
  : mpMap(pMap)
{
  entities.each<SolidBody>([this](ex::Entity entity, const SolidBody&) {
    mSolidBodies.push_back(entity);
  });

  eventManager.subscribe<ex::ComponentAddedEvent<SolidBody>>(*this);
  eventManager.subscribe<ex::ComponentRemovedEvent<SolidBody>>(*this);
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
  auto bboxForSolidBodyTest = bbox;
  bboxForSolidBodyTest.topLeft.y = y;
  bboxForSolidBodyTest.size.height = 1;
  if (testSolidBodyCollision(bboxForSolidBodyTest)) {
    return true;
  }

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
  auto bboxForSolidBodyTest = bbox;
  bboxForSolidBodyTest.topLeft.x = x;
  bboxForSolidBodyTest.size.width = 1;
  if (testSolidBodyCollision(bboxForSolidBodyTest)) {
    return true;
  }

  const auto startY = bbox.top();
  const auto endY = bbox.bottom();
  for (int y = startY; y <= endY; ++y) {
    if (mpMap->collisionData(x, y).isSolidOn(edge)) {
      return true;
    }
  }

  return false;
}


bool CollisionChecker::testSolidBodyCollision(
  const BoundingBox& bboxToTest
) const {
  return any_of(begin(mSolidBodies), end(mSolidBodies),
    [&bboxToTest](const ex::Entity& entity) {
      if (
        entity.has_component<BoundingBox>() &&
        entity.has_component<WorldPosition>()
      ) {
        const auto solidBodyBbox = engine::toWorldSpace(
          *entity.component<const BoundingBox>(),
          *entity.component<const WorldPosition>());
        return solidBodyBbox.intersects(bboxToTest);
      }

      return false;
    });
}


bool CollisionChecker::isTouchingCeiling(
  const BoundingBox& worldSpaceBbox
) const {
  return testHorizontalSpan(
    worldSpaceBbox,
    worldSpaceBbox.top() - 1,
    SolidEdge::bottom());
}


bool CollisionChecker::isOnSolidGround(
  const BoundingBox& worldSpaceBbox
) const {
  return testHorizontalSpan(
    worldSpaceBbox,
    worldSpaceBbox.bottom() + 1,
    SolidEdge::top());
}


bool CollisionChecker::isTouchingLeftWall(
  const BoundingBox& worldSpaceBbox
) const {
  return testVerticalSpan(
    worldSpaceBbox,
    worldSpaceBbox.left() - 1,
    SolidEdge::right());
}


bool CollisionChecker::isTouchingRightWall(
  const BoundingBox& worldSpaceBbox
) const {
  return testVerticalSpan(
    worldSpaceBbox,
    worldSpaceBbox.right() + 1,
    SolidEdge::left());
}


void CollisionChecker::receive(
  const ex::ComponentAddedEvent<SolidBody>& event
) {
  mSolidBodies.push_back(event.entity);
}


void CollisionChecker::receive(
  const ex::ComponentRemovedEvent<SolidBody>& event
) {
  const auto it = find_if(begin(mSolidBodies), end(mSolidBodies),
    [&event](const auto& entity) {
      return entity == event.entity;
    });

  if (it != end(mSolidBodies)) {
    mSolidBodies.erase(it);
  }
}

}
