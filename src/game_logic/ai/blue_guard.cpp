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

#include "engine/entity_tools.hpp"
#include "engine/life_time_components.hpp"
#include "engine/movement.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/entity_factory.hpp"
#include "game_logic/player.hpp"

#include "game_service_provider.hpp"

#include <boost/optional.hpp>


namespace rigel { namespace game_logic { namespace ai {

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
  components::BlueGuard& state,
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

  // TODO: Check player not cloaked, not riding elevator
  return
    player.stateIs<OnGround>() &&
    hasLineOfSightHorizontal &&
    hasLineOfSightVertical;
}


base::Vector offsetForShot(const components::BlueGuard& state) {
  const auto offsetY = state.mIsCrouched ? -1 : -2;
  const auto facingLeft = state.mOrientation == Orientation::Left;
  const auto offsetX = facingLeft ? -1 : 3;

  return {offsetX, offsetY};
}

}


BlueGuardSystem::BlueGuardSystem(
  const Player* pPlayer,
  CollisionChecker* pCollisionChecker,
  EntityFactory* pEntityFactory,
  IGameServiceProvider* pServiceProvider,
  engine::RandomNumberGenerator* pRandomGenerator,
  entityx::EventManager& events
)
  : mpPlayer(pPlayer)
  , mpCollisionChecker(pCollisionChecker)
  , mpEntityFactory(pEntityFactory)
  , mpServiceProvider(pServiceProvider)
  , mpRandomGenerator(pRandomGenerator)
{
  events.subscribe<events::ShootableDamaged>(*this);
}


void BlueGuardSystem::update(entityx::EntityManager& es) {
  es.each<components::BlueGuard, Sprite, WorldPosition, Active>(
    [this](
      entityx::Entity entity,
      components::BlueGuard& state,
      Sprite& sprite,
      WorldPosition& position,
      const Active&
    ) {
      if (state.mTypingOnTerminal) {
        const auto noticesPlayer =
          playerInNoticeableRange(position, mpPlayer->position());

        if (noticesPlayer) {
          stopTyping(state, sprite, position);
          updateGuard(entity, state, sprite, position);
        } else {
          // Animate typing on terminal
          const auto skipOneMove = (mpRandomGenerator->gen() & 4) != 0;
          const auto moveHand = mIsOddFrame && !skipOneMove;
          const auto typingFrameOffset = moveHand ? 1 : 0;

          sprite.mFramesToRender[0] = TYPING_BASE_FRAME + typingFrameOffset;
        }
      } else {
        updateGuard(entity, state, sprite, position);
      }

      engine::synchronizeBoundingBoxToSprite(entity);
    });

  mIsOddFrame = !mIsOddFrame;
}


void BlueGuardSystem::receive(const events::ShootableDamaged& event) {
  auto entity = event.mEntity;
  if (entity.has_component<components::BlueGuard>()) {
    auto& state = *entity.component<components::BlueGuard>();

    if (state.mTypingOnTerminal) {
      stopTyping(
        state,
        *entity.component<Sprite>(),
        *entity.component<WorldPosition>());
    }
  }
}


void BlueGuardSystem::stopTyping(
  components::BlueGuard& state,
  engine::components::Sprite& sprite,
  WorldPosition& position
) {
  state.mTypingOnTerminal = false;
  state.mOneStepWalkedSinceTypingStop = false;

  const auto playerX = mpPlayer->orientedPosition().x;
  state.mOrientation = position.x <= playerX
    ? Orientation::Right
    : Orientation::Left;

  const auto orientationOffset =
    state.mOrientation == Orientation::Left ? SPRITE_ORIENTATION_OFFSET : 0;
  sprite.mFramesToRender[0] = 0 + orientationOffset;
}


void BlueGuardSystem::updateGuard(
  entityx::Entity guardEntity,
  components::BlueGuard& state,
  engine::components::Sprite& sprite,
  WorldPosition& position
) {
  const auto walkOneStep = [this, &state, &guardEntity]() {
    return engine::walk(*mpCollisionChecker, guardEntity, state.mOrientation);
  };

  // If a guard was previously typing on a terminal, it will not attack the
  // player until after the next walked step, even if all other conditions are
  // fulfilled.
  const auto canAttack =
    state.mOneStepWalkedSinceTypingStop &&
    playerVisible(state, position, *mpPlayer);

  if (canAttack) {
    // Change stance if necessary
    if (state.mStanceChangeCountdown <= 0) {
      const auto playerCrouched = mpPlayer->isCrouching();
      const auto playerBelow = mpPlayer->position().y > position.y;
      state.mIsCrouched = playerCrouched || playerBelow;

      if (state.mIsCrouched) {
        state.mStanceChangeCountdown = mpRandomGenerator->gen() % 16;
      }
    } else {
      --state.mStanceChangeCountdown;
    }

    // Fire gun
    const auto facingLeft = state.mOrientation == Orientation::Left;
    const auto wantsToShoot = (mpRandomGenerator->gen() % 8) == 0;
    if (wantsToShoot && engine::isOnScreen(guardEntity)) {
      mpServiceProvider->playSound(data::SoundId::EnemyLaserShot);
      mpEntityFactory->createProjectile(
        ProjectileType::EnemyLaserShot,
        position + offsetForShot(state),
        facingLeft ? ProjectileDirection::Left : ProjectileDirection::Right);
    }

    // Update sprite
    if (wantsToShoot && !state.mIsCrouched) {
      // Show gun recoil animation in non-crouched state
      sprite.mFramesToRender[0] = facingLeft ? 15 : 14;
    } else {
      const auto animationFrame = state.mIsCrouched ? 5 : 4;
      const auto orientationOffset =
        facingLeft ? SPRITE_ORIENTATION_OFFSET : 0;
      sprite.mFramesToRender[0] = animationFrame + orientationOffset;
    }
  } else {
    state.mStanceChangeCountdown = 0;

    if (mIsOddFrame) {
      const auto walkedSuccessfully = walkOneStep();

      ++state.mStepsWalked;
      if (state.mStepsWalked >= 20 || !walkedSuccessfully) {
        state.mOrientation = opposite(state.mOrientation);

        // After changing orientation, walk one step in the new direction on
        // the same frame. The original code used a jump to accomplish this,
        // which means you can get into an infinite loop in the original game
        // by placing a blue guard in the right situation (no move possible).
        walkOneStep();
        state.mStepsWalked = 1;
      }

      state.mOneStepWalkedSinceTypingStop = true;
    }

    // Update sprite
    const auto walkAnimationFrame = state.mStepsWalked % 4;
    const auto orientationOffset =
      state.mOrientation == Orientation::Left ? SPRITE_ORIENTATION_OFFSET : 0;
    sprite.mFramesToRender[0] = walkAnimationFrame + orientationOffset;
  }
}

}}}
