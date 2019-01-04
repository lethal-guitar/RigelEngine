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

#include "watch_bot.hpp"

#include "base/match.hpp"
#include "data/sound_ids.hpp"
#include "engine/entity_tools.hpp"
#include "engine/movement.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/physical_components.hpp"
#include "game_logic/player.hpp"

#include "game_service_provider.hpp"


namespace rigel { namespace game_logic { namespace behaviors {

namespace {

const int LAND_ON_GROUND_ANIM[] = { 1, 2, 1 };

const int LOOK_LEFT_RIGHT_ANIM[] = {
  1, 1, 1, 3, 3, 1, 6, 6, 7, 8, 7, 6, 6, 6, 7, 8, 7, 6, 6, 6,
  1, 1, 3, 3, 3, 1, 1, 1, 6, 6, 1, 1
};

const int LOOK_RIGHT_LEFT_ANIM[] = {
  1, 1, 6, 6, 7, 8, 7, 6, 6, 1, 1, 3, 3, 1, 6, 6, 1, 1, 1, 3,
  4, 5, 4, 3, 3, 3, 4, 5, 4, 3, 1, 1
};


void advanceRandomNumberGenerator(GlobalDependencies& d) {
  // The result isn't used, this is just done in order to exactly mimick
  // how the original game uses the random number generator (since each
  // invocation influences subsequent calls).
  d.mpRandomGenerator->gen();
}

}


void WatchBot::update(
  GlobalDependencies& d,
  GlobalState& s,
  const bool isOnScreen,
  entityx::Entity entity
) {
  using namespace engine;
  using namespace engine::components;
  using namespace watch_bot;

  const auto& position = *entity.component<WorldPosition>();
  const auto& playerPos = s.mpPlayer->orientedPosition();

  auto& animationFrame = entity.component<Sprite>()->mFramesToRender[0];
  auto& movingBody = *entity.component<MovingBody>();

  auto jump = [&]() {
    animationFrame = 0;

    const auto newOrientation = position.x > playerPos.x
      ? Orientation::Left
      : Orientation::Right;
    mState = Jumping{newOrientation};
  };


  base::match(mState,
    [&, this](Jumping& state) {
      moveHorizontally(
        *d.mpCollisionChecker,
        entity,
        orientation::toMovement(state.mOrientation));
      const auto speed = state.mFramesElapsed < 2 ? 2 : 1;
      const auto moveResult = moveVertically(
        *d.mpCollisionChecker,
        entity,
        -speed);

      ++state.mFramesElapsed;

      const auto collidedWithCeiling = moveResult != MovementResult::Completed;
      if (collidedWithCeiling || state.mFramesElapsed >= 5) {
        movingBody.mGravityAffected = true;
        movingBody.mVelocity.y = -0.5f;
        mState = Falling{state.mOrientation};
        return;
      }
    },

    [&, this](const Falling& state) {
      moveHorizontally(
        *d.mpCollisionChecker,
        entity,
        orientation::toMovement(state.mOrientation));
    },

    [&, this](OnGround& state) {
      const auto shouldLookAround = (d.mpRandomGenerator->gen() & 0x21) != 0;

      ++state.mFramesElapsed;
      if (shouldLookAround && state.mFramesElapsed == 1) {
        // Stop landing animation
        removeSafely<AnimationSequence>(entity);

        const auto orientation = d.mpRandomGenerator->gen() % 2 == 0
          ? Orientation::Left
          : Orientation::Right;
        mState = LookingAround{orientation};
        return;
      }

      if (state.mFramesElapsed == 3) {
        jump();
      }
    },

    [&, this](LookingAround& state) {
      // TODO: Is there a way to use AnimationSequence here, but still only
      // update on odd frames?
      if (state.mFramesElapsed < 32) {
        const auto sequence = state.mOrientation == Orientation::Left
          ? LOOK_LEFT_RIGHT_ANIM
          : LOOK_RIGHT_LEFT_ANIM;
        animationFrame = sequence[state.mFramesElapsed];
      }

      if (s.mpPerFrameState->mIsOddFrame) {
        ++state.mFramesElapsed;
      }

      if (state.mFramesElapsed == 33) {
        animationFrame = 1;
        advanceRandomNumberGenerator(d);
      } else if (state.mFramesElapsed == 34) {
        jump();
      }
    }
  );

  engine::synchronizeBoundingBoxToSprite(entity);
}


void WatchBot::onCollision(
  GlobalDependencies& d,
  GlobalState& s,
  const engine::events::CollidedWithWorld&,
  entityx::Entity entity
) {
  using namespace engine::components;
  using namespace watch_bot;

  const auto isOnScreen = entity.component<Active>()->mIsOnScreen;

  base::match(mState,
    [&, this](const Falling& state) {
      if (isOnScreen) {
        d.mpServiceProvider->playSound(data::SoundId::DukeJumping);
      }

      engine::startAnimationSequence(entity, LAND_ON_GROUND_ANIM);
      entity.component<MovingBody>()->mGravityAffected = false;
      mState = OnGround{};
      advanceRandomNumberGenerator(d);

      engine::synchronizeBoundingBoxToSprite(entity);
    },

    [](const auto&) {}
  );
}

}}}
