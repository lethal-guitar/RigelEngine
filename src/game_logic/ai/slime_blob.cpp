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

#include "slime_blob.hpp"

#include "engine/base_components.hpp"
#include "engine/collision_checker.hpp"
#include "engine/movement.hpp"
#include "engine/physical_components.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/entity_factory.hpp"
#include "game_logic/player.hpp"

RIGEL_DISABLE_WARNINGS
#include <atria/variant/match_boost.hpp>
RIGEL_RESTORE_WARNINGS


namespace rigel { namespace game_logic { namespace ai {

using namespace engine::components;
using namespace engine::orientation;
using engine::CollisionChecker;
using game_logic::components::Shootable;


namespace {

const auto NUM_BREAK_ANIMATION_STEPS = 15;
const auto BREAK_ANIM_SPEED = 3; // frames between visible animation steps

const auto SLIME_BLOB_SPAWN_OFFSET = base::Vector{2, 0};

const auto SPRITE_ORIENTATION_OFFSET = 5;
const auto WALKING_ON_GROUND_BASE_FRAME = 3;

const auto STRETCH_UP_ANIM_START = 10;
const auto CONTRACT_DOWN_ANIM_START = 12;
const auto CONTRACT_DOWN_ANIM_END = 10;
const auto IN_FLIGHT_ANIM_FRAME = 13;
const auto CONTRACT_UP_ANIM_START = 14;
const auto CONTRACT_UP_ANIM_END = 16;
const auto STRETCH_DOWN_ANIM_END = 14;


int toOffset(const Orientation orientation) {
  return orientation == Orientation::Right ? SPRITE_ORIENTATION_OFFSET : 0;
}

}


void configureSlimeContainer(entityx::Entity entity) {
  // Render slots: Main part, roof, animated glass contents
  entity.component<Sprite>()->mFramesToRender = {2, 8, 0};
  entity.assign<BoundingBox>(BoundingBox{{1, -2}, {3, 3}});
  entity.assign<components::SlimeContainer>();

  entity.component<Shootable>()->mDestroyWhenKilled = false;
}


SlimeBlobSystem::SlimeBlobSystem(
  const Player* pPlayer,
  CollisionChecker* pCollisionChecker,
  EntityFactory* pEntityFactory,
  engine::RandomNumberGenerator* pRandomGenerator,
  entityx::EventManager& events
)
  : mpPlayer(pPlayer)
  , mpCollisionChecker(pCollisionChecker)
  , mpEntityFactory(pEntityFactory)
  , mpRandomGenerator(pRandomGenerator)
{
  events.subscribe<events::ShootableKilled>(*this);
}


void SlimeBlobSystem::update(entityx::EntityManager& es) {
  using engine::components::Active;
  using engine::components::WorldPosition;

  // Slime containers
  es.each<Sprite, WorldPosition, components::SlimeContainer, Active>(
    [this](
      entityx::Entity entity,
      Sprite& sprite,
      const WorldPosition& position,
      components::SlimeContainer& state,
      const engine::components::Active&
    ) {
      const auto stillIntact = entity.has_component<Shootable>();
      if (stillIntact) {
        // Animate slime blob inside
        sprite.mFramesToRender[2] = mpRandomGenerator->gen() % 2;
      } else {
        ++state.mBreakAnimationStep;
        if (state.mBreakAnimationStep >= NUM_BREAK_ANIMATION_STEPS) {
          entity.remove<components::SlimeContainer>();
          entity.remove<BoundingBox>();
          entity.remove<Active>();

          mpEntityFactory->createActor(67, position + SLIME_BLOB_SPAWN_OFFSET);
        }

        const auto visibleFrame =
          state.mBreakAnimationStep / BREAK_ANIM_SPEED;
        sprite.mFramesToRender[0] = 2 + visibleFrame;
      }
    });


  // Slime blobs
  es.each<Sprite, WorldPosition, BoundingBox, components::SlimeBlob, Active>(
    [this](
      entityx::Entity entity,
      Sprite& sprite,
      WorldPosition& position,
      const BoundingBox& bbox,
      components::SlimeBlob& blobState,
      const engine::components::Active&
    ) {
      using namespace components::detail;

      const auto playerPosition = mpPlayer->orientedPosition();
      atria::variant::match(blobState.mState,
        [&](OnGround& state) {
          // Animate walking
          state.mIsOddUpdate = !state.mIsOddUpdate;

          const auto newAnimFrame =
            WALKING_ON_GROUND_BASE_FRAME +
            (state.mIsOddUpdate ? 1 : 0) +
            toOffset(blobState.mOrientation);
          sprite.mFramesToRender[0] = newAnimFrame;

          // Decide if we should continue walking or change state
          const auto isFacingLeft =
            blobState.mOrientation == Orientation::Left;
          const auto movingTowardsPlayer =
            (isFacingLeft && position.x >= playerPosition.x) ||
            (!isFacingLeft && position.x <= playerPosition.x);

          if (movingTowardsPlayer) {
            if (newAnimFrame % 2 == 1) {
              const auto walkedSuccessfully = engine::walk(
                *mpCollisionChecker,
                entity,
                blobState.mOrientation);

              if (!walkedSuccessfully) {
                blobState.mState = Idle{};
              }
            }
          } else {
            blobState.mState = Idle{};
          }
        },


        [&](OnCeiling& state) {
          // Check if we are above the player, and fall back down if so
          if (position.x == playerPosition.x) {
            blobState.mState = Descending{};
            return;
          }

          // Animate
          state.mIsOddUpdate = !state.mIsOddUpdate;
          const auto playerIsRight = position.x <= playerPosition.x;
          const auto baseFrame = playerIsRight ? 19 : 17;
          sprite.mFramesToRender[0] = baseFrame + (state.mIsOddUpdate ? 1 : 0);

          // Move
          if (state.mIsOddUpdate) {
            const auto orientationForMovement = playerIsRight
              ? Orientation::Right
              : Orientation::Left;
            const auto walkedSuccessfully = engine::walkOnCeiling(
              *mpCollisionChecker,
              entity,
              orientationForMovement);

            if (!walkedSuccessfully) {
              sprite.mFramesToRender[0] -= 2;
              blobState.mState = Descending{};
            }
          }
        },


        [&](Idle& state) {
          // Randomly decide to fly up
          if ((mpRandomGenerator->gen() % 32) == 0) {
            blobState.mState = Ascending{};
            sprite.mFramesToRender[0] = STRETCH_UP_ANIM_START;
            return;
          }

          // Animate
          const auto newAnimFrame =
            mpRandomGenerator->gen() % 4 + toOffset(blobState.mOrientation);
          sprite.mFramesToRender[0] = newAnimFrame;

          // Wait until time-out elapsed
          ++state.mFramesElapsed;
          if (state.mFramesElapsed >= 10) {
            // Orient towards player and back to movement
            const auto newOrientation = position.x <= playerPosition.x
              ? Orientation::Right
              : Orientation::Left;
            blobState.mOrientation = newOrientation;

            blobState.mState = OnGround{};
          }
        },


        [&](Ascending& state) {
          auto& animationFrame = sprite.mFramesToRender[0];

          if (animationFrame < IN_FLIGHT_ANIM_FRAME) {
            // Animate getting ready to fly up (stretch upwards). Assumes we
            // previously set the animation frame to STRETCH_UP_ANIM_START
            ++animationFrame;
          } else if (animationFrame == IN_FLIGHT_ANIM_FRAME) {
            // Fly upwards
            const auto willCollide =
              mpCollisionChecker->isTouchingCeiling(position, bbox);
            if (willCollide) {
              animationFrame = CONTRACT_UP_ANIM_START;
            }

            // always move, even when colliding. This is ok because the next
            // anim frame has an offset which makes us not collide anymore.
            position.y -= 1;
          } else {
            // Animate arrival on ceiling (contract)
            ++animationFrame;
            if (animationFrame >= CONTRACT_UP_ANIM_END) {
              blobState.mState = OnCeiling{};
            }
          }
        },


        [&](Descending& state) {
          auto& animationFrame = sprite.mFramesToRender[0];

          if (animationFrame == IN_FLIGHT_ANIM_FRAME) {
            const auto offset = base::Vector{0, 3};

            const auto willCollide =
              mpCollisionChecker->isOnSolidGround(position + offset, bbox);
            if (willCollide) {
              animationFrame = CONTRACT_DOWN_ANIM_START;
            }

            // always move, even when colliding. This is ok because the next
            // anim frame has an offset which makes us not collide anymore.
            position.y += 1;
          } else {
            if (animationFrame == STRETCH_DOWN_ANIM_END) {
              position.y += 1;
            }

            --animationFrame;
            if (animationFrame <= CONTRACT_DOWN_ANIM_END) {
              animationFrame = 0;
              blobState.mState = Idle{};
              blobState.mOrientation = Orientation::Left;
            }
          }
        });

      engine::synchronizeBoundingBoxToSprite(entity);
    });
}


void SlimeBlobSystem::receive(const events::ShootableKilled& event) {
  auto entity = event.mEntity;
  if (
    !entity.has_component<components::SlimeContainer>() ||
    !entity.has_component<Shootable>()
  ) {
    return;
  }

  auto& sprite = *entity.component<Sprite>();
  sprite.mFramesToRender.pop_back();
  sprite.mFramesToRender[0] = 2;

  sprite.flashWhite();
}

}}}
