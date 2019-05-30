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

#include "boss_episode_1.hpp"

#include "base/match.hpp"
#include "common/game_service_provider.hpp"
#include "common/global_level_events.hpp"
#include "data/player_model.hpp"
#include "engine/collision_checker.hpp"
#include "engine/movement.hpp"
#include "engine/particle_system.hpp"
#include "engine/physical_components.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/entity_factory.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic::behaviors {

namespace {

constexpr auto BOSS_KILL_SCORE = 50'000;

constexpr auto BOMB_DROP_OFFSET = base::Vector{3, 1};

constexpr auto ZIG_ZAG_VERTICAL_MOVEMENT_SEQUENCE = std::array<int, 10>{
  -1, -1, 0, 0, 1, 1, 1, 0, 0, -1 };

}


void BossEpisode1::update(
  GlobalDependencies& d,
  GlobalState& s,
  const bool isOnScreen,
  entityx::Entity entity
) {
  using namespace boss_episode_1;
  using namespace engine::components;
  using boss_episode_1::Dieing;

  auto& position = *entity.component<WorldPosition>();
  auto& body = *entity.component<MovingBody>();
  auto& sprite = *entity.component<Sprite>();
  auto& playerPos = s.mpPlayer->position();

  auto startSlammingDown = [&, this]() {
    const auto isTouchingGround = d.mpCollisionChecker->isOnSolidGround(
      position,
      *entity.component<BoundingBox>());
    if (isTouchingGround) {
      d.mpServiceProvider->playSound(data::SoundId::HammerSmash);
      mState = RisingBackUp{};
    } else {
      mState = SlammingDown{};
      body.mGravityAffected = true;
    }
  };


  base::match(mState,
    [&, this](const AwaitingActivation&) {
      d.mpEvents->emit(rigel::events::BossActivated{entity});
      mStartingAltitude = position.y;
      startSlammingDown();
    },

    [&, this](const RisingBackUp&) {
      if (position.y <= mStartingAltitude) {
        mState = FlyingLeftOnUpperLevel{};
      } else {
        --position.y;
      }
    },

    [&, this](const FlyingLeftOnUpperLevel&) {
      const auto result = engine::moveHorizontally(
        *d.mpCollisionChecker,
        entity,
        -2);
      if (result != engine::MovementResult::Completed) {
        mState = FlyingRightDroppingBombs{};
      }
    },

    [&, this](const FlyingRightDroppingBombs&) {
      if (s.mpPerFrameState->mIsOddFrame) {
        d.mpEntityFactory->createActor(76, position + BOMB_DROP_OFFSET);
      }
      const auto result = engine::moveHorizontally(
        *d.mpCollisionChecker,
        entity,
        2);
      if (result != engine::MovementResult::Completed) {
        mState = MovingDownOnRightSide{};
        body.mGravityAffected = true;
      }
    },

    [&, this](const FlyingLeftOnLowerLevel&) {
       const auto result = engine::moveHorizontally(
        *d.mpCollisionChecker,
        entity,
        -2);
      if (result != engine::MovementResult::Completed) {
        mState = MovingUpOnLeftSide{};
      }
    },

    [&, this](const MovingUpOnLeftSide&) {
      if (position.y <= mStartingAltitude) {
        mState = ZigZagging{};
      } else {
        --position.y;
      }
    },

    [&, this](ZigZagging& state) {
      const auto result = engine::moveHorizontally(
        *d.mpCollisionChecker,
        entity,
        engine::orientation::toMovement(state.mOrientation));
      if (result != engine::MovementResult::Completed) {
        state.mOrientation =
          engine::orientation::opposite(state.mOrientation);
      }

      position.y += ZIG_ZAG_VERTICAL_MOVEMENT_SEQUENCE[
        state.mFramesElapsed % ZIG_ZAG_VERTICAL_MOVEMENT_SEQUENCE.size()];

      ++state.mFramesElapsed;
      if (
        state.mFramesElapsed > 50 &&
        position.x - 1 <= playerPos.x &&
        position.x + 9 >= playerPos.x
      ) {
        startSlammingDown();
      }
    },

    [&, this](Dieing& state) {
      auto bigExplosionEffect = [&]() {
        d.mpEvents->emit(rigel::events::ScreenFlash{});
        d.mpServiceProvider->playSound(data::SoundId::BigExplosion);
      };

      auto rand = [&]() {
        return d.mpRandomGenerator->gen();
      };

      auto randomExplosionSound = [&]() {
        // TODO: Eliminate duplication with code in effects_system.cpp
        const auto randomChoice = d.mpRandomGenerator->gen();
        const auto soundId = randomChoice % 2 == 0
          ? data::SoundId::AlternateExplosion
          : data::SoundId::Explosion;
        d.mpServiceProvider->playSound(soundId);
      };


      if (state.mFramesElapsed == 0) {
        d.mpServiceProvider->stopMusic();
        s.mpPlayer->model().giveScore(BOSS_KILL_SCORE);
        body.mGravityAffected = false;
        sprite.mFramesToRender[1] = 3;
      }

      switch (state.mFramesElapsed) {
        case 1: case 5: case 12: case 14: case 19: case 23: case 25: case 28:
        case 30: case 34: case 38: case 41: case 46: case 48:
          randomExplosionSound();
          d.mpParticles->spawnParticles(
            position + base::Vector{rand() % 4, -(rand() % 8)},
            loader::INGAME_PALETTE[rand() % 16],
            rand() % 2 - 1);
          spawnOneShotSprite(
            *d.mpEntityFactory,
            1,
            position + base::Vector{rand() % 4, -(rand() % 8)});
          spawnMovingEffectSprite(
            *d.mpEntityFactory,
            3,
            SpriteMovement::FlyDown,
            position + base::Vector{rand() % 4, -(rand() % 8)});
          break;

        default:
          break;
      }

      if (state.mFramesElapsed < 48) {
        sprite.mShow = !s.mpPerFrameState->mIsOddFrame;

        if ((rand() / 4) % 2 != 0 && s.mpPerFrameState->mIsOddFrame) {
          bigExplosionEffect();
        } else {
          randomExplosionSound();
        }
      } else if (state.mFramesElapsed == 48) {
        sprite.mShow = true;
        bigExplosionEffect();
      } else { // > 50
        if (position.y > 3) {
          position.y -= 2;
        }
      }

      if (state.mFramesElapsed == 58) {
        d.mpEvents->emit(rigel::events::ExitReached{false});
      }

      ++state.mFramesElapsed;
    },

    [](const auto&) {});
}


void BossEpisode1::onCollision(
  GlobalDependencies& d,
  GlobalState&,
  const engine::events::CollidedWithWorld&,
  entityx::Entity entity
) {
  using namespace boss_episode_1;
  using namespace engine::components;

  auto& body = *entity.component<MovingBody>();

  base::match(mState,
    [&, this](const SlammingDown&) {
      body.mGravityAffected = false;
      d.mpServiceProvider->playSound(data::SoundId::HammerSmash);
      mState = RisingBackUp{};
    },

    [&, this](const MovingDownOnRightSide&) {
      body.mGravityAffected = false;
      mState = FlyingLeftOnLowerLevel{};
    },

    [](const auto&) {});
}


void BossEpisode1::onKilled(
  GlobalDependencies&,
  GlobalState&,
  const base::Point<float>&,
  entityx::Entity entity
) {
  engine::removeSafely<game_logic::components::PlayerDamaging>(entity);
  mState = boss_episode_1::Dieing{};
}

}
