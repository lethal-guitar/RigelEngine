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

#include "slime_blob.hpp"

#include "base/match.hpp"
#include "engine/collision_checker.hpp"
#include "engine/movement.hpp"
#include "engine/physical_components.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/behavior_controller.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/ientity_factory.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic::behaviors
{

namespace
{

const auto NUM_BREAK_ANIMATION_STEPS = 15;
const auto BREAK_ANIM_SPEED = 3; // frames between visible animation steps

const auto SLIME_BLOB_SPAWN_OFFSET = base::Vec2{2, 0};

const auto SPRITE_ORIENTATION_OFFSET = 5;
const auto WALKING_ON_GROUND_BASE_FRAME = 3;

const auto STRETCH_UP_ANIM_START = 10;
const auto CONTRACT_DOWN_ANIM_START = 12;
const auto CONTRACT_DOWN_ANIM_END = 10;
const auto IN_FLIGHT_ANIM_FRAME = 13;
const auto CONTRACT_UP_ANIM_START = 14;
const auto CONTRACT_UP_ANIM_END = 16;
const auto STRETCH_DOWN_ANIM_END = 14;

} // namespace


void SlimeContainer::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool,
  entityx::Entity entity)
{
  using namespace engine::components;
  using game_logic::components::BehaviorController;
  using game_logic::components::Shootable;

  auto& sprite = *entity.component<Sprite>();

  const auto stillIntact = entity.has_component<Shootable>();
  if (stillIntact)
  {
    // Animate slime blob inside
    sprite.mFramesToRender[2] = d.mpRandomGenerator->gen() % 2;
  }
  else
  {
    ++mBreakAnimationStep;
    const auto visibleFrame = mBreakAnimationStep / BREAK_ANIM_SPEED;
    sprite.mFramesToRender[0] = 2 + visibleFrame;

    if (mBreakAnimationStep >= NUM_BREAK_ANIMATION_STEPS)
    {
      const auto& position = *entity.component<WorldPosition>();
      d.mpEntityFactory->spawnActor(
        data::ActorID::Green_slime_blob, position + SLIME_BLOB_SPAWN_OFFSET);

      entity.remove<BoundingBox>();
      entity.remove<BehaviorController>();
    }
  }
}


void SlimeContainer::onKilled(
  GlobalDependencies& d,
  GlobalState& s,
  const base::Vec2f&,
  entityx::Entity entity)
{
  using namespace engine::components;

  auto& sprite = *entity.component<Sprite>();
  sprite.mFramesToRender[2] = engine::IGNORE_RENDER_SLOT;
  sprite.mFramesToRender[0] = 2;
  sprite.flashWhite();
}


void SlimeBlob::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool,
  entityx::Entity entity)
{
  using namespace engine::components;
  using namespace engine::orientation;

  auto toOffset = [](const Orientation orientation) {
    return orientation == Orientation::Right ? SPRITE_ORIENTATION_OFFSET : 0;
  };


  const auto& playerPosition = s.mpPlayer->orientedPosition();
  const auto& bbox = *entity.component<BoundingBox>();
  auto& position = *entity.component<WorldPosition>();
  auto& sprite = *entity.component<engine::components::Sprite>();

  base::match(
    mState,
    [&](OnGround& state) {
      // Animate walking
      state.mIsOddUpdate = !state.mIsOddUpdate;

      // clang-format off
      const auto newAnimFrame =
        WALKING_ON_GROUND_BASE_FRAME +
        (state.mIsOddUpdate ? 1 : 0) +
        toOffset(mOrientation);
      // clang-format on
      sprite.mFramesToRender[0] = newAnimFrame;

      // Decide if we should continue walking or change state
      const auto isFacingLeft = mOrientation == Orientation::Left;
      const auto movingTowardsPlayer =
        (isFacingLeft && position.x >= playerPosition.x) ||
        (!isFacingLeft && position.x <= playerPosition.x);

      if (movingTowardsPlayer)
      {
        if (newAnimFrame % 2 == 1)
        {
          const auto walkedSuccessfully =
            engine::walk(*d.mpCollisionChecker, entity, mOrientation);

          if (!walkedSuccessfully)
          {
            mState = Idle{};
          }
        }
      }
      else
      {
        mState = Idle{};
      }
    },


    [&](OnCeiling& state) {
      // Check if we are above the player, and fall back down if so
      if (position.x == playerPosition.x)
      {
        mState = Descending{};
        return;
      }

      // Animate
      state.mIsOddUpdate = !state.mIsOddUpdate;
      const auto playerIsRight = position.x <= playerPosition.x;
      const auto baseFrame = playerIsRight ? 19 : 17;
      sprite.mFramesToRender[0] = baseFrame + (state.mIsOddUpdate ? 1 : 0);

      // Move
      if (state.mIsOddUpdate)
      {
        const auto orientationForMovement =
          playerIsRight ? Orientation::Right : Orientation::Left;
        const auto walkedSuccessfully = engine::walkOnCeiling(
          *d.mpCollisionChecker, entity, orientationForMovement);

        if (!walkedSuccessfully)
        {
          sprite.mFramesToRender[0] -= 2;
          mState = Descending{};
        }
      }
    },


    [&](Idle& state) {
      // Randomly decide to fly up
      if ((d.mpRandomGenerator->gen() % 32) == 0)
      {
        mState = Ascending{};
        sprite.mFramesToRender[0] = STRETCH_UP_ANIM_START;
        return;
      }

      // Animate
      const auto newAnimFrame =
        d.mpRandomGenerator->gen() % 4 + toOffset(mOrientation);
      sprite.mFramesToRender[0] = newAnimFrame;

      // Wait until time-out elapsed
      ++state.mFramesElapsed;
      if (state.mFramesElapsed >= 10)
      {
        // Orient towards player and back to movement
        const auto newOrientation = position.x <= playerPosition.x
          ? Orientation::Right
          : Orientation::Left;
        mOrientation = newOrientation;

        mState = OnGround{};
      }
    },


    [&](Ascending& state) {
      auto& animationFrame = sprite.mFramesToRender[0];

      if (animationFrame < IN_FLIGHT_ANIM_FRAME)
      {
        // Animate getting ready to fly up (stretch upwards). Assumes we
        // previously set the animation frame to STRETCH_UP_ANIM_START
        ++animationFrame;
      }
      else if (animationFrame == IN_FLIGHT_ANIM_FRAME)
      {
        // Fly upwards
        const auto willCollide =
          d.mpCollisionChecker->isTouchingCeiling(position, bbox);
        if (willCollide)
        {
          animationFrame = CONTRACT_UP_ANIM_START;
        }

        // always move, even when colliding. This is ok because the next
        // anim frame has an offset which makes us not collide anymore.
        position.y -= 1;
      }
      else
      {
        // Animate arrival on ceiling (contract)
        ++animationFrame;
        if (animationFrame >= CONTRACT_UP_ANIM_END)
        {
          mState = OnCeiling{};
        }
      }
    },


    [&](Descending& state) {
      auto& animationFrame = sprite.mFramesToRender[0];

      if (animationFrame == IN_FLIGHT_ANIM_FRAME)
      {
        const auto offset = base::Vec2{0, 3};

        const auto willCollide =
          d.mpCollisionChecker->isOnSolidGround(position + offset, bbox);
        if (willCollide)
        {
          animationFrame = CONTRACT_DOWN_ANIM_START;
        }

        // always move, even when colliding. This is ok because the next
        // anim frame has an offset which makes us not collide anymore.
        position.y += 1;
      }
      else
      {
        if (animationFrame == STRETCH_DOWN_ANIM_END)
        {
          position.y += 1;
        }

        --animationFrame;
        if (animationFrame <= CONTRACT_DOWN_ANIM_END)
        {
          animationFrame = 0;
          mState = Idle{};
          mOrientation = Orientation::Left;
        }
      }
    });

  engine::synchronizeBoundingBoxToSprite(entity);
}

} // namespace rigel::game_logic::behaviors
