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

#include "blue_guard.hpp"

#include "common/game_service_provider.hpp"
#include "engine/entity_tools.hpp"
#include "engine/movement.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/ientity_factory.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/player.hpp"

#include <optional>


namespace rigel::game_logic::behaviors {

using namespace engine::components;
using namespace engine::orientation;
using engine::CollisionChecker;


namespace {

const auto SPRITE_ORIENTATION_OFFSET = 6;
const auto TYPING_BASE_FRAME = 12;
const auto GUARD_WIDTH = 3;


bool playerInNoticeableRange(
  const WorldPosition& myPosition,
  const WorldPosition& playerPosition
) {
  const auto playerCenterX = playerPosition.x + 1;
  const auto myCenterX = myPosition.x + GUARD_WIDTH/2;
  const auto centerToCenterDistance = std::abs(playerCenterX - myCenterX);

  return
    myPosition.y == playerPosition.y &&
    centerToCenterDistance <= 6;
}


bool playerVisible(
  BlueGuard& state,
  const WorldPosition& myPosition,
  const Player& player
) {
  const auto playerX = player.position().x;
  const auto playerY = player.position().y;
  const auto facingLeft = state.mOrientation == Orientation::Left;

  const auto hasLineOfSightHorizontal =
    (facingLeft && myPosition.x >= playerX) ||
    (!facingLeft && myPosition.x <= playerX);
  const auto hasLineOfSightVertical =
    playerY - 3 < myPosition.y &&
    playerY + 3 > myPosition.y;

  return
    player.isInRegularState() &&
    !player.isCloaked() &&
    hasLineOfSightHorizontal &&
    hasLineOfSightVertical;
}


base::Vector offsetForShot(const BlueGuard& state) {
  const auto offsetY = state.mIsCrouched ? -1 : -2;
  const auto facingLeft = state.mOrientation == Orientation::Left;
  const auto offsetX = facingLeft ? -1 : 3;

  return {offsetX, offsetY};
}

}


void BlueGuard::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool,
  entityx::Entity entity
) {
  const auto& position = *entity.component<WorldPosition>();
  auto& sprite = *entity.component<Sprite>();

  auto walkOneStep = [&]() {
    return engine::walk(*d.mpCollisionChecker, entity, mOrientation);
  };

  auto updatePatrolling = [&]() {
    // There is a bug in the original game which we replicate here. When a blue
    // guard is hit by the player while typing on a terminal, he will
    // immediately attack the player _only_ if the player is to the left of the
    // guard. Otherwise, the guard will walk one step first before attacking.
    //
    // This bug makes it quite easy to kill the guard protecting the key
    // card in level L1 without taking damage.
    const auto canAttack =
      playerVisible(*this, position, *s.mpPlayer) &&
      (!mTypingInterruptedByAttack || s.mpPlayer->position().x < position.x);
    mTypingInterruptedByAttack = false;

    if (canAttack) {
      // Change stance if necessary
      if (mStanceChangeCountdown <= 0) {
        const auto playerCrouched = s.mpPlayer->isCrouching();
        const auto playerBelow = s.mpPlayer->position().y > position.y;
        mIsCrouched = playerCrouched || playerBelow;

        if (mIsCrouched) {
          mStanceChangeCountdown = d.mpRandomGenerator->gen() % 16;
        }
      } else {
        --mStanceChangeCountdown;
      }

      // Fire gun
      const auto facingLeft = mOrientation == Orientation::Left;
      const auto wantsToShoot = (d.mpRandomGenerator->gen() % 8) == 0;
      if (wantsToShoot) {
        if (engine::isOnScreen(entity)) {
          d.mpServiceProvider->playSound(data::SoundId::EnemyLaserShot);
        }

        spawnEnemyLaserShot(
          *d.mpEntityFactory,
          position + offsetForShot(*this),
          mOrientation);
      }

      // Update sprite
      if (wantsToShoot && !mIsCrouched) {
        // Show gun recoil animation in non-crouched state
        sprite.mFramesToRender[0] = facingLeft ? 15 : 14;
      } else {
        const auto animationFrame = mIsCrouched ? 5 : 4;
        const auto orientationOffset =
          facingLeft ? SPRITE_ORIENTATION_OFFSET : 0;
        sprite.mFramesToRender[0] = animationFrame + orientationOffset;
      }
    } else {
      mStanceChangeCountdown = 0;

      if (s.mpPerFrameState->mIsOddFrame) {
        const auto walkedSuccessfully = walkOneStep();

        ++mStepsWalked;
        if (mStepsWalked >= 20 || !walkedSuccessfully) {
          mOrientation = opposite(mOrientation);

          // After changing orientation, walk one step in the new direction on
          // the same frame. The original code used a jump to accomplish this,
          // which means you can get into an infinite loop in the original game
          // by placing a blue guard in a situation where no move is possible.
          walkOneStep();
          mStepsWalked = 1;
        }
      }

      // Update sprite
      const auto walkAnimationFrame = mStepsWalked % 4;
      const auto orientationOffset =
        mOrientation == Orientation::Left ? SPRITE_ORIENTATION_OFFSET : 0;
      sprite.mFramesToRender[0] = walkAnimationFrame + orientationOffset;
    }
  };


  if (mTypingOnTerminal) {
    const auto noticesPlayer =
      playerInNoticeableRange(position, s.mpPlayer->position());

    if (noticesPlayer) {
      stopTyping(s, entity);
      updatePatrolling();
    } else {
      // Animate typing on terminal
      const auto skipOneMove = (d.mpRandomGenerator->gen() / 4) % 2 != 0;
      const auto moveHand = s.mpPerFrameState->mIsOddFrame && !skipOneMove;
      const auto typingFrameOffset = moveHand ? 1 : 0;

      sprite.mFramesToRender[0] = TYPING_BASE_FRAME + typingFrameOffset;
    }
  } else {
    updatePatrolling();
  }

  engine::synchronizeBoundingBoxToSprite(entity);
}


void BlueGuard::onHit(
  GlobalDependencies& d,
  GlobalState& s,
  const base::Point<float>& inflictorVelocity,
  entityx::Entity entity
) {
  if (mTypingOnTerminal) {
    stopTyping(s, entity);
    mTypingInterruptedByAttack = true;
  }
}


void BlueGuard::stopTyping(GlobalState& s, entityx::Entity entity) {
  mTypingOnTerminal = false;

  const auto& position = *entity.component<WorldPosition>();
  const auto playerX = s.mpPlayer->orientedPosition().x;
  mOrientation = position.x <= playerX
    ? Orientation::Right
    : Orientation::Left;
}

}
