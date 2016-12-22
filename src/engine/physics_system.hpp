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

#pragma once

#include "base/grid.hpp"
#include "base/spatial_types.hpp"
#include "base/warnings.hpp"
#include "data/tile_set.hpp"
#include "engine/base_components.hpp"
#include "engine/physical_components.hpp"
#include "engine/timing.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <cstdint>
#include <tuple>
#include <vector>


namespace rigel { namespace data { namespace map { class Map; }}}

namespace rigel { namespace engine {

/** Implements game physics/world interaction
 *
 * Operates on all entities with Physical, BoundingBox and WorldPosition
 * components. The Physical component's velocity is used to change the world
 * position, respecting world collision data. If gravityAffected is true,
 * entities will also fall down until they hit solid ground.
 *
 * Entities that collided with the world on the last update() will be tagged
 * with the CollidedWithWorld component.
 */
class PhysicsSystem : public entityx::System<PhysicsSystem> {
public:
  explicit PhysicsSystem(
    const data::map::Map* pMap,
    const data::map::TileAttributes* pTileAttributes);

  void update(
    entityx::EntityManager& es,
    entityx::EventManager& events,
    entityx::TimeDelta dt) override;

private:
  data::map::CollisionData worldAt(int x, int y) const;

  std::tuple<base::Vector, bool> applyHorizontalMovement(
    const components::BoundingBox& bbox,
    const base::Vector& currentPosition,
    std::int16_t movementX,
    bool allowStairStepping
  ) const;

  std::tuple<base::Vector, float, bool> applyVerticalMovement(
    const components::BoundingBox& bbox,
    const base::Vector& currentPosition,
    float currentVelocity,
    std::int16_t movementY,
    bool beginFallingOnHittingCeiling
  ) const;

  float applyGravity(
    const components::BoundingBox& bbox,
    float currentVelocity);

private:
  const data::map::Map* mpMap;
  const data::map::TileAttributes* mpTileAttributes;
  TimeStepper mTimeStepper;
};

}}
