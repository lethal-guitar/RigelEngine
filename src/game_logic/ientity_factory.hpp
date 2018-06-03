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

#pragma once

#include "base/warnings.hpp"
#include "engine/base_components.hpp"
#include "data/map.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

namespace rigel { namespace game_logic {

enum class ProjectileType {
  PlayerRegularShot,
  PlayerLaserShot,
  PlayerRocketShot,
  PlayerFlameShot,
  EnemyLaserShot,
  EnemyRocket
};


enum class ProjectileDirection {
  Left,
  Right,
  Up,
  Down
};


struct IEntityFactory {
  virtual ~IEntityFactory() = default;

  virtual entityx::Entity createEntitiesForLevel(
    const data::map::ActorDescriptionList& actors) = 0;

  /** Create a sprite entity using the given actor ID. If assignBoundingBox is
   * true, the dimensions of the sprite's first frame are used to assign a
   * bounding box.
   */
  virtual entityx::Entity createSprite(
    data::ActorID actorID,
    bool assignBoundingBox = false) = 0;

  virtual entityx::Entity createSprite(
    data::ActorID actorID,
    const base::Vector& position,
    bool assignBoundingBox = false) = 0;

  virtual entityx::Entity createProjectile(
    ProjectileType type,
    const engine::components::WorldPosition& pos,
    ProjectileDirection direction) = 0;

  virtual entityx::Entity createActor(
    data::ActorID actorID,
    const base::Vector& position) = 0;
};

}}
