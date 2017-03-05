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

#include "elevator.hpp"

#include "engine/entity_tools.hpp"
#include "engine/physical_components.hpp"
#include "game_logic/player/components.hpp"
#include "game_mode.hpp"

namespace ex = entityx;


namespace rigel { namespace game_logic { namespace interaction {

using engine::components::BoundingBox;
using engine::components::Physical;
using engine::components::SolidBody;
using engine::components::WorldPosition;
using rigel::game_logic::components::PlayerControlled;

namespace {

const auto ELEVATOR_SPEED = 2.0f;


int determineMovementDirection(const PlayerInputState& inputState) {
  if (inputState.mMovingUp) {
    return -1;
  } else if (inputState.mMovingDown) {
    return 1;
  }

  return 0;
}

}


void configureElevator(entityx::Entity entity) {
  using engine::components::ActivationSettings;

  entity.assign<components::Elevator>();
  entity.assign<BoundingBox>(BoundingBox{{0, 0}, {4, 3}});
  entity.assign<Physical>(base::Point<float>{0.0f, 0.0f}, true);
  entity.assign<ActivationSettings>(ActivationSettings::Policy::Always);
  entity.assign<SolidBody>();
}


ElevatorSystem::ElevatorSystem(
  entityx::Entity player,
  IGameServiceProvider* pServiceProvider
)
  : mPlayer(player)
  , mpServiceProvider(pServiceProvider)
  , mPreviousMovement(0)
{
}


void ElevatorSystem::update(
  ex::EntityManager& es,
  ex::EventManager& events,
  ex::TimeDelta dt
) {
  auto& playerState = *mPlayer.component<PlayerControlled>();
  auto& playerPhysical = *mPlayer.component<Physical>();

  // Reset player state back to default if currently attached. This makes sure
  // the player is in the right state if the following updateElevatorAttachment
  // detaches the player.
  if (mAttachedElevator) {
    playerState.mIsInteracting = false;
    playerPhysical.mGravityAffected = true;
  }

  updateElevatorAttachment(es, playerState);

  int movement = 0;
  if (mAttachedElevator) {
    movement = determineMovementDirection(mPlayerInputs);
    playerState.mIsInteracting = movement != 0;
    updateElevatorMovement(movement, playerPhysical);
  }

  mIsOddFrame = !mIsOddFrame;

  if (movement < 0 && mIsOddFrame) {
    mpServiceProvider->playSound(data::SoundId::FlameThrowerShot);
  }
}


bool ElevatorSystem::isPlayerAttached() const {
  return mAttachedElevator.valid();
}


void ElevatorSystem::setInputState(PlayerInputState inputState) {
  mPlayerInputs = inputState;
}


// TODO: This should be const, but it needs a const overload for
// the Entity::component() getter
ex::Entity ElevatorSystem::findAttachableElevator(ex::EntityManager& es) {
  ex::Entity attachableElevator;

  const auto& playerPos = *mPlayer.component<WorldPosition>();
  const auto& playerBox = *mPlayer.component<BoundingBox>();
  const auto leftAttachPoint = playerPos.x;
  const auto rightAttachPoint = playerPos.x + playerBox.size.width - 1;

  // Note: We don't use the elvator's bounding box to check if we can attach,
  // but hardcoded values. This is because the bounding box will be modified
  // during elevator movement, which would throw off the attachment check.
  // Making the attachment check independent of the actual bounding box means
  // we can be more straightforward when updating the elevator's state.
  ex::ComponentHandle<WorldPosition> position;
  ex::ComponentHandle<components::Elevator> tag;
  for (auto entity : es.entities_with_components(position, tag)) {
    const auto top = position->y - 2;
    const auto left = position->x;
    const auto right = left + 3;
    if (
      playerPos.y + 1 == top &&
      leftAttachPoint >= left &&
      rightAttachPoint <= right
    ) {
      attachableElevator = entity;
      break;
    }
  }

  return attachableElevator;
}


void ElevatorSystem::updateElevatorAttachment(
  ex::EntityManager& es,
  const PlayerControlled& playerState
) {
  // Find attachable elevator. If the player is about to jump, we don't
  // try to attach, since we would be detached one frame later anyway.
  auto attachableElevator = !playerState.mPerformedJump
    ? findAttachableElevator(es) : ex::Entity{};

  // Return previously attached elevator back to default state, if there is
  // one
  if (mAttachedElevator && attachableElevator != mAttachedElevator) {
    mAttachedElevator.component<BoundingBox>()->size.height = 3;
    mAttachedElevator.component<Physical>()->mVelocity.y = 2.0f;
    mAttachedElevator.component<Physical>()->mGravityAffected = true;
    engine::setTag<SolidBody>(mAttachedElevator, true);
    mAttachedElevator.invalidate();
  }

  if (attachableElevator) {
    mAttachedElevator = attachableElevator;
    mAttachedElevator.component<Physical>()->mGravityAffected = false;
    mPreviousMovement = 0;
  }
}


void ElevatorSystem::updateElevatorMovement(
  const int movement,
  Physical& playerPhysical
) {
  auto& elevatorPhysical = *mAttachedElevator.component<Physical>();

  mAttachedElevator.component<BoundingBox>()->size.height = 3;
  engine::setTag<SolidBody>(mAttachedElevator, true);

  playerPhysical.mVelocity.y = movement * ELEVATOR_SPEED;
  elevatorPhysical.mVelocity.y = movement * ELEVATOR_SPEED;

  if (movement != mPreviousMovement) {
    mPreviousMovement = movement;

    engine::setTag<SolidBody>(mAttachedElevator, movement == 0);
    playerPhysical.mGravityAffected = movement == 0;

    // Bounding box adjustment: In order to get the expected behavior at the
    // edges of the available movement space for the elevator, we extend the
    // player's bounding box to also cover the elevator's area when moving
    // down, and extend the elevator's bounding box to also cover the player
    // when moving up. Since the elevator is only a solid body while not
    // moving, this is not a problem. It guarantees that the player can't
    // keep moving when the elevator touches the ground, and that the
    // elevator doesn't move into the player when the player touches the
    // ceiling. Both of these cases would otherwise break, since the player
    // would get stuck in the elevator, or the elevator would move into the
    // player and then get detached.
    const auto elevatorBboxHeight = movement < 0 ? 8 : 3;
    mAttachedElevator.component<BoundingBox>()->size.height =
      elevatorBboxHeight;

    auto& playerBounds = *mPlayer.component<BoundingBox>();
    if (movement > 0) {
      playerBounds.size.height = 8;
      playerBounds.topLeft.y = 3;
    } else {
      // TODO: Assign player bounding box corresponding to state
      playerBounds.size.height = 5;
      playerBounds.topLeft.y = 0;
    }
  }
}

}}}
