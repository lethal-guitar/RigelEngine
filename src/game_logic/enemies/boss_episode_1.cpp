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
#include "data/player_model.hpp"
#include "engine/collision_checker.hpp"
#include "engine/movement.hpp"
#include "engine/particle_system.hpp"
#include "engine/physical_components.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "frontend/game_service_provider.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/ientity_factory.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic::behaviors
{

namespace
{

constexpr auto BOMB_DROP_OFFSET = base::Vec2{3, 1};

constexpr auto ZIG_ZAG_VERTICAL_MOVEMENT_SEQUENCE =
  std::array<int, 10>{-1, -1, 0, 0, 1, 1, 1, 0, 0, -1};

} // namespace


void BossEpisode1::update(
  GlobalDependencies& d,
  GlobalState& s,
  const bool isOnScreen,
  entityx::Entity entity)
{
  using namespace engine::components;

  auto& position = *entity.component<WorldPosition>();
  auto& body = *entity.component<MovingBody>();
  auto& playerPos = s.mpPlayer->position();

  auto startSlammingDown = [&, this]() {
    const auto isTouchingGround = d.mpCollisionChecker->isOnSolidGround(
      position, *entity.component<BoundingBox>());
    if (isTouchingGround)
    {
      d.mpServiceProvider->playSound(data::SoundId::HammerSmash);
      mState = RisingBackUp{};
    }
    else
    {
      mState = SlammingDown{};
      body.mGravityAffected = true;
    }
  };


  base::match(
    mState,
    [&, this](const AwaitingActivation&) {
      d.mpEvents->emit(rigel::events::BossActivated{entity});
      mStartingAltitude = position.y;
      startSlammingDown();
    },

    [&, this](const RisingBackUp&) {
      if (position.y <= mStartingAltitude)
      {
        mState = FlyingLeftOnUpperLevel{};
      }
      else
      {
        --position.y;
      }
    },

    [&, this](const FlyingLeftOnUpperLevel&) {
      const auto result =
        engine::moveHorizontally(*d.mpCollisionChecker, entity, -2);
      if (result != engine::MovementResult::Completed)
      {
        mState = FlyingRightDroppingBombs{};
      }
    },

    [&, this](const FlyingRightDroppingBombs&) {
      if (s.mpPerFrameState->mIsOddFrame)
      {
        d.mpEntityFactory->spawnActor(
          data::ActorID::Napalm_bomb_small, position + BOMB_DROP_OFFSET);
      }
      const auto result =
        engine::moveHorizontally(*d.mpCollisionChecker, entity, 2);
      if (result != engine::MovementResult::Completed)
      {
        mState = MovingDownOnRightSide{};
        body.mGravityAffected = true;
      }
    },

    [&, this](const FlyingLeftOnLowerLevel&) {
      const auto result =
        engine::moveHorizontally(*d.mpCollisionChecker, entity, -2);
      if (result != engine::MovementResult::Completed)
      {
        mState = MovingUpOnLeftSide{};
      }
    },

    [&, this](const MovingUpOnLeftSide&) {
      if (position.y <= mStartingAltitude)
      {
        mState = ZigZagging{};
      }
      else
      {
        --position.y;
      }
    },

    [&, this](ZigZagging& state) {
      const auto result = engine::moveHorizontally(
        *d.mpCollisionChecker,
        entity,
        engine::orientation::toMovement(state.mOrientation));
      if (result != engine::MovementResult::Completed)
      {
        state.mOrientation = engine::orientation::opposite(state.mOrientation);
      }

      position.y += ZIG_ZAG_VERTICAL_MOVEMENT_SEQUENCE
        [state.mFramesElapsed % ZIG_ZAG_VERTICAL_MOVEMENT_SEQUENCE.size()];

      ++state.mFramesElapsed;
      // clang-format off
      if (
        state.mFramesElapsed > 50 &&
        position.x - 1 <= playerPos.x &&
        position.x + 9 >= playerPos.x)
      // clang-format on
      {
        startSlammingDown();
      }
    },

    [](const auto&) {});
}


void BossEpisode1::onCollision(
  GlobalDependencies& d,
  GlobalState&,
  const engine::events::CollidedWithWorld&,
  entityx::Entity entity)
{
  using namespace engine::components;

  auto& body = *entity.component<MovingBody>();

  base::match(
    mState,
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
  GlobalDependencies& d,
  GlobalState&,
  const base::Vec2f&,
  entityx::Entity entity)
{
  using engine::components::MovingBody;
  using engine::components::Sprite;

  entity.component<MovingBody>()->mGravityAffected = false;
  entity.component<Sprite>()->mFramesToRender[1] = 3;
  engine::removeSafely<game_logic::components::PlayerDamaging>(entity);

  d.mpEvents->emit(rigel::events::BossDestroyed{entity});
}

} // namespace rigel::game_logic::behaviors
