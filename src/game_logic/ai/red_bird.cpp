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


namespace rigel { namespace game_logic { namespace ai {

using namespace engine::components;


namespace {

const int FLY_ANIMATION_L[] = {0, 1, 2, 1};
const int FLY_ANIMATION_R[] = {3, 4, 5, 4};
const int HOVER_ANIMATION[] = {6, 7};


void fly(entityx::Entity entity, const bool left) {
  engine::startAnimationSequence(
    entity,
    left ? FLY_ANIMATION_L : FLY_ANIMATION_R,
    0,
    true);
  entity.component<MovingBody>()->mVelocity.x = left ? -1.0f : 1.0f;
}

}


void configureRedBird(entityx::Entity entity) {
  using namespace engine::components::parameter_aliases;
  entity.assign<MovingBody>(Velocity{-1.0f, 0.0f}, GravityAffected{false});
  entity.assign<ActivationSettings>(
    ActivationSettings::Policy::AlwaysAfterFirstActivation);
  entity.assign<components::RedBird>();

  engine::startAnimationSequence(entity, FLY_ANIMATION_L, 0, true);
}


RedBirdSystem::RedBirdSystem(
  entityx::Entity player,
  entityx::EventManager& events
)
  : mPlayer(player)
{
  events.subscribe<engine::events::CollidedWithWorld>(*this);
}


void RedBirdSystem::update(entityx::EntityManager& es) {
  es.each<components::RedBird, WorldPosition, MovingBody, Active>(
    [this](
      entityx::Entity entity,
      components::RedBird& birdState,
      WorldPosition& position,
      MovingBody& body,
      const Active&
    ) {
      using namespace components::detail;

      auto& sprite = *entity.component<Sprite>();

      const auto& playerPosition = *mPlayer.component<WorldPosition>();
      base::match(birdState.mState,
        [&](const Flying&) {
          const auto wantsToAttack =
            position.y + 2 < playerPosition.y &&
            position.x > playerPosition.x &&
            position.x < playerPosition.x + 2;
          if (wantsToAttack) {
            birdState.mState = Hovering{};
            body.mVelocity = {};
            engine::startAnimationSequence(entity, HOVER_ANIMATION, 0, true);
          }
        },

        [&](Hovering& state) {
          ++state.mFramesElapsed;
          if (state.mFramesElapsed >= 6) {
            birdState.mState = PlungingDown{position.y};
            body.mGravityAffected = true;
            entity.remove<AnimationSequence>();
            sprite.mFramesToRender[0] = 6;
          }
        },

        [&](const PlungingDown&) {
          // no-op, state transition handled by collision event handler
        },

        [&](RisingUp& state) {
          if (state.mBackAtOriginalHeight) {
            birdState.mState = Flying{};
            const auto flyingLeft = !mIsOddFrame;
            fly(entity, flyingLeft);
            return;
          }

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
    });

  mIsOddFrame = !mIsOddFrame;
}


void RedBirdSystem::receive(const engine::events::CollidedWithWorld& event) {
  using namespace components::detail;

  auto entity = event.mEntity;
  if (!entity.has_component<components::RedBird>()) {
    return;
  }

  auto& body = *entity.component<MovingBody>();
  auto& birdState = *entity.component<components::RedBird>();

  base::match(birdState.mState,
    [&](const Flying&) {
      if (event.mCollidedLeft || event.mCollidedRight) {
        fly(entity, !event.mCollidedLeft);
      }
    },

    [&](const PlungingDown& state) {
      if (event.mCollidedBottom) {
        birdState.mState = RisingUp{state.mInitialHeight};
        body.mGravityAffected = false;
        engine::startAnimationSequence(entity, HOVER_ANIMATION, 0, true);
      }
    },

    [](const Hovering&) {},
    [](const RisingUp&) {});
}

}}}
