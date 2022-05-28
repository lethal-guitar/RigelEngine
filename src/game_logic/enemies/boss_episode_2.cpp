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

#include "boss_episode_2.hpp"

#include "base/match.hpp"
#include "engine/base_components.hpp"
#include "engine/physical_components.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/global_dependencies.hpp"

#include <array>


namespace rigel::game_logic::behaviors
{

namespace
{

constexpr auto FLY_RIGHT_MOVEMENT_SEQ = std::array<base::Vec2f, 39>{{
  {0.0f, 1.0f},  {0.0f, 1.0f},  {1.0f, 2.0f},  {1.0f, 2.0f},  {2.0f, 1.0f},
  {2.0f, 1.0f},  {2.0f, 0.0f},  {2.0f, 0.0f},  {2.0f, 0.0f},  {2.0f, 0.0f},
  {2.0f, 0.0f},  {2.0f, 0.0f},  {2.0f, 0.0f},  {2.0f, 0.0f},  {2.0f, 0.0f},
  {2.0f, 0.0f},  {2.0f, 0.0f},  {2.0f, 0.0f},  {2.0f, 0.0f},  {2.0f, 0.0f},
  {2.0f, 0.0f},  {2.0f, 0.0f},  {2.0f, 0.0f},  {2.0f, 0.0f},  {2.0f, 0.0f},
  {2.0f, 0.0f},  {2.0f, 0.0f},  {2.0f, 0.0f},  {2.0f, 0.0f},  {2.0f, 0.0f},
  {2.0f, 0.0f},  {2.0f, 0.0f},  {2.0f, 0.0f},  {2.0f, -1.0f}, {2.0f, -1.0f},
  {1.0f, -2.0f}, {1.0f, -2.0f}, {0.0f, -1.0f}, {0.0f, -1.0f},
}};


template <typename Sequence>
constexpr auto invertHorizontalDirection(const Sequence& sequence)
{
  auto invertedSequence = sequence;
  for (auto& elem : invertedSequence)
  {
    elem.x *= -1.0f;
  }
  return invertedSequence;
}


constexpr auto FLY_LEFT_MOVEMENT_SEQ =
  invertHorizontalDirection(FLY_RIGHT_MOVEMENT_SEQ);

int FLY_RIGHT_ANIM_SEQ[] = {2, 2, 3};
int FLY_LEFT_ANIM_SEQ[] = {4, 4, 5};

constexpr auto JUMP_RIGHT_MOVEMENT_SEQ = std::array<base::Vec2f, 9>{{
  {0.0f, -2.0f},
  {0.0f, -2.0f},
  {1.0f, -2.0f},
  {2.0f, -1.0f},
  {3.0f, 0.0f},
  {2.0f, 1.0f},
  {1.0f, 2.0f},
  {0.0f, 2.0f},
  {0.0f, 2.0f},
}};

constexpr auto JUMP_LEFT_MOVEMENT_SEQ =
  invertHorizontalDirection(JUMP_RIGHT_MOVEMENT_SEQ);

} // namespace


void BossEpisode2::update(
  GlobalDependencies& d,
  GlobalState& s,
  const bool isOnScreen,
  entityx::Entity entity)
{
  using namespace engine::components;

  auto& position = *entity.component<WorldPosition>();
  auto& body = *entity.component<MovingBody>();

  if (mDestructionPending)
  {
    body.mGravityAffected = false;
    body.mVelocity = {};
    engine::removeSafely<MovementSequence>(entity);
    engine::removeSafely<AnimationSequence>(entity);
    d.mpEvents->emit(rigel::events::BossDestroyed{entity});
    return;
  }

  if (mCoolDownFrames > 0)
  {
    --mCoolDownFrames;
    return;
  }

  base::match(
    mState,
    [&, this](WarmingUp& state) {
      ++state.mFramesElapsed;
      if (state.mFramesElapsed == 30)
      {
        mStartingHeight = position.y;
        d.mpEvents->emit(rigel::events::BossActivated{entity});
        mState = FlyingRight{};
      }
    },

    [&, this](FlyingRight& state) {
      if (state.mFramesElapsed == 0)
      {
        entity.replace<MovementSequence>(FLY_RIGHT_MOVEMENT_SEQ, true);

        entity.remove<AnimationLoop>();
        engine::startAnimationSequence(entity, FLY_RIGHT_ANIM_SEQ);
      }

      ++state.mFramesElapsed;
      if (state.mFramesElapsed == 37)
      {
        engine::startAnimationLoop(entity, 1, 0, 1);
      }

      if (state.mFramesElapsed == 39)
      {
        mCoolDownFrames = 25;
        mState = FlyingLeft{};
      }
    },

    [&, this](FlyingLeft& state) {
      if (state.mFramesElapsed == 0)
      {
        entity.replace<MovementSequence>(FLY_LEFT_MOVEMENT_SEQ, true);
        entity.remove<AnimationLoop>();
        engine::startAnimationSequence(entity, FLY_LEFT_ANIM_SEQ);
      }

      ++state.mFramesElapsed;
      if (state.mFramesElapsed == 37)
      {
        engine::startAnimationLoop(entity, 1, 0, 1);
      }

      if (state.mFramesElapsed == 39)
      {
        mCoolDownFrames = 25;
        mState = FallingDown{};
      }
    },

    [&, this](const FallingDown&) {
      if (body.mGravityAffected == false)
      {
        body.mGravityAffected = true;
        body.mIgnoreCollisions = false;
        body.mVelocity.y = 0.5f;
      }
    },

    [&, this](JumpingRight& state) {
      if (state.mFramesElapsed == 0)
      {
        body.mIgnoreCollisions = true;
        entity.replace<MovementSequence>(JUMP_RIGHT_MOVEMENT_SEQ, true);
      }

      ++state.mFramesElapsed;
      if (state.mFramesElapsed == 9)
      {
        state.mFramesElapsed = 0;

        ++state.mJumpsCompleted;
        if (state.mJumpsCompleted == 8)
        {
          mState = JumpingLeft{};
        }
      }
    },

    [&, this](JumpingLeft& state) {
      if (state.mFramesElapsed == 0)
      {
        entity.replace<MovementSequence>(JUMP_LEFT_MOVEMENT_SEQ, true);
      }

      ++state.mFramesElapsed;
      if (state.mFramesElapsed == 9)
      {
        state.mFramesElapsed = 0;

        ++state.mJumpsCompleted;
        if (state.mJumpsCompleted == 8)
        {
          mState = RisingUp{};
        }
      }
    },

    [&, this](const RisingUp&) {
      --position.y;
      if (position.y == mStartingHeight)
      {
        mCoolDownFrames = 100;
        mState = FlyingRight();
      }
    });
}


void BossEpisode2::onCollision(
  GlobalDependencies& d,
  GlobalState&,
  const engine::events::CollidedWithWorld& event,
  entityx::Entity entity)
{
  using namespace engine::components;

  auto& body = *entity.component<MovingBody>();

  if (auto pState = std::get_if<FallingDown>(&mState);
      pState && event.mCollidedBottom)
  {
    mCoolDownFrames = 30;
    body.mGravityAffected = false;
    body.mVelocity.y = 0.0f;
    mState = JumpingRight{};
  }
}


void BossEpisode2::onKilled(
  GlobalDependencies& d,
  GlobalState&,
  const base::Vec2f&,
  entityx::Entity entity)
{
  mDestructionPending = true;
}

} // namespace rigel::game_logic::behaviors
