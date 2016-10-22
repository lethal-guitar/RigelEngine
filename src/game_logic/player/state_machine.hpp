#pragma once

#include <engine/timing.hpp>


namespace rigel { namespace game_logic { namespace player {

const auto WALK_START_DELAY = (1.0 / 280.0) * 16.0;


struct InputState {
  bool mMovingLeft = false;
  bool mMovingRight = false;
  bool mMovingUp = false;
  bool mMovingDown = false;
  bool mJumping = false;
  bool mShooting = false;
};


enum class Orientation {
  Left,
  Right
};


enum class State {
  Standing,
  Walking,
  LookingUp,
  Crouching
};


struct StateComponent {
  Orientation mOrientation;
  State mState;
};


class StateMachine {
public:
  StateMachine(StateComponent* pState);

  void update(engine::TimeDelta dt, const InputState& inputs);

private:
  InputState filterConflictingInputs(const InputState& inputs) const;

private:
  StateComponent* mpState;

  bool mWalkStartPending = false;
  engine::TimeDelta mElapsedTimeForWalkStartDelay;
};


StateMachine::StateMachine(StateComponent* pState)
  : mpState(pState)
{
}


void StateMachine::update(
  const engine::TimeDelta dt,
  const InputState& unfilteredInputs
) {
  const auto inputs = filterConflictingInputs(unfilteredInputs);

  if (inputs.mMovingLeft) {
    mpState->mOrientation = Orientation::Left;
  } else if (inputs.mMovingRight) {
    mpState->mOrientation = Orientation::Right;
  }

  const auto previousState = mpState->mState;

  const auto horizontalMovement = inputs.mMovingLeft || inputs.mMovingRight;

  if (horizontalMovement) {
    if (previousState == State::Standing) {
      if (!mWalkStartPending) {
        mWalkStartPending = true;
        mElapsedTimeForWalkStartDelay = 0.0;
      }

      mElapsedTimeForWalkStartDelay += dt;
      if (mElapsedTimeForWalkStartDelay >= WALK_START_DELAY) {
        mpState->mState = State::Walking;
        mWalkStartPending = false;
      }
    } else {
      mpState->mState = State::Walking;
    }
  } else {
    mpState->mState = State::Standing;
  }

  if (inputs.mMovingUp) {
    mpState->mState = State::LookingUp;
  } else if (inputs.mMovingDown) {
    mpState->mState = State::Crouching;
  }
}


InputState StateMachine::filterConflictingInputs(
  const InputState& inputs
) const {
  auto filtered = inputs;

  if (filtered.mMovingUp && filtered.mMovingDown) {
    filtered.mMovingUp = filtered.mMovingDown = false;
  }

  if (filtered.mMovingLeft && filtered.mMovingRight) {
    filtered.mMovingLeft = filtered.mMovingRight = false;
  }

  return filtered;
}

}}}
