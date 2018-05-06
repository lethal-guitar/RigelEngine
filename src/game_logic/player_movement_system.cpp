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


void initializePlayerEntity(entityx::Entity player, const bool isFacingRight) {
  using namespace engine::components::parameter_aliases;

  PlayerControlled controls;
  controls.mOrientation =
    isFacingRight ? Orientation::Right : Orientation::Left;
  auto sprite = player.component<Sprite>();
  if (sprite)
  {
    sprite->mFramesToRender = {isFacingRight ? 39 : 0};
  }
  player.assign<PlayerControlled>(controls);

  player.assign<MovingBody>(
    Velocity{0.0f, 0.0f}, GravityAffected{true}, IsPlayer{true});
  player.assign<BoundingBox>(BoundingBox{{0, 0}, {3, 5}});

}


void resetForRespawn(
  entityx::Entity player,
  const base::Vector& checkpointPosition
) {
  player.remove<PlayerControlled>();
  player.remove<BoundingBox>();

  player.component<MovingBody>()->mVelocity = {};
  player.component<Sprite>()->mFramesToRender = {39};
  player.component<Sprite>()->mShow = true;
  *player.component<WorldPosition>() = checkpointPosition;
  PlayerControlled controls;
  controls.mOrientation = Orientation::Right;
  player.assign<PlayerControlled>(controls);
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
  const data::map::Map& map
)
  : mPlayer(player)
  , mWalkRequestedLastFrame(false)
  , mLadderFlags(map.width(), map.height())
{
  for (int row=0; row<map.height(); ++row) {
    for (int col=0; col<map.width(); ++col) {
      const auto isLadder =
        map.attributes().isLadder(map.tileAt(0, col, row)) ||
        map.attributes().isLadder(map.tileAt(1, col, row));
      mLadderFlags.setValueAt(col, row, isLadder);
    }
  }
}


void PlayerMovementSystem::update(const PlayerInputState& inputState) {
  assert(mPlayer.has_component<PlayerControlled>());
  assert(mPlayer.has_component<MovingBody>());
  assert(mPlayer.has_component<WorldPosition>());

  auto& state = *mPlayer.component<PlayerControlled>();
  auto& body = *mPlayer.component<MovingBody>();
  auto& boundingBox = *mPlayer.component<BoundingBox>();
  auto& worldPosition = *mPlayer.component<WorldPosition>();

  if (state.isPlayerDead() || state.mIsInteracting) {
    return;
  }

  auto movingLeft = inputState.mMovingLeft;
  auto movingRight = inputState.mMovingRight;
  auto movingUp = inputState.mMovingUp;
  auto movingDown = inputState.mMovingDown;
  auto jumping = inputState.mJumping;

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
      if (ladderTouchPoint && canClimbUp(worldSpacePlayerBounds)) {
        state.mState = PlayerState::ClimbingLadder;

        // Snap player position to ladder
        const auto relativeLadderTouchX = ladderTouchPoint->x - worldPosition.x;
        const auto offsetForOrientation =
          state.mOrientation == Orientation::Left ? 0 : 1;
        const auto diff = relativeLadderTouchX - offsetForOrientation;
        worldPosition.x += diff;

        body.mGravityAffected = false;
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
        body.mVelocity.y = -1.0f;
      } else {
        body.mVelocity.y = 0.0f;
      }
    } else if (movingDown) {
      if (canClimbDown(worldSpacePlayerBounds)) {
        body.mVelocity.y = 1.0f;
      } else {
        state.mState = PlayerState::Airborne;
        body.mGravityAffected = true;
        body.mVelocity.y = 1.0f;
        verticalMovementWanted = false;
      }
    } else {
      body.mVelocity.y = 0.0f;
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
  // There's no delay for stopping, but starting to actually walk has 1 frame
  // of delay to allow for turning without moving.
  if (!horizontalMovementWanted) {
    if (
      state.mState == PlayerState::Walking
    ) {
      state.mState = PlayerState::Standing;
    }
    body.mVelocity.x = 0.0f;
  } else {

    if (state.mState == PlayerState::Standing) {
      state.mState = PlayerState::Walking;
    }

    if (
      state.mState == PlayerState::Walking ||
      state.mState == PlayerState::Airborne
    ) {
      const auto canStartMoving = mWalkRequestedLastFrame ||
        state.mOrientation == oldOrientation;
      if (horizontalMovementWanted && canStartMoving) {
        body.mVelocity.x = movingLeft ? -1.0f : 1.0f;
      }
    }
  }

  mWalkRequestedLastFrame = horizontalMovementWanted;

  if (
    body.mVelocity.y == 0.0f &&
    state.mState == PlayerState::Airborne
  ) {
    state.mState = PlayerState::Standing;
  }

  if (
    body.mVelocity.y != 0.0f &&
    state.mState != PlayerState::Airborne &&
    state.mState != PlayerState::ClimbingLadder
  ) {
    state.mState = PlayerState::Airborne;
  }

  if (!jumping) {
    state.mPerformedJump = false;
  }

  if (
    jumping &&
    state.mState != PlayerState::Airborne &&
    !state.mPerformedJump
  ) {
    body.mVelocity.y = -3.6f;
    body.mGravityAffected = true;
    state.mState = PlayerState::Airborne;
    state.mPerformedJump = true;
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
