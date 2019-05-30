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

#include "rigelatin_soldier.hpp"

#include "base/match.hpp"
#include "base/math_tools.hpp"
#include "engine/base_components.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/physical_components.hpp"
#include "engine/sprite_tools.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/entity_factory.hpp"
#include "game_logic/player.hpp"


namespace rigel { namespace game_logic { namespace behaviors {

namespace {

constexpr auto FLY_SPEED = 2;

constexpr base::Point<float> JUMP_ARC[] = {
  {0.0f, -2.0f},
  {0.0f, -2.0f},
  {0.0f, -1.0f},
  {0.0f,  0.0f},
};

}


void RigelatinSoldier::update(
  GlobalDependencies& d,
  GlobalState& s,
  const bool isOnScreen,
  entityx::Entity entity
) {
  using rigelatin_soldier::Ready;
  using rigelatin_soldier::Jumping;
  using rigelatin_soldier::Waiting;
  using namespace engine::components;

  const auto& position = *entity.component<WorldPosition>();

  auto& orientation = *entity.component<Orientation>();
  auto& movingBody = *entity.component<MovingBody>();
  auto& animationFrame = entity.component<Sprite>()->mFramesToRender[0];

  base::match(mState,
    [&, this](const Ready&) {
      updateReadyState(d, s, entity);
    },

    [&, this](Jumping& state) {
      ++state.mFramesElapsed;
      if (state.mFramesElapsed == 1) {
        engine::reassign<MovementSequence>(entity, JUMP_ARC, true, false);
      } else if (state.mFramesElapsed == 4) {
        movingBody.mGravityAffected = true;
        animationFrame = 2;
      }

      state.mPreviousPosX = position.x;

      const auto movement = engine::orientation::toMovement(orientation);
      state.mLastHorizontalMovementResult = engine::moveHorizontally(
        *d.mpCollisionChecker, entity, movement * FLY_SPEED);
    },

    [&, this](Waiting& state) {
      ++state.mFramesElapsed;

      if (state.mFramesElapsed == 4) {
        // Reset previously set "attack" animation state
        animationFrame = 0;
      } else if (state.mFramesElapsed == 20) {
        mState = Ready{};
      }
    }
  );

  engine::synchronizeBoundingBoxToSprite(entity);
}


void RigelatinSoldier::onCollision(
  GlobalDependencies& d,
  GlobalState& s,
  const engine::events::CollidedWithWorld& event,
  entityx::Entity entity
) {
  using rigelatin_soldier::Jumping;
  using engine::MovementResult;
  using namespace engine::components;

  if (!event.mCollidedBottom) {
    return;
  }

  auto& position = *entity.component<WorldPosition>();

  base::match(mState,
    [&, this](const Jumping& state) {
      // In RigelatinSoldier::update(), we don't know if we are going to hit
      // the ground on the current frame or not, as the Physics update happens
      // after all behavior controllers have been updated. Therefore, we need
      // to check here if we did horizontal movement during update(), and
      // revert it if that's the case.
      if (state.mLastHorizontalMovementResult != MovementResult::Failed) {
        position.x = state.mPreviousPosX;
      }

      mState = rigelatin_soldier::Ready{};
      updateReadyState(d, s, entity);

      engine::synchronizeBoundingBoxToSprite(entity);
    },

    [](const auto&) {}
  );
}


void RigelatinSoldier::updateReadyState(
  GlobalDependencies& d,
  GlobalState& s,
  entityx::Entity entity
) {
  using rigelatin_soldier::Ready;
  using rigelatin_soldier::Jumping;
  using rigelatin_soldier::Waiting;
  using namespace engine::components;

  const auto& playerPos = s.mpPlayer->orientedPosition();
  const auto& position = *entity.component<WorldPosition>();

  auto& orientation = *entity.component<Orientation>();
  auto& movingBody = *entity.component<MovingBody>();
  auto& animationFrame = entity.component<Sprite>()->mFramesToRender[0];

  auto attack = [&]() {
    const auto facingLeft = orientation == Orientation::Left;
    const auto movement =
      facingLeft ? SpriteMovement::FlyLeft : SpriteMovement::FlyRight;
    const auto xOffset = facingLeft ? 0 : 4;

    auto projectile = spawnMovingEffectSprite(
      *d.mpEntityFactory, 300, movement, position + base::Vector{xOffset, -4});
    projectile.assign<components::PlayerDamaging>(1);

    animationFrame = 3;
    mState = Waiting{};
  };

  auto jump = [&]() {
    movingBody.mGravityAffected = false;
    animationFrame = 1;

    mState = Jumping{};
  };


  // Orient towards player
  orientation = position.x >= playerPos.x
    ? Orientation::Left
    : Orientation::Right;

  // Attack immediately, or decide between jumping and possibly attacking
  if (d.mpRandomGenerator->gen() % 2 == 0) {
    attack();
  } else {
    const auto adjustment = orientation == Orientation::Left ? -1 : 1;
    mDecisionCounter += adjustment;

    if (mDecisionCounter > 0 && mDecisionCounter < 6) {
      jump();
    } else {
      mDecisionCounter = base::clamp(mDecisionCounter, 1, 5);

      if (d.mpRandomGenerator->gen() % 2 != 0) {
        attack();
      }
    }
  }
}
}}}
