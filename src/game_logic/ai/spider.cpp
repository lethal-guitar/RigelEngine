/* Copyright (C) 2018, Nikolai Wuttke. All rights reserved.
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

#include "spider.hpp"

#include "engine/collision_checker.hpp"
#include "engine/entity_tools.hpp"
#include "engine/movement.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/ai/simple_walker.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/entity_factory.hpp"


namespace rigel { namespace game_logic { namespace ai {

using namespace engine::components;
using namespace engine::orientation;
using engine::CollisionChecker;


namespace {

constexpr auto SHAKE_OFF_THRESHOLD = 2;


auto floorWalkerConfig() {
  static auto config = []() {
    ai::components::SimpleWalker::Configuration c;
    c.mAnimStart = 3;
    c.mAnimEnd = 5;
    return c;
  }();

  return &config;
}


auto ceilingWalkerConfig() {
  static auto config = []() {
    ai::components::SimpleWalker::Configuration c;
    c.mAnimStart = 0;
    c.mAnimEnd = 2;
    c.mWalkOnCeiling = true;
    return c;
  }();

  return &config;
}


int baseFrameForClinging(const SpiderClingPosition where) {
  switch (where) {
    case SpiderClingPosition::Head:
      return 7;

    case SpiderClingPosition::Weapon:
      return 11;

    case SpiderClingPosition::Back:
      return 9;
  }

  assert(false);
  return 0;
}


base::Vector offsetForClinging(
  const SpiderClingPosition where,
  const Orientation playerOrientation
) {
  const auto playerFacingRight = playerOrientation == Orientation::Right;
  switch (where) {
    case SpiderClingPosition::Head:
      return playerFacingRight ? base::Vector{0, -3} : base::Vector{1, -3};

    case SpiderClingPosition::Weapon:
      return playerFacingRight ? base::Vector{2, -1} : base::Vector{-1, -1};

    case SpiderClingPosition::Back:
      return playerFacingRight ? base::Vector{-2, -2} : base::Vector{3, -2};
  }

  assert(false);
  return base::Vector{0, 0};
}


void walkOnFloor(components::Spider& self, entityx::Entity entity) {
  self.mState = components::Spider::State::OnFloor;

  auto& sprite = *entity.component<Sprite>();
  sprite.mFramesToRender[0] = 3;

  entity.assign<ai::components::SimpleWalker>(floorWalkerConfig());
}

}


SpiderSystem::SpiderSystem(
  Player* pPlayer,
  CollisionChecker* pCollisionChecker,
  engine::RandomNumberGenerator* pRandomGenerator,
  IEntityFactory* pEntityFactory,
  entityx::EventManager& events
)
  : mpPlayer(pPlayer)
  , mpCollisionChecker(pCollisionChecker)
  , mpRandomGenerator(pRandomGenerator)
  , mpEntityFactory(pEntityFactory)
{
  events.subscribe<engine::events::CollidedWithWorld>(*this);
}


void SpiderSystem::update(entityx::EntityManager& es) {
  using components::Spider;
  using State = Spider::State;

  es.each<Spider, Sprite, WorldPosition, BoundingBox, Active>(
    [this](
      entityx::Entity entity,
      Spider& self,
      Sprite& sprite,
      WorldPosition& position,
      const BoundingBox& bbox,
      const Active&
    ) {
      const auto worldSpaceBox = engine::toWorldSpace(bbox, position);
      const auto& playerPosition = mpPlayer->orientedPosition();
      const auto playerOrientation = mpPlayer->orientation();

      auto tryClingToPlayer = [&, this](const SpiderClingPosition clingPos) {
        if (mpPlayer->hasSpiderAt(clingPos) || mpPlayer->isDead()) {
          return false;
        }

        mpPlayer->attachSpider(clingPos);
        self.mState = State::ClingingToPlayer;
        self.mPreviousPlayerOrientation = mpPlayer->orientation();
        self.mClingPosition = clingPos;

        engine::removeSafely<ai::components::SimpleWalker>(entity);
        entity.remove<game_logic::components::Shootable>();
        entity.remove<MovingBody>();
        return true;
      };

      auto walkOnCeiling = [&, this]() {
        self.mState = State::OnCeiling;
        entity.assign<ai::components::SimpleWalker>(ceilingWalkerConfig());
        sprite.mFramesToRender[0] = 0;
      };

      auto isTouchingPlayer = [&, this]() {
        return worldSpaceBox.intersects(mpPlayer->worldSpaceHitBox());
      };

      auto fallOff = [&, this]() {
        using M = SpriteMovement;
        const auto movementType = mpRandomGenerator->gen() % 2 != 0
          ? M::FlyUpperLeft
          : M::FlyUpperRight;
        spawnMovingEffectSprite(*mpEntityFactory, 232, movementType, position);
        mpPlayer->detachSpider(self.mClingPosition);
        entity.destroy();
      };

      auto startFalling = [&, this]() {
        sprite.mFramesToRender[0] = 6;
        entity.remove<ai::components::SimpleWalker>();
        self.mState = State::Falling;
        entity.component<MovingBody>()->mGravityAffected = true;
      };

      auto clingToPlayer = [&, this]() {
        *entity.component<Orientation>() = playerOrientation;
        position = playerPosition +
          offsetForClinging(self.mClingPosition, playerOrientation);
        sprite.mFramesToRender[0] =
          baseFrameForClinging(self.mClingPosition) +
          mpRandomGenerator->gen() % 2;
      };

      auto updateShakeOff = [&, this]() {
        const auto playerTurnedThisFrame =
          playerOrientation != self.mPreviousPlayerOrientation;
        self.mPreviousPlayerOrientation = playerOrientation;

        if (playerTurnedThisFrame) {
          ++self.mShakeOffProgress;
        } else if (mIsOddFrame && self.mShakeOffProgress > 0) {
          --self.mShakeOffProgress;
        }

        if (self.mShakeOffProgress >= SHAKE_OFF_THRESHOLD) {
          fallOff();
        }
      };

      switch (self.mState) {
        case State::Uninitialized:
          if (mpCollisionChecker->isOnSolidGround(worldSpaceBox)) {
            walkOnFloor(self, entity);
          } else {
            walkOnCeiling();
          }
          break;

        case State::OnCeiling:
          if (
            position.x == playerPosition.x &&
            position.y < playerPosition.y - 3
          ) {
            startFalling();
          }
          break;

        case State::Falling:
          if (isTouchingPlayer()) {
            tryClingToPlayer(SpiderClingPosition::Head);
          }
          break;

        case State::OnFloor:
          if (isTouchingPlayer()) {
            const auto success = tryClingToPlayer(SpiderClingPosition::Weapon);
            if (!success) {
              tryClingToPlayer(SpiderClingPosition::Back);
            }
          }
          break;

        case State::ClingingToPlayer:
          // TODO: Disappear when player gets eaten

          if (mpPlayer->isDead()) {
            fallOff();
          } else {
            clingToPlayer();
            updateShakeOff();
          }
          break;

        default:
          break;
      }
    });

  mIsOddFrame = !mIsOddFrame;
}


void SpiderSystem::receive(const engine::events::CollidedWithWorld& event) {
  using State = components::Spider::State;

  auto entity = event.mEntity;
  if (!entity.has_component<components::Spider>()) {
    return;
  }

  auto& self = *entity.component<components::Spider>();
  if (self.mState == State::Falling) {
    walkOnFloor(self, entity);
  }
}

}}}
