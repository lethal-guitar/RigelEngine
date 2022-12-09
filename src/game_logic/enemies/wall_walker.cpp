/* Copyright (C) 2019, Nikolai Wuttke. All rights reserved.
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

#include "wall_walker.hpp"

#include "engine/collision_checker.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/global_dependencies.hpp"


namespace rigel::game_logic::behaviors
{

static_assert(static_cast<int>(WallWalker::Direction::Up) == 0);
static_assert(static_cast<int>(WallWalker::Direction::Down) == 1);
static_assert(static_cast<int>(WallWalker::Direction::Left) == 2);
static_assert(static_cast<int>(WallWalker::Direction::Right) == 3);

WallWalker::WallWalker(engine::RandomNumberGenerator& rng)
  : mDirection(static_cast<Direction>(rng.gen() % 4))
{
}


void WallWalker::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool isOnScreen,
  entityx::Entity entity)
{
  auto& animationFrame =
    entity.component<engine::components::Sprite>()->mFramesToRender[0];
  auto& position = *entity.component<engine::components::WorldPosition>();
  const auto& bbox = *entity.component<engine::components::BoundingBox>();

  mShouldSkipThisFrame = !mShouldSkipThisFrame;
  if (mShouldSkipThisFrame)
  {
    return;
  }

  mMovementToggle = !mMovementToggle;

  if (mFramesUntilDirectionSwitch != 0)
  {
    --mFramesUntilDirectionSwitch;
  }

  switch (mDirection)
  {
    case Direction::Up:
      animationFrame = mMovementToggle * 2;
      if (mMovementToggle != 0)
      {
        position.y--;
      }
      break;

    case Direction::Down:
      animationFrame = mMovementToggle * 2;
      if (mMovementToggle == 0)
      {
        position.y++;
      }
      break;

    case Direction::Left:
      animationFrame = mMovementToggle;
      if (mMovementToggle == 0)
      {
        position.x--;
      }
      break;

    case Direction::Right:
      animationFrame = mMovementToggle;
      if (mMovementToggle != 0)
      {
        position.x++;
      }
      break;
  }

  engine::synchronizeBoundingBoxToSprite(entity);

  auto applyCollision = [&]() {
    switch (mDirection)
    {
      case Direction::Up:
        if (d.mpCollisionChecker->isTouchingCeiling(
              position + base::Vec2{0, 1}, bbox))
        {
          position.y++;
          return true;
        }
        break;

      case Direction::Down:
        if (d.mpCollisionChecker->isOnSolidGround(
              position + base::Vec2{0, -1}, bbox))
        {
          position.y--;
          return true;
        }
        break;

      case Direction::Left:
        if (
          d.mpCollisionChecker->isTouchingLeftWall(
            position + base::Vec2{1, 0}, bbox) ||
          !d.mpCollisionChecker->isOnSolidGround(
            position + base::Vec2{-2, 0}, bbox))
        {
          position.x++;
          return true;
        }
        break;

      case Direction::Right:
        if (
          d.mpCollisionChecker->isTouchingRightWall(
            position + base::Vec2{-1, 0}, bbox) ||
          !d.mpCollisionChecker->isOnSolidGround(
            position + base::Vec2{2, 0}, bbox))
        {
          position.x--;
          return true;
        }
        break;
    }

    return false;
  };


  while (applyCollision() || mFramesUntilDirectionSwitch == 0)
  {
    const auto newDirectionChoice = d.mpRandomGenerator->gen() % 2;
    if (mDirection == Direction::Up || mDirection == Direction::Down)
    {
      mDirection = newDirectionChoice == 1 ? Direction::Right : Direction::Left;
    }
    else
    {
      mDirection = newDirectionChoice == 1 ? Direction::Down : Direction::Up;
    }

    mFramesUntilDirectionSwitch = d.mpRandomGenerator->gen() % 32 + 10;
  }
}

} // namespace rigel::game_logic::behaviors
