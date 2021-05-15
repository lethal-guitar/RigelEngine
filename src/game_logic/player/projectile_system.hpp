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

#pragma once

#include "base/spatial_types.hpp"
#include "base/warnings.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS


namespace rigel
{
struct IGameServiceProvider;
}
namespace rigel::data::map
{
class Map;
}
namespace rigel::engine
{
class CollisionChecker;
}
namespace rigel::game_logic
{
struct IEntityFactory;
}


namespace rigel::game_logic::player
{

class ProjectileSystem
{
public:
  ProjectileSystem(
    IEntityFactory* pEntityFactory,
    IGameServiceProvider* pServiceProvider,
    const engine::CollisionChecker* pCollisionChecker,
    const data::map::Map* pMap);

  void update(entityx::EntityManager& es);

private:
  void spawnWallImpactEffect(
    entityx::Entity entity,
    const base::Vector& position,
    const base::Point<float>& velocity,
    bool isRocket);
  bool isCollidingWithWorld(const base::Rect<int>& bbox);

  IEntityFactory* mpEntityFactory;
  IGameServiceProvider* mpServiceProvider;
  const engine::CollisionChecker* mpCollisionChecker;
  const data::map::Map* mpMap;
};

} // namespace rigel::game_logic::player
