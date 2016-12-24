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

const auto NUM_WALK_ANIM_STATES = 4;
const auto FRAMES_PER_ORIENTATION = 39;

RIGEL_DISABLE_GLOBAL_CTORS_WARNING

const std::unordered_map<int, int> ATTACK_FRAME_MAP = {
  {0, 18},
  {17, 34},
  {16, 19},
  {20, 27},
  {25, 26},

  {18, 0},
  {34, 17},
  {19, 16},
  {27, 20},
  {26, 25}
};

RIGEL_RESTORE_WARNINGS


int orientedAnimationFrame(
  const int frame,
  const player::Orientation orientation
) {
  const auto orientationOffset =
    orientation == Orientation::Right ? FRAMES_PER_ORIENTATION : 0;
  return frame + orientationOffset;
}


int baseAnimationFrame(
  const int frame,
  const player::Orientation orientation
) {
  const auto orientationOffset =
    orientation == Orientation::Right ? FRAMES_PER_ORIENTATION : 0;
  return frame - orientationOffset;
}


void toggleAttackAnimationFrame(
  engine::components::Sprite& sprite,
  const player::Orientation orientation
) {
  const auto currentFrame =
    baseAnimationFrame(sprite.mFramesToRender[0], orientation);
  const auto iter = ATTACK_FRAME_MAP.find(currentFrame);
  if (iter != ATTACK_FRAME_MAP.end()) {
    sprite.mFramesToRender[0] =
      orientedAnimationFrame(iter->second, orientation);
  }
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
{
  assert(mPlayer.has_component<PlayerControlled>());
  auto& state = *mPlayer.component<PlayerControlled>().get();
  mPreviousOrientation = state.mOrientation;
  mPreviousState = state.mState;
}


void AnimationSystem::update(
  entityx::EntityManager& es,
  entityx::EventManager& events,
  entityx::TimeDelta dt
) {
  assert(mPlayer.has_component<PlayerControlled>());
  assert(mPlayer.has_component<Sprite>());
  assert(mPlayer.has_component<WorldPosition>());

  auto& state = *mPlayer.component<PlayerControlled>().get();
  auto& sprite = *mPlayer.component<Sprite>().get();
  const auto& position = *mPlayer.component<WorldPosition>().get();

  if (state.mState == player::PlayerState::Dead) {
    return;
  }

  if (state.mState == player::PlayerState::Dieing) {
    // Initialize animation on first frame
    if (!state.mDeathAnimationState) {
      state.mDeathAnimationState = detail::DeathAnimationState{};
    }

    updateDeathAnimation(state, sprite, dt);
  } else {
    sprite.mShow = true;
    if (state.mMercyFramesTimeElapsed) {
      updateMercyFramesAnimation(*state.mMercyFramesTimeElapsed, sprite);
    }

    if (
      state.mState != mPreviousState ||
      state.mOrientation != mPreviousOrientation
    ) {
      updateAnimation(state, sprite);

      if (state.mState == PlayerState::Walking) {
        state.mPositionAtAnimatedMoveStart = position.x;
      } else {
        state.mPositionAtAnimatedMoveStart = boost::none;
      }

      mPreviousState = state.mState;
      mPreviousOrientation = state.mOrientation;
    }

    if (state.mState == PlayerState::Walking) {
      const auto walkStartPosition = *state.mPositionAtAnimatedMoveStart;
      const auto distance = std::abs(walkStartPosition - position.x);
      const auto frame = 1 + (distance / 2) % NUM_WALK_ANIM_STATES;
      sprite.mFramesToRender[0] =
        orientedAnimationFrame(frame, state.mOrientation);
    }

    updateAttackAnimation(state, sprite, dt);
  }
}


void AnimationSystem::updateMercyFramesAnimation(
  const engine::TimeDelta mercyTimeElapsed,
  Sprite& sprite
) {
  // TODO: Flash white at end of mercy frames instead of blinking to
  // invisible.
  const auto mercyFramesElapsed =
    static_cast<int>(engine::timeToGameFrames(mercyTimeElapsed));
  const auto blinkSprite = mercyFramesElapsed % 2 != 0;
  sprite.mShow = !blinkSprite;
}


void AnimationSystem::updateDeathAnimation(
  PlayerControlled& playerState,
  Sprite& sprite,
  engine::TimeDelta dt
) {
  assert(playerState.mState == PlayerState::Dieing);
  assert(playerState.mDeathAnimationState);

  auto& animationState = *playerState.mDeathAnimationState;
  if (!updateAndCheckIfDesiredTicksElapsed(animationState.mStepper, 2, dt)) {
    return;
  }

  const auto elapsedFrames = ++animationState.mElapsedFrames;

  boost::optional<int> newFrameToShow;
  if (elapsedFrames == 1) {
    newFrameToShow = 29;
  } else if (elapsedFrames == 5) {
    newFrameToShow = 30;
  } else if (elapsedFrames == 6) {
    newFrameToShow = 31;
  } else if (elapsedFrames == 7) {
    newFrameToShow = 32;
  } else if (elapsedFrames == 17) {
    // TODO: Trigger particles
    sprite.mShow = false;
    mpServiceProvider->playSound(data::SoundId::AlternateExplosion);
  } else if (elapsedFrames >= 42) {
    playerState.mState = PlayerState::Dead;
  }

  if (newFrameToShow) {
    sprite.mFramesToRender[0] =
      orientedAnimationFrame(*newFrameToShow, playerState.mOrientation);
  }
}


void AnimationSystem::updateAnimation(
  const PlayerControlled& state,
  Sprite& sprite
) {
  // All the magic numbers in this function are matched to the frame indices in
  // the game's sprite sheet for Duke.

  int newAnimationFrame = 0;

  switch (state.mState) {
    case PlayerState::Standing:
    case PlayerState::Walking:
      newAnimationFrame = 0;
      break;


    case PlayerState::LookingUp:
      newAnimationFrame = 16;
      break;

    case PlayerState::Crouching:
      newAnimationFrame = 17;
      break;

    case PlayerState::Airborne:
      newAnimationFrame = 8;
      break;

    case PlayerState::ClimbingLadder:
      newAnimationFrame = 36;
      break;

    default:
      break;
  }

  const auto frameToShow =
    orientedAnimationFrame(newAnimationFrame, state.mOrientation);
  sprite.mFramesToRender[0] = frameToShow;
}


void AnimationSystem::updateAttackAnimation(
  components::PlayerControlled& state,
  engine::components::Sprite& sprite,
  engine::TimeDelta dt
) {
  if (state.mShotFired && mElapsedForShotAnimation == boost::none) {
    mElapsedForShotAnimation = 0.0;

    const auto shotDirection =
      player::shotDirection(state.mState, state.mOrientation);
    const auto spriteId = muzzleFlashActorId(shotDirection);
    const auto playerDrawIndex = sprite.mDrawOrder;

    mMuzzleFlashEntity = mpEntityFactory->createSprite(spriteId);
    mMuzzleFlashEntity.component<Sprite>()->mDrawOrder = playerDrawIndex + 1;
    mMuzzleFlashEntity.assign<WorldPosition>();

    toggleAttackAnimationFrame(sprite, state.mOrientation);
  }

  if (mMuzzleFlashEntity.valid()) {
    *mMuzzleFlashEntity.component<WorldPosition>().get() =
      *mPlayer.component<WorldPosition>().get() +
        muzzleFlashOffset(state.mState, state.mOrientation);

    *mElapsedForShotAnimation += dt;
    if (*mElapsedForShotAnimation >= engine::gameFramesToTime(1)) {
      mMuzzleFlashEntity.destroy();
      mElapsedForShotAnimation = boost::none;
      toggleAttackAnimationFrame(sprite, state.mOrientation);
    }
  }
}

}}}
