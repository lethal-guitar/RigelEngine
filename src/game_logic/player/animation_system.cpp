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

#include "animation_system.hpp"

#include "base/warnings.hpp"
#include "data/sound_ids.hpp"
#include "engine/life_time_components.hpp"
#include "engine/physical_components.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/entity_factory.hpp"
#include "game_logic/player/attack_traits.hpp"
#include "game_mode.hpp"

RIGEL_DISABLE_WARNINGS
#include <boost/optional.hpp>
RIGEL_RESTORE_WARNINGS

#include <cmath>
#include <unordered_map>


namespace ex = entityx;

namespace rigel { namespace game_logic { namespace player {

using namespace engine::components;
using namespace game_logic::components;

namespace {

const auto DEATH_ANIM_BASE_FRAME = 29;
const auto FRAMES_PER_ORIENTATION = 39;
const auto MOVEMENT_BASED_ANIM_SPEED_SCALE = 2;
const auto MUZZLE_FLASH_DRAW_ORDER = 12;
const auto NUM_LADDER_ANIM_STATES = 2;
const auto NUM_WALK_ANIM_STATES = 4;


const std::unordered_map<int, int> ATTACK_FRAME_MAP = {
  {0, 18},
  {17, 34},
  {16, 19},
  {20, 27},
  {25, 26},

  // TODO generate the reverse mappings programmatically
  {18, 0},
  {34, 17},
  {19, 16},
  {27, 20},
  {26, 25}
};


const std::unordered_map<player::PlayerState, int> STATE_FRAME_MAP = {
  {PlayerState::Standing, 0},
  {PlayerState::Walking, 0},
  {PlayerState::LookingUp, 16},
  {PlayerState::Crouching, 17}
};


int deathAnimationFrame(const int elapsedTicks, const int currentFrame) {
  if (elapsedTicks == 0) {
    // Keep showing the player's previous animation frame for one tick
    return currentFrame;
  }

  const auto stage = std::min(3, elapsedTicks >= 5 ? elapsedTicks - 4 : 0);
  return DEATH_ANIM_BASE_FRAME + stage;
}


data::ActorID muzzleFlashActorId(const ProjectileDirection direction) {
  static const data::ActorID DIRECTION_MAP[] = { 35, 36, 33, 34 };
  return DIRECTION_MAP[static_cast<std::size_t>(direction)];
}


base::Vector muzzleFlashOffset(
  const player::PlayerState state,
  const player::Orientation orientation
) {
  const auto horizontalOffset = state == player::PlayerState::LookingUp
    ? (orientation == player::Orientation::Left ? 0 : 2)
    : (orientation == player::Orientation::Left ? -3 : 3);
  const auto verticalOffset = state == player::PlayerState::LookingUp
    ? -5
    : (state == player::PlayerState::Crouching ? -1 : -2);

  return {horizontalOffset, verticalOffset};
}

}


AnimationSystem::AnimationSystem(
  ex::Entity player,
  IGameServiceProvider* pServiceProvider,
  EntityFactory* pFactory
)
  : mPlayer(player)
  , mpServiceProvider(pServiceProvider)
  , mpEntityFactory(pFactory)
  , mShotAnimationActive(false)
  , mPreviousState(mPlayer.component<PlayerControlled>()->mState)
  , mWasInteracting(false)
{
}


void AnimationSystem::update(
  entityx::EntityManager& es,
  entityx::EventManager& events,
  entityx::TimeDelta dt
) {
  assert(mPlayer.has_component<PlayerControlled>());
  assert(mPlayer.has_component<Sprite>());
  assert(mPlayer.has_component<WorldPosition>());

  auto& state = *mPlayer.component<PlayerControlled>();
  if (state.mState == player::PlayerState::Dead) {
    return;
  }

  auto& sprite = *mPlayer.component<Sprite>();
  // Update mercy frame blink effect
  // ----------------------------------
  if (!state.isPlayerDead()) {
    sprite.mShow = true;

    if (state.mMercyFramesTimeElapsed) {
      const auto mercyTimeElapsed = *state.mMercyFramesTimeElapsed;
      // TODO: Flash white at end of mercy frames instead of blinking to
      // invisible.
      const auto mercyFramesElapsed =
        static_cast<int>(engine::timeToGameFrames(mercyTimeElapsed));
      const auto blinkSprite = mercyFramesElapsed % 2 != 0;
      sprite.mShow = !blinkSprite;
    }
  }

  // Death sequence
  // ----------------------------------
  if (state.mState == PlayerState::Dieing) {
    // Initialize animation on first frame
    if (!state.mDeathAnimationFramesElapsed) {
      state.mDeathAnimationFramesElapsed = 0;
    }

    auto& elapsedFrames = *state.mDeathAnimationFramesElapsed;

    ++elapsedFrames;
    if (elapsedFrames == 17) {
      // TODO: Trigger particles
      sprite.mShow = false;
      mpServiceProvider->playSound(data::SoundId::AlternateExplosion);
    } else if (elapsedFrames >= 42) {
      state.mState = PlayerState::Dead;
    }
  }

  // Update sprite's animation frame
  // ----------------------------------

  // 'normalize' the frame index by removing the orientation offset, if any.
  const auto currentAnimationFrame =
    sprite.mFramesToRender[0] % FRAMES_PER_ORIENTATION;

  const auto newAnimationFrame = determineAnimationFrame(
    state, dt, currentAnimationFrame);

  const auto orientationOffset =
    state.mOrientation == Orientation::Right ? FRAMES_PER_ORIENTATION : 0;
  sprite.mFramesToRender[0] = newAnimationFrame + orientationOffset;
}


int AnimationSystem::determineAnimationFrame(
  PlayerControlled& state,
  const engine::TimeDelta dt,
  const int currentAnimationFrame
) {
  if (state.mState != player::PlayerState::Dieing) {
    auto newAnimationFrame =
      movementAnimationFrame(state, currentAnimationFrame);
    return attackAnimationFrame(state, dt, newAnimationFrame);
  } else {
    return deathAnimationFrame(
      *state.mDeathAnimationFramesElapsed,
      currentAnimationFrame);
  }
}


int AnimationSystem::movementAnimationFrame(
  PlayerControlled& state,
  const int currentAnimationFrame
) {
  int newAnimationFrame = currentAnimationFrame;

  const auto& playerPosition = *mPlayer.component<WorldPosition>();
  if (
    state.mState != mPreviousState ||
    (mWasInteracting && !state.mIsInteracting)
  ) {
    const auto it = STATE_FRAME_MAP.find(state.mState);
    if (it != STATE_FRAME_MAP.end()) {
      newAnimationFrame = it->second;
    }

    if (state.mState == PlayerState::Walking) {
      state.mPositionAtAnimatedMoveStart = playerPosition.x;
    } else if (state.mState == PlayerState::ClimbingLadder) {
      state.mPositionAtAnimatedMoveStart = playerPosition.y;
    } else {
      state.mPositionAtAnimatedMoveStart = boost::none;
    }

    mPreviousState = state.mState;
  }

  const auto calcMovementBasedFrame = [](
    PlayerControlled& playerState,
    const int currentPosition,
    const int numAnimStates
  ) {
    const auto startPosition = *playerState.mPositionAtAnimatedMoveStart;
    const auto distance = std::abs(startPosition - currentPosition);
    return (distance / MOVEMENT_BASED_ANIM_SPEED_SCALE) % numAnimStates;
  };

  if (state.mState == PlayerState::Walking) {
    newAnimationFrame = 1 + calcMovementBasedFrame(
      state, playerPosition.x, NUM_WALK_ANIM_STATES);
  } else if (state.mState == PlayerState::ClimbingLadder) {
    newAnimationFrame = 35 + calcMovementBasedFrame(
      state, playerPosition.y, NUM_LADDER_ANIM_STATES);
  } else if (state.mState == PlayerState::Airborne) {
    const auto verticalVelocity =
      mPlayer.component<engine::components::Physical>()->mVelocity.y;
    if (verticalVelocity != 0.0f) {
      if (verticalVelocity <= 0.0f) {
        newAnimationFrame = 6;
      } else if (verticalVelocity < 2.0f) {
        newAnimationFrame = 7;
      } else {
        newAnimationFrame = 8;
      }
    }
  }

  if (state.mIsInteracting) {
    newAnimationFrame = 33;
  }
  mWasInteracting = state.mIsInteracting;

  return newAnimationFrame;
}


int AnimationSystem::attackAnimationFrame(
  PlayerControlled& state,
  engine::TimeDelta dt,
  const int currentAnimationFrame
) {
  auto newAnimationFrame = currentAnimationFrame;

  if (mShotAnimationActive) {
    mShotAnimationActive = false;

    const auto iter = ATTACK_FRAME_MAP.find(currentAnimationFrame);
    if (iter != ATTACK_FRAME_MAP.end()) {
      newAnimationFrame = iter->second;
    }
  }

  const auto& playerPosition = *mPlayer.component<WorldPosition>();
  if (state.mShotFired && !mShotAnimationActive) {
    mShotAnimationActive = true;

    const auto shotDirection =
      player::shotDirection(state.mState, state.mOrientation);
    const auto spriteId = muzzleFlashActorId(shotDirection);

    auto muzzleFlashEntity = mpEntityFactory->createSprite(spriteId);
    muzzleFlashEntity.component<Sprite>()->mDrawOrder =
      MUZZLE_FLASH_DRAW_ORDER;
    muzzleFlashEntity.assign<WorldPosition>(
       playerPosition + muzzleFlashOffset(state.mState, state.mOrientation));
    muzzleFlashEntity.assign<AutoDestroy>(AutoDestroy::afterTimeout(1));

    const auto iter = ATTACK_FRAME_MAP.find(currentAnimationFrame);
    if (iter != ATTACK_FRAME_MAP.end()) {
      newAnimationFrame = iter->second;
    }
  }

  return newAnimationFrame;
}

}}}
