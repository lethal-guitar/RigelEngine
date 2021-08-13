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
#include "game_logic/damage_components.hpp"
#include "game_logic/ientity_factory.hpp"


namespace rigel::game_logic::behaviors
{

using namespace engine::components;
using namespace engine::orientation;


namespace
{

constexpr auto SHAKE_OFF_THRESHOLD = 2;


auto floorWalkerConfig()
{
  static auto config = []() {
    behaviors::SimpleWalker::Configuration c;
    c.mAnimStart = 3;
    c.mAnimEnd = 5;
    return c;
  }();

  return &config;
}


auto ceilingWalkerConfig()
{
  static auto config = []() {
    behaviors::SimpleWalker::Configuration c;
    c.mAnimStart = 0;
    c.mAnimEnd = 2;
    c.mWalkOnCeiling = true;
    return c;
  }();

  return &config;
}


int baseFrameForClinging(const SpiderClingPosition where)
{
  switch (where)
  {
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
  const Orientation playerOrientation)
{
  const auto playerFacingRight = playerOrientation == Orientation::Right;
  switch (where)
  {
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

} // namespace


void Spider::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool isOnScreen,
  entityx::Entity entity)
{
  auto& position = *entity.component<WorldPosition>();
  const auto& bbox = *entity.component<BoundingBox>();
  auto& sprite = *entity.component<Sprite>();
  const auto worldSpaceBox = engine::toWorldSpace(bbox, position);
  const auto& playerPosition = s.mpPlayer->orientedPosition();
  const auto playerOrientation = s.mpPlayer->orientation();

  auto stopWalking = [&]() {
    mWalkerBehavior.mpConfig = nullptr;
  };

  auto tryClingToPlayer = [&, this](const SpiderClingPosition clingPos) {
    // clang-format off
    if (
      s.mpPlayer->hasSpiderAt(clingPos) ||
      s.mpPlayer->isDead() ||
      s.mpPlayer->isCloaked())
    // clang-format on
    {
      return false;
    }

    s.mpPlayer->attachSpider(clingPos);
    mState = State::ClingingToPlayer;
    mPreviousPlayerOrientation = s.mpPlayer->orientation();
    mClingPosition = clingPos;

    stopWalking();
    entity.remove<game_logic::components::Shootable>();
    entity.remove<MovingBody>();
    return true;
  };

  auto walkOnCeiling = [&, this]() {
    mState = State::OnCeiling;
    sprite.mFramesToRender[0] = 0;

    // If the spider is floating in the air upon spawning, do not animate.
    if (d.mpCollisionChecker->isTouchingCeiling(worldSpaceBox))
    {
      mWalkerBehavior.mpConfig = ceilingWalkerConfig();
    }
  };

  auto isTouchingPlayer = [&, this]() {
    return worldSpaceBox.intersects(s.mpPlayer->worldSpaceHitBox());
  };

  auto detachAndDestroy = [&, this]() {
    s.mpPlayer->detachSpider(mClingPosition);
    entity.destroy();
  };

  auto fallOff = [&, this]() {
    using M = SpriteMovement;
    const auto movementType =
      d.mpRandomGenerator->gen() % 2 != 0 ? M::FlyUpperLeft : M::FlyUpperRight;
    spawnMovingEffectSprite(
      *d.mpEntityFactory,
      data::ActorID::Spider_shaken_off,
      movementType,
      position);
    detachAndDestroy();
  };

  auto startFalling = [&, this]() {
    sprite.mFramesToRender[0] = 6;
    stopWalking();
    mState = State::Falling;
    entity.component<MovingBody>()->mGravityAffected = true;
  };

  auto clingToPlayer = [&, this]() {
    *entity.component<Orientation>() = playerOrientation;
    position =
      playerPosition + offsetForClinging(mClingPosition, playerOrientation);
    sprite.mFramesToRender[0] =
      baseFrameForClinging(mClingPosition) + d.mpRandomGenerator->gen() % 2;
  };

  auto updateShakeOff = [&, this]() {
    const auto playerTurnedThisFrame =
      playerOrientation != mPreviousPlayerOrientation;
    mPreviousPlayerOrientation = playerOrientation;

    if (playerTurnedThisFrame)
    {
      ++mShakeOffProgress;
    }
    else if (s.mpPerFrameState->mIsOddFrame && mShakeOffProgress > 0)
    {
      --mShakeOffProgress;
    }

    if (mShakeOffProgress >= SHAKE_OFF_THRESHOLD)
    {
      fallOff();
    }
  };

  switch (mState)
  {
    case State::Uninitialized:
      if (d.mpCollisionChecker->isOnSolidGround(worldSpaceBox))
      {
        walkOnFloor(entity);
      }
      else
      {
        walkOnCeiling();
      }
      break;

    case State::OnCeiling:
      if (position.x == playerPosition.x && position.y < playerPosition.y - 3)
      {
        startFalling();
      }
      break;

    case State::Falling:
      if (isTouchingPlayer())
      {
        tryClingToPlayer(SpiderClingPosition::Head);
      }
      break;

    case State::OnFloor:
      if (isTouchingPlayer())
      {
        const auto success = tryClingToPlayer(SpiderClingPosition::Weapon);
        if (!success)
        {
          tryClingToPlayer(SpiderClingPosition::Back);
        }
      }
      break;

    case State::ClingingToPlayer:
      if (s.mpPlayer->isIncapacitated())
      {
        detachAndDestroy();
        return;
      }

      if (s.mpPlayer->isDead())
      {
        fallOff();
      }
      else
      {
        clingToPlayer();
        updateShakeOff();
      }
      break;

    default:
      break;
  }

  if (entity && mWalkerBehavior.mpConfig)
  {
    mWalkerBehavior.update(d, s, isOnScreen, entity);
  }
}


void Spider::onCollision(
  GlobalDependencies& d,
  GlobalState& s,
  const engine::events::CollidedWithWorld& event,
  entityx::Entity entity)
{
  using engine::components::Orientation;

  if (mState == State::Falling)
  {
    walkOnFloor(entity);
    *entity.component<Orientation>() = Orientation::Right;
  }
}


void Spider::walkOnFloor(entityx::Entity entity)
{
  mState = State::OnFloor;

  auto& sprite = *entity.component<Sprite>();
  sprite.mFramesToRender[0] = 3;

  mWalkerBehavior.mpConfig = floorWalkerConfig();
}

} // namespace rigel::game_logic::behaviors
