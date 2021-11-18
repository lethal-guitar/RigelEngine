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

#include "spike_ball.hpp"

#include "common/game_service_provider.hpp"
#include "data/sound_ids.hpp"
#include "engine/collision_checker.hpp"
#include "engine/entity_tools.hpp"
#include "engine/life_time_components.hpp"
#include "engine/physical_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/global_dependencies.hpp"


namespace rigel::game_logic::behaviors
{

using namespace engine::components;

namespace
{

// clang-format off
constexpr base::Vec2f JUMP_ARC[] = {
  {0.0f, -2.0f},
  {0.0f, -2.0f},
  {0.0f, -1.0f},
  {0.0f, -1.0f},
  {0.0f, -1.0f}
};
// clang-format on

} // namespace


void SpikeBall::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool isOnScreen,
  entityx::Entity entity)
{
  auto jump = [&]() {
    mJumpBackCooldown = 9;
    engine::reassign<MovementSequence>(entity, JUMP_ARC, true, false);

    if (isOnScreen)
    {
      d.mpServiceProvider->playSound(data::SoundId::DukeJumping);
    }
  };


  if (!mInitialized)
  {
    entity.assign<MovementSequence>(JUMP_ARC, true, false);
    mInitialized = true;
  }

  if (mJumpBackCooldown > 0)
  {
    --mJumpBackCooldown;
  }

  const auto& position = *entity.component<WorldPosition>();
  const auto& bounds = *entity.component<BoundingBox>();
  const auto onSolidGround =
    d.mpCollisionChecker->isOnSolidGround(position, bounds);
  if (mJumpBackCooldown == 0 && onSolidGround)
  {
    jump();
  }
}


void SpikeBall::onHit(
  GlobalDependencies& d,
  GlobalState& s,
  entityx::Entity inflictorEntity,
  entityx::Entity entity)
{
  const auto inflictorVelocity = inflictorEntity.has_component<MovingBody>()
    ? inflictorEntity.component<MovingBody>()->mVelocity
    : base::Vec2f{};

  auto& body = *entity.component<MovingBody>();
  body.mVelocity.x = inflictorVelocity.x > 0 ? 1.0f : -1.0f;
}


void SpikeBall::onCollision(
  GlobalDependencies& d,
  GlobalState& s,
  const engine::events::CollidedWithWorld& event,
  entityx::Entity entity)
{
  auto& body = *entity.component<MovingBody>();
  if (event.mCollidedLeft)
  {
    body.mVelocity.x = 1.0f;
  }
  else if (event.mCollidedRight)
  {
    body.mVelocity.x = -1.0f;
  }

  if (event.mCollidedTop)
  {
    if (entity.component<Active>()->mIsOnScreen)
    {
      d.mpServiceProvider->playSound(data::SoundId::DukeJumping);
    }

    mJumpBackCooldown = 3;

    engine::removeSafely<MovementSequence>(entity);
    body.mVelocity.y = 0.0f;
  }
}

} // namespace rigel::game_logic::behaviors
