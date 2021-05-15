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

#include "big_green_cat.hpp"

#include "engine/base_components.hpp"
#include "engine/collision_checker.hpp"
#include "engine/movement.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/global_dependencies.hpp"


namespace rigel::game_logic::behaviors
{

namespace
{

const int ANIMATION_SEQUENCE[] = {0, 1, 2, 1};

constexpr auto MOVEMENT_SPEED = 2;

} // namespace


void BigGreenCat::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool isOnScreen,
  entityx::Entity entity)
{
  auto& animationFrame =
    entity.component<engine::components::Sprite>()->mFramesToRender[0];
  auto& orientation = *entity.component<engine::components::Orientation>();

  if (mWaitFramesRemaining != 0)
  {
    animationFrame = 0;
    --mWaitFramesRemaining;
    return;
  }

  ++mAnimationStep;
  mAnimationStep %= 4;

  const auto isTouchingGround = d.mpCollisionChecker->isOnSolidGround(
    *entity.component<engine::components::WorldPosition>(),
    *entity.component<engine::components::BoundingBox>());
  if (isTouchingGround)
  {
    animationFrame = ANIMATION_SEQUENCE[mAnimationStep];
  }
  else
  {
    animationFrame = 2;
  }

  const auto result = engine::moveHorizontallyWithStairStepping(
    *d.mpCollisionChecker,
    entity,
    MOVEMENT_SPEED * engine::orientation::toMovement(orientation));
  if (result != engine::MovementResult::Completed)
  {
    orientation = engine::orientation::opposite(orientation);
    mWaitFramesRemaining = FRAMES_TO_WAIT;
    mAnimationStep = 0;
  }
}

} // namespace rigel::game_logic::behaviors
