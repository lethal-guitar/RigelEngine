/* Copyright (C) 2017, Nikolai Wuttke. All rights reserved.
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

#include "red_bird.hpp"

#include "base/match.hpp"
#include "engine/base_components.hpp"
#include "engine/collision_checker.hpp"
#include "engine/physical_components.hpp"
#include "engine/sprite_tools.hpp"
#include "game_logic/behavior_controller.hpp"
#include "game_logic/player.hpp"


using namespace rigel::engine::components;


namespace {

constexpr int FLY_ANIMATION[] = {0, 1, 2, 1};
constexpr int HOVER_ANIMATION[] = {6, 7};

constexpr int FLY_ANIMATION_ORIENTATION_OFFSET = 3;

}


void rigel::game_logic::configureRedBird(entityx::Entity entity) {
  using namespace engine::components::parameter_aliases;
  entity.assign<MovingBody>(Velocity{}, GravityAffected{false});
  entity.assign<ActivationSettings>(
    ActivationSettings::Policy::AlwaysAfterFirstActivation);
  entity.assign<components::BehaviorController>(behaviors::RedBird{});
}


namespace rigel::game_logic::behaviors {

void RedBird::update(
  GlobalDependencies& d,
  GlobalState& s,
  const bool isOnScreen,
  entityx::Entity entity
) {
  auto& position = *entity.component<WorldPosition>();
  auto& body = *entity.component<MovingBody>();
  auto& sprite = *entity.component<Sprite>();

  const auto& playerPosition = s.mpPlayer->orientedPosition();
  base::match(mState,
    [&, this](Flying& state) {
      // clang-format off
      const auto wantsToAttack =
        position.y + 2 < playerPosition.y &&
        position.x > playerPosition.x &&
        position.x < playerPosition.x + 2;
      // clang-format on

      if (wantsToAttack) {
        sprite.mFramesToRender[0] =
          HOVER_ANIMATION[int(s.mpPerFrameState->mIsOddFrame)];
        mState = Hovering{};
      } else {
        const auto result = engine::moveHorizontally(
          *d.mpCollisionChecker,
          entity,
          engine::orientation::toMovement(state.mOrientation));
        if (result != engine::MovementResult::Completed) {
          state.mOrientation =
            engine::orientation::opposite(state.mOrientation);
        } else {
          // Animate flying
          ++state.mAnimStep;
          sprite.mFramesToRender[0] =
            FLY_ANIMATION[state.mAnimStep % 4] +
              (state.mOrientation == Orientation::Right
                ? FLY_ANIMATION_ORIENTATION_OFFSET : 0);
        }
      }
    },

    [&, this](Hovering& state) {
      sprite.mFramesToRender[0] =
        HOVER_ANIMATION[int(s.mpPerFrameState->mIsOddFrame)];

      ++state.mFramesElapsed;
      if (state.mFramesElapsed >= 7) {
        mState = PlungingDown{position.y};
        body.mGravityAffected = true;
        sprite.mFramesToRender[0] = 6;
      }
    },

    [&](const PlungingDown& state) {
      const auto bbox = *entity.component<BoundingBox>();
      if (d.mpCollisionChecker->isOnSolidGround(position, bbox)) {
        startRisingUp(state.mInitialHeight, entity);
      }
    },

    [&, this](RisingUp& state) {
      if (state.mBackAtOriginalHeight) {
        const auto flyingLeft = !s.mpPerFrameState->mIsOddFrame;
        const auto newOrientation =
          flyingLeft ? Orientation::Left : Orientation::Right;
        mState = Flying{newOrientation};
        return;
      }

      sprite.mFramesToRender[0] =
        HOVER_ANIMATION[int(s.mpPerFrameState->mIsOddFrame)];

      if (position.y > state.mInitialHeight) {
        --position.y;
      } else {
        // We wait 1 frame in the air before returning to regular flying
        // mode. This is accomplished using this bool, which is checked
        // before the position check, and so will only be checked again on
        // the next frame.
        state.mBackAtOriginalHeight = true;
      }
    });

  engine::synchronizeBoundingBoxToSprite(entity);
}


void RedBird::onCollision(
  GlobalDependencies& d,
  GlobalState& s,
  const engine::events::CollidedWithWorld& event,
  entityx::Entity entity
) {
  base::match(mState,
    [&, this](const PlungingDown& state) {
      if (event.mCollidedBottom) {
        startRisingUp(state.mInitialHeight, entity);
      }
    },

    [](const auto&) {});
}


void RedBird::startRisingUp(const int initialHeight, entityx::Entity entity) {
  auto& body = *entity.component<MovingBody>();

  mState = RisingUp{initialHeight};
  body.mGravityAffected = false;
}

}
