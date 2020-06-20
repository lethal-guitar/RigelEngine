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
#include "common/game_service_provider.hpp"
#include "data/sound_ids.hpp"
#include "engine/collision_checker.hpp"
#include "engine/entity_tools.hpp"
#include "engine/movement.hpp"
#include "engine/physical_components.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/sprite_tools.hpp"
#include "game_logic/behavior_controller.hpp"
#include "game_logic/effect_components.hpp"
#include "game_logic/entity_factory.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic::behaviors {

namespace {

constexpr auto CONTAINER_OFFSET = base::Vector{0, -2};


const effects::EffectSpec CARRIER_SELF_DESTRUCT_EFFECT_SPEC[] = {
  {effects::RandomExplosionSound{}, 0},
  {effects::SpriteCascade{data::ActorID::Shot_impact_FX}, 0},
};


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
  // The result isn't used, this is just done in order to exactly mimic
  // how the original game uses the random number generator (since each
  // invocation influences subsequent calls).
  d.mpRandomGenerator->gen();
}


struct WatchBotContainer {
  void update(
    GlobalDependencies& dependencies,
    GlobalState& state,
    bool isOnScreen,
    entityx::Entity entity);

  int mFramesElapsed = 0;
};

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
  const auto& bbox = *entity.component<BoundingBox>();
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

      if (d.mpCollisionChecker->isOnSolidGround(position, bbox)) {
        land(entity, d);
      }
    },

    [&, this](OnGround& state) {
      const auto randomChoice = d.mpRandomGenerator->gen();
      const auto shouldLookAround =
        randomChoice % 2 != 0 &&
        (randomChoice / 32) % 2 != 0;

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
  using namespace watch_bot;

  base::match(mState,
    [&, this](const Falling& state) {
      land(entity, d);
    },

    [](const auto&) {}
  );
}


void WatchBot::land(
  entityx::Entity entity,
  GlobalDependencies& d
) {
  using namespace engine::components;
  using namespace watch_bot;

  const auto isOnScreen = entity.component<Active>()->mIsOnScreen;
  if (isOnScreen) {
    d.mpServiceProvider->playSound(data::SoundId::DukeJumping);
  }

  engine::startAnimationSequence(entity, LAND_ON_GROUND_ANIM);
  entity.component<MovingBody>()->mGravityAffected = false;
  mState = OnGround{};
  advanceRandomNumberGenerator(d);

  engine::synchronizeBoundingBoxToSprite(entity);
}


void WatchBotCarrier::update(
  GlobalDependencies& d,
  GlobalState& s,
  const bool isOnScreen,
  entityx::Entity entity
) {
  using namespace engine::components;

  const auto& position = *entity.component<WorldPosition>();
  const auto& playerPos = s.mpPlayer->position();

  auto& animationFrame = entity.component<Sprite>()->mFramesToRender[0];

  auto playerInRange = [&]() {
    return std::abs(playerPos.x - position.x) <= 5;
  };

  auto move = [&, this](const int movement) {
    const auto result = engine::moveHorizontally(
      *d.mpCollisionChecker, entity, movement);
    if (result != engine::MovementResult::Completed) {
      mState = State::ReleasingPayload;
    }

    *mPayload.component<WorldPosition>() = position + CONTAINER_OFFSET;
  };

  auto releasePayload = [&, this]() {
    mPayload.assign<components::BehaviorController>(WatchBotContainer{});
    mPayload.assign<ActivationSettings>(ActivationSettings::Policy::Always);

    mPayload.component<Sprite>()->mFramesToRender.push_back(1);
    engine::startAnimationLoop(mPayload, 1, 1, 5, 1);

    // Disassociate from payload, so that it keeps living if the carrier
    // is destroyed
    mPayload = {};
  };

  auto explode = [&]() {
    engine::reassign<components::DestructionEffects>(
      entity,
      CARRIER_SELF_DESTRUCT_EFFECT_SPEC,
      components::DestructionEffects::TriggerCondition::Manual,
      // TODO: This shouldn't be hardcoded
      BoundingBox{{}, {5, 3}});

    triggerEffects(entity, *d.mpEntityManager);
  };


  if (!mPayload && mState == State::ApproachingPlayer) {
    mPayload = d.mpEntityFactory->createSprite(
      data::ActorID::Watchbot_container, position + CONTAINER_OFFSET, true);
  }

  switch (mState) {
    case State::ApproachingPlayer:
      if (playerInRange()) {
        mState = State::ReleasingPayload;
      } else {
        const auto shouldMoveRight = position.x < playerPos.x;
        if (shouldMoveRight) {
          // This is asymmetrical with the else branch, but it's like this
          // in the original code.
          if (position.x + 3 < playerPos.x) {
            move(1);
          }
        } else {
          move(-1);
        }
      }
      break;

    case State::ReleasingPayload:
      ++mFramesElapsed;
      if (mFramesElapsed == 6) {
        animationFrame = 1;
      } else if (mFramesElapsed == 7) {
        releasePayload();
      } else if (mFramesElapsed == 20) {
        animationFrame = 0;
      } else if (mFramesElapsed == 34) {
        explode();
        entity.destroy();
      }
      break;
  }

  if (entity) {
    engine::synchronizeBoundingBoxToSprite(entity);
  }
}


void WatchBotCarrier::onKilled(
  GlobalDependencies& d,
  GlobalState& s,
  const base::Point<float>&,
  entityx::Entity entity
) {
  if (mPayload) {
    mPayload.destroy();
  }
}


void WatchBotContainer::update(
  GlobalDependencies& d,
  GlobalState& s,
  const bool isOnScreen,
  entityx::Entity entity
) {
  using namespace engine::components;

  const auto& position = *entity.component<WorldPosition>();
  auto& sprite = *entity.component<Sprite>();


  if (mFramesElapsed < 10) {
    engine::moveVertically(*d.mpCollisionChecker, entity, -1);
  }

  ++mFramesElapsed;
  if (mFramesElapsed == 25) {
    sprite.flashWhite();
    sprite.mFramesToRender[0] = engine::IGNORE_RENDER_SLOT;

    spawnMovingEffectSprite(
      *d.mpEntityFactory,
      data::ActorID::Watchbot_container_debris_1,
      SpriteMovement::FlyLeft,
      position);
    spawnMovingEffectSprite(
      *d.mpEntityFactory,
      data::ActorID::Watchbot_container_debris_2,
      SpriteMovement::FlyRight,
      position);
    d.mpServiceProvider->playSound(data::SoundId::DukeAttachClimbable);

    d.mpEntityFactory->createActor(data::ActorID::Watchbot, position + base::Vector{1, 3});

    entity.destroy();
  }
}

}
