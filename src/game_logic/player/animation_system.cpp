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

#include "data/sound_ids.hpp"
#include "game_mode.hpp"

RIGEL_DISABLE_WARNINGS
#include <boost/optional.hpp>
RIGEL_RESTORE_WARNINGS


namespace ex = entityx;

namespace rigel { namespace game_logic { namespace player {

using namespace engine::components;
using namespace game_logic::components;

namespace {

const auto FRAMES_PER_ORIENTATION = 39;



int orientedAnimationFrame(
  const int frame,
  const player::Orientation orientation
) {
  const auto orientationOffset =
    orientation == Orientation::Right ? FRAMES_PER_ORIENTATION : 0;
  return frame + orientationOffset;
}

}


AnimationSystem::AnimationSystem(
  ex::Entity player,
  IGameServiceProvider* pServiceProvider
)
  : mPlayer(player)
  , mpServiceProvider(pServiceProvider)
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

  auto& state = *mPlayer.component<PlayerControlled>().get();
  auto& sprite = *mPlayer.component<Sprite>().get();

  if (state.mState == player::PlayerState::Dead) {
    return;
  }

  if (state.mState == player::PlayerState::Dieing) {
    // Initialize animation on first frame
    if (!state.mDeathAnimationState) {
      state.mDeathAnimationState = detail::DeathAnimationState{};

      if (mPlayer.has_component<Animated>()) {
        mPlayer.remove<Animated>();
      }
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

      mPreviousState = state.mState;
      mPreviousOrientation = state.mOrientation;
    }
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

  boost::optional<int> endFrameOffset;
  int newAnimationFrame = 0;

  switch (state.mState) {
    case PlayerState::Standing:
      newAnimationFrame = 0;
      break;

    case PlayerState::Walking:
      newAnimationFrame = 1;
      endFrameOffset = 3;
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

  if (mPlayer.has_component<Animated>()) {
    mPlayer.remove<Animated>();
  }
  if (endFrameOffset) {
    mPlayer.assign<Animated>(Animated{{AnimationSequence{
      4, frameToShow, frameToShow + *endFrameOffset}}});
  }
}

}}}
