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

#include "player_movement_system.hpp"

#include "data/map.hpp"
#include "engine/base_components.hpp"
#include "engine/physical_components.hpp"
#include "engine/visual_components.hpp"


namespace ex = entityx;


namespace rigel { namespace game_logic {

using namespace components;
using namespace rigel::engine::components;
using namespace player;
using namespace std;

using engine::TimeStepper;
using engine::updateAndCheckIfDesiredTicksElapsed;


void initializePlayerEntity(entityx::Entity player, const bool isFacingRight) {
  PlayerControlled controls;
  controls.mOrientation =
    isFacingRight ? Orientation::Right : Orientation::Left;
  auto sprite = player.component<Sprite>();
  if (sprite)
  {
    sprite->mFramesToRender = {isFacingRight ? 39 : 0};
  }
  player.assign<PlayerControlled>(controls);

  player.assign<Physical>(Physical{{0.0f, 0.0f}, true, true});
  player.assign<BoundingBox>(BoundingBox{{0, 0}, {3, 5}});
}


/* WARNING: PROTOTYPE CODE
 *
 * The current PlayerMovementSystem is a quick & dirty first prototype of the
 * player movement. It's not very easy to folow or maintainable. It will be
 * replaced by a new implementation as soon as more player movement features
 * will be implemented.
 *
 * TODO: Rewrite PlayerMovementSystem
 */


PlayerMovementSystem::PlayerMovementSystem(
  entityx::Entity player,
  const PlayerInputState* pInputs,
  const data::map::Map& map,
  const data::map::TileAttributes& tileAttributes
)
  : mpPlayerControlInput(pInputs)
  , mPlayer(player)
  , mLadderFlags(map.width(), map.height())
{
  for (int row=0; row<map.height(); ++row) {
    for (int col=0; col<map.width(); ++col) {
      const auto isLadder =
        tileAttributes.isLadder(map.tileAt(0, col, row)) ||
        tileAttributes.isLadder(map.tileAt(1, col, row));
      mLadderFlags.setValueAt(col, row, isLadder);
    }
  }
}


void PlayerMovementSystem::update(
  entityx::EntityManager& es,
  entityx::EventManager& events,
  entityx::TimeDelta dt
) {
  assert(mPlayer.has_component<PlayerControlled>());
  assert(mPlayer.has_component<Physical>());
  assert(mPlayer.has_component<WorldPosition>());

  const auto hasTicks =
    updateAndCheckIfDesiredTicksElapsed(mTimeStepper, 2, dt);

  auto& state = *mPlayer.component<PlayerControlled>().get();
  auto& physical = *mPlayer.component<Physical>().get();
  auto& boundingBox = *mPlayer.component<BoundingBox>().get();
  auto& worldPosition = *mPlayer.component<WorldPosition>().get();

  if (state.isPlayerDead()) {
    return;
  }

  auto movingLeft = mpPlayerControlInput->mMovingLeft;
  auto movingRight = mpPlayerControlInput->mMovingRight;
  auto movingUp = mpPlayerControlInput->mMovingUp;
  auto movingDown = mpPlayerControlInput->mMovingDown;
  auto jumping = mpPlayerControlInput->mJumping;

  // Filter out conflicting directional inputs
  if (movingLeft && movingRight) {
    movingLeft = movingRight = false;
  }
  if (movingUp && movingDown) {
    movingUp = movingDown = false;
  }

  const auto oldState = state.mState;
  auto horizontalMovementWanted = movingLeft || movingRight;
  auto verticalMovementWanted = movingUp || movingDown;

  auto worldSpacePlayerBounds = boundingBox;
  worldSpacePlayerBounds.topLeft += worldPosition;
  worldSpacePlayerBounds.topLeft.y -= worldSpacePlayerBounds.size.height-1;

  // Check for ladder attachment
  if (
    verticalMovementWanted &&
    state.mState != PlayerState::ClimbingLadder
  ) {
    if (movingUp) {
      const auto ladderTouchPoint = findLadderTouchPoint(
        worldSpacePlayerBounds);

      // If a ladder is in reach, start climbing
      if (ladderTouchPoint) {
        state.mState = PlayerState::ClimbingLadder;

        // Snap player position to ladder
        const auto relativeLadderTouchX = ladderTouchPoint->x - worldPosition.x;
        const auto offsetForOrientation =
          state.mOrientation == Orientation::Left ? 0 : 1;
        const auto diff = relativeLadderTouchX - offsetForOrientation;
        worldPosition.x += diff;

        physical.mGravityAffected = false;
      }
    }
  }

  if (state.mState == PlayerState::ClimbingLadder) {
    horizontalMovementWanted = false;
  }

  // Adjust orientation
  const auto oldOrientation = state.mOrientation;
  if (horizontalMovementWanted) {
    state.mOrientation =
      movingLeft ? Orientation::Left : Orientation::Right;
  }


  if (state.mState == PlayerState::Airborne) {
    verticalMovementWanted = false;
  }

  // Crouching/Looking up cancel out horizontal movement
  if (
      verticalMovementWanted &&
      (state.mState == PlayerState::LookingUp ||
      state.mState == PlayerState::Crouching ||
      state.mState == PlayerState::Standing ||
      state.mState == PlayerState::Walking)
  ) {
    horizontalMovementWanted = false;
  }

  if (state.mState == PlayerState::ClimbingLadder) {
    if (movingUp) {
      if (canClimbUp(worldSpacePlayerBounds)) {
        physical.mVelocity.y = -1.0f;
      } else {
        physical.mVelocity.y = 0.0f;
      }
    } else if (movingDown) {
      if (canClimbDown(worldSpacePlayerBounds)) {
        physical.mVelocity.y = 1.0f;
      } else {
        state.mState = PlayerState::Airborne;
        physical.mGravityAffected = true;
        physical.mVelocity.y = 1.0f;
        verticalMovementWanted = false;
      }
    } else {
      physical.mVelocity.y = 0.0f;
    }
  }

  state.mIsLookingDown = state.mIsLookingUp = false;
  if (
    verticalMovementWanted &&
    state.mState != PlayerState::ClimbingLadder
  ) {
    if (movingUp) {
      state.mState = PlayerState::LookingUp;
      state.mIsLookingUp = true;
    } else {
      state.mState = PlayerState::Crouching;
      state.mIsLookingDown = true;
    }
  }

  if (
    !verticalMovementWanted &&
    (state.mState == PlayerState::LookingUp ||
    state.mState == PlayerState::Crouching)
  ) {
    // If there's no more vertical movement requested, we start from standing
    // and let the normal horizontal movement logic figure out what to do
    state.mState = PlayerState::Standing;
  }

  // Update velocity for walking.
  // There's no delay for stopping, but starting to actually walk has 2 ticks
  // of delay.
  if (!horizontalMovementWanted) {
    if (
      state.mState == PlayerState::Walking
    ) {
      state.mState = PlayerState::Standing;
    }
    physical.mVelocity.x = 0.0f;
  } else {
    if (state.mState == PlayerState::Standing) {
      state.mState = PlayerState::Walking;
    }

    if (
      state.mState == PlayerState::Walking ||
      state.mState == PlayerState::Airborne
    ) {
      if (hasTicks) { // Delay for acceleration
        if (horizontalMovementWanted) {
          physical.mVelocity.x = movingLeft ? -1.0f : 1.0f;
        }
      }
    }
  }

  if (
    physical.mVelocity.y == 0.0f &&
    state.mState == PlayerState::Airborne
  ) {
    state.mState = PlayerState::Standing;
  }

  if (jumping && state.mState != PlayerState::Airborne) {
    physical.mVelocity.y = -3.6f;
    physical.mGravityAffected = true;
    state.mState = PlayerState::Airborne;
  }

  if (
    state.mState != oldState ||
    state.mOrientation != oldOrientation
  ) {
    const auto boundingBoxHeight =
      state.mState == PlayerState::Crouching ? 4 : 5;
    boundingBox = BoundingBox{
      {0, 0},
      {3, boundingBoxHeight}
    };
  }
}

bool PlayerMovementSystem::canClimbUp(
  const BoundingBox& worldSpacePlayerBounds
) const {
  // Is there still ladder above the player's current position?
  const auto row = worldSpacePlayerBounds.topLeft.y - 1;
  for (int x=0; x<worldSpacePlayerBounds.size.width; ++x) {
    const auto col = x + worldSpacePlayerBounds.topLeft.x;
    if (mLadderFlags.valueAtWithDefault(col, row, 0) == 1) {
      return true;
    }
  }
  return false;
}

bool PlayerMovementSystem::canClimbDown(
  const BoundingBox& worldSpacePlayerBounds
) const {
  // Is there still ladder below the player's current position?
  const auto row = worldSpacePlayerBounds.bottomLeft().y + 1;
  for (int x=0; x<worldSpacePlayerBounds.size.width; ++x) {
    const auto col = x + worldSpacePlayerBounds.topLeft.x;
    if (mLadderFlags.valueAtWithDefault(col, row, 0) == 1) {
      return true;
    }
  }
  return false;
}

boost::optional<base::Vector> PlayerMovementSystem::findLadderTouchPoint(
  const BoundingBox& worldSpacePlayerBounds
) const {
  const auto position = worldSpacePlayerBounds.topLeft;
  const auto size = worldSpacePlayerBounds.size;
  for (auto row=position.y; row<position.y+size.height; ++row) {
    for (auto col=position.x; col<position.x+size.width; ++col) {
      if (mLadderFlags.valueAtWithDefault(col, row, 0)) {
        return base::Vector{col, row};
      }
    }
  }

  return boost::none;
}

}}
