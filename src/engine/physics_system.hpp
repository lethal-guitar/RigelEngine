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

#include <base/grid.hpp>
#include <base/spatial_types.hpp>
#include <data/tile_set.hpp>
#include <engine/timing.hpp>

#include <entityx/entityx.h>
#include <cstdint>
#include <tuple>
#include <vector>


namespace rigel { namespace data { namespace map { class Map; }}}


namespace rigel { namespace engine {

using BoundingBox = base::Rect<int>;


namespace components {

struct Physical {
  // Bounding box for world collision. Relative to entity's world position
  BoundingBox mCollisionRect;

  base::Point<float> mVelocity;
  bool mGravityAffected;
};

}


BoundingBox toWorldSpace(
  const BoundingBox& bbox, const base::Vector& entityPosition);


/** Implements game physics/world interaction
 *
 * Operates on all entities with Physical and WorldPosition components.
 * The Physical component's velocity is used to change the world position,
 * respecting world collision data. If gravityAffected is true, entities will
 * also fall down until they hit solid ground.
 */
class PhysicsSystem : public entityx::System<PhysicsSystem> {
public:
  explicit PhysicsSystem(const data::map::Map& map);

  void update(
    entityx::EntityManager& es,
    entityx::EventManager& events,
    entityx::TimeDelta dt) override;

private:
  const data::map::CollisionData& worldAt(int x, int y) const;

  base::Vector applyHorizontalMovement(
    const BoundingBox& bbox,
    const base::Vector& currentPosition,
    const std::int16_t movementX
  ) const;

  std::tuple<base::Vector, float> applyVerticalMovement(
    const BoundingBox& bbox,
    const base::Vector& currentPosition,
    float currentVelocity,
    const std::int16_t movementY
  ) const;

  float applyGravity(
    const BoundingBox& bbox,
    float currentVelocity);

private:
  base::Grid<data::map::CollisionData> mCollisionData;
  TimeStepper mTimeStepper;
};

}}
