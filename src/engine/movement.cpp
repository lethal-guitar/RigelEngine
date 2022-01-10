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

#include "base/math_utils.hpp"
#include "base/static_vector.hpp"
#include "data/map.hpp"
#include "engine/collision_checker.hpp"

#include <algorithm>


namespace rigel::engine
{

using namespace engine::components;


namespace
{

enum class ConveyorBeltFlag
{
  None,
  Left,
  Right
};


constexpr auto WALK_OFF_LEDGE_LEEWAY = 2;

constexpr auto MAX_WIDTH_FOR_CONVEYOR_CHECK = 16u;


template <typename CallableT>
MovementResult move(int* pPosition, const int amount, CallableT isColliding)
{
  if (amount == 0)
  {
    return MovementResult::Completed;
  }

  const auto desiredDistance = std::abs(amount);
  const auto movement = base::sgn(amount);

  const auto previousPosition = *pPosition;
  for (int i = 0; i < desiredDistance; ++i)
  {
    if (isColliding())
    {
      break;
    }

    *pPosition += movement;
  }

  const auto actualDistance = std::abs(*pPosition - previousPosition);
  if (actualDistance == 0)
  {
    return MovementResult::Failed;
  }

  return actualDistance == desiredDistance ? MovementResult::Completed
                                           : MovementResult::MovedPartially;
}


bool canWalkUpStairStep(
  const CollisionChecker& collisionChecker,
  entityx::Entity entity,
  const int movement)
{
  auto stairSteppedBbox = toWorldSpace(
    *entity.component<BoundingBox>(), *entity.component<WorldPosition>());
  stairSteppedBbox.topLeft.y -= 1;

  if (
    (movement < 0 && collisionChecker.isTouchingLeftWall(stairSteppedBbox)) ||
    (movement > 0 && collisionChecker.isTouchingRightWall(stairSteppedBbox)))
  {
    return false;
  }

  return true;
}

} // namespace


namespace ex = entityx;

bool walk(
  const CollisionChecker& collisionChecker,
  ex::Entity entity,
  const components::Orientation orientation)
{
  auto& position = *entity.component<WorldPosition>();
  const auto& bbox = *entity.component<BoundingBox>();

  const auto amount = orientation::toMovement(orientation);
  const auto newPosition = position + base::Vec2{amount, 0};
  const auto movingLeft = amount < 0;

  const auto xToTest =
    newPosition.x + WALK_OFF_LEDGE_LEEWAY * (movingLeft ? -1 : 1);
  const auto stillOnSolidGround = collisionChecker.isOnSolidGround(
    {{xToTest, newPosition.y}, {bbox.size.width, 1}});

  const auto collidingWithWorld = movingLeft
    ? collisionChecker.isTouchingLeftWall(position, bbox)
    : collisionChecker.isTouchingRightWall(position, bbox);

  if (stillOnSolidGround && !collidingWithWorld)
  {
    position = newPosition;
    return true;
  }

  return false;
}


bool walkOnCeiling(
  const CollisionChecker& collisionChecker,
  ex::Entity entity,
  const components::Orientation orientation)
{
  // TODO: Eliminate duplication with the regular walk()
  auto& position = *entity.component<WorldPosition>();
  const auto& bbox = *entity.component<BoundingBox>();

  const auto amount = orientation::toMovement(orientation);
  const auto newPosition = position + base::Vec2{amount, 0};
  const auto movingLeft = amount < 0;

  const auto xOffset = bbox.size.width * amount;
  const auto offset = base::Vec2{xOffset, 0};
  const auto stillOnCeiling =
    collisionChecker.isTouchingCeiling(position + offset, bbox);

  const auto collidingWithWorld = movingLeft
    ? collisionChecker.isTouchingLeftWall(position, bbox)
    : collisionChecker.isTouchingRightWall(position, bbox);

  if (stillOnCeiling && !collidingWithWorld)
  {
    position = newPosition;
    return true;
  }

  return false;
}


MovementResult moveHorizontally(
  const CollisionChecker& collisionChecker,
  ex::Entity entity,
  const int amount)
{
  auto& position = *entity.component<WorldPosition>();
  auto& bbox = *entity.component<BoundingBox>();

  return move(&position.x, amount, [&]() {
    return amount < 0 ? collisionChecker.isTouchingLeftWall(position, bbox)
                      : collisionChecker.isTouchingRightWall(position, bbox);
  });
}


MovementResult moveVertically(
  const CollisionChecker& collisionChecker,
  ex::Entity entity,
  const int amount)
{
  auto& position = *entity.component<WorldPosition>();
  auto& bbox = *entity.component<BoundingBox>();

  return move(&position.y, amount, [&]() {
    return amount < 0 ? collisionChecker.isTouchingCeiling(position, bbox)
                      : collisionChecker.isOnSolidGround(position, bbox);
  });
}


MovementResult moveHorizontallyWithStairStepping(
  const CollisionChecker& collisionChecker,
  ex::Entity entity,
  const int amount)
{
  if (amount == 0)
  {
    return MovementResult::Completed;
  }

  const auto desiredDistance = std::abs(amount);
  const auto step = base::sgn(amount);

  auto& position = *entity.component<WorldPosition>();

  MovementResult finalResult = MovementResult::Completed;
  for (int i = 0; i < desiredDistance; ++i)
  {
    const auto result = moveHorizontally(collisionChecker, entity, step);
    if (result != MovementResult::Completed)
    {
      if (canWalkUpStairStep(collisionChecker, entity, step))
      {
        position.x += step;
        position.y -= 1;
      }
      else
      {
        finalResult =
          i > 0 ? MovementResult::MovedPartially : MovementResult::Failed;
        break;
      }
    }
  }

  return finalResult;
}


int determineConveyorBeltMotionAmount(
  const CollisionChecker& collisionChecker,
  const data::map::Map& map,
  entityx::Entity entity)
{
  using std::any_of;
  using std::begin;
  using std::end;

  auto getFlag = [&map](const int x, const int y) {
    const auto attributes = map.attributes(x, y);
    if (attributes.isConveyorBeltLeft())
    {
      return ConveyorBeltFlag::Left;
    }
    else if (attributes.isConveyorBeltRight())
    {
      return ConveyorBeltFlag::Right;
    }
    else
    {
      return ConveyorBeltFlag::None;
    }
  };

  base::static_vector<ConveyorBeltFlag, MAX_WIDTH_FOR_CONVEYOR_CHECK> flags;

  {
    const auto& position = *entity.component<WorldPosition>();
    const auto& bbox = *entity.component<BoundingBox>();
    const auto worldBbox = toWorldSpace(bbox, position);
    for (auto x = 0; x < worldBbox.size.width; ++x)
    {
      flags.push_back(getFlag(worldBbox.left() + x, worldBbox.bottom() + 1));
    }
  }

  auto anyFlagIs = [&flags](const ConveyorBeltFlag desiredFlag) {
    return any_of(begin(flags), end(flags), [&](const ConveyorBeltFlag flag) {
      return flag == desiredFlag;
    });
  };

  if (anyFlagIs(ConveyorBeltFlag::Left))
  {
    return -1;
  }
  else if (anyFlagIs(ConveyorBeltFlag::Right))
  {
    return 1;
  }

  return 0;
}


void applyConveyorBeltMotion(
  const CollisionChecker& collisionChecker,
  const data::map::Map& map,
  entityx::Entity entity)
{
  const auto amount =
    determineConveyorBeltMotionAmount(collisionChecker, map, entity);
  moveHorizontally(collisionChecker, entity, amount);
}

} // namespace rigel::engine
