/* Copyright (C) 2020, Nikolai Wuttke. All rights reserved.
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

#include "enemy_rocket.hpp"

#include "engine/base_components.hpp"
#include "engine/movement.hpp"
#include "engine/physical_components.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/ientity_factory.hpp"
#include "game_logic/player.hpp"


namespace ec = rigel::engine::components;

namespace rigel::game_logic::behaviors
{

void EnemyRocket::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool isOnScreen,
  entityx::Entity entity)
{
  auto& position = *entity.component<ec::WorldPosition>();

  auto move = [&]() {
    if (mDirection.x != 0)
    {
      return engine::moveHorizontally(
        *d.mpCollisionChecker, entity, mDirection.x);
    }
    else
    {
      return engine::moveVertically(
        *d.mpCollisionChecker, entity, mDirection.y);
    }
  };

  auto explode = [&]() {
    spawnOneShotSprite(
      *d.mpEntityFactory, data::ActorID::Explosion_FX_1, position);
    entity.destroy();
  };


  ++mFramesElapsed;
  if (mFramesElapsed >= 4)
  {
    // Once at full speed (after 4 frames), the rocket moves twice per update,
    // and the first update is not checked for collision. This can cause the
    // rocket to move through walls under the right circumstances. Most likely
    // an oversight in the original game, but we replicate it here.
    position += mDirection;
  }

  const auto result = move();
  if (result != engine::MovementResult::Completed)
  {
    explode();
    return;
  }

  const auto bbox =
    engine::toWorldSpace(*entity.component<ec::BoundingBox>(), position);
  if (bbox.intersects(s.mpPlayer->worldSpaceHitBox()))
  {
    s.mpPlayer->takeDamage(1);
    explode();
  }
}

} // namespace rigel::game_logic::behaviors
