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

#include "base/warnings.hpp"
#include "data/map.hpp"
#include "engine/base_components.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS


namespace rigel { namespace engine {

class CollisionChecker {
public:
  CollisionChecker(const data::map::Map* pMap);

  /** Walk entity by the given amount if possible.
   *
   * The entity must have a WorldPosition and a BoundingBox.
   * walk() will try to add the given amount (can be positive or negative)
   * to the entity's position, and return true if it suceeded, false otherwise.
   * For the move to suceed, the new position must still be on solid ground
   * (i.e. no walking off the edge of a platform) and there must be no
   * collisions with the world.
   */
  bool walkEntity(entityx::Entity entity, int amount) const;

  /** As above, but for walking on the ceiling */
  bool walkEntityOnCeiling(entityx::Entity entity, int amount) const;

  bool isOnSolidGround(
    const engine::components::WorldPosition& position,
    const engine::components::BoundingBox& bbox) const;

  bool isTouchingCeiling(
    const engine::components::WorldPosition& position,
    const engine::components::BoundingBox& bbox) const;

  bool isTouchingLeftWall(
    const engine::components::WorldPosition& position,
    const engine::components::BoundingBox& bbox) const;

  bool isTouchingRightWall(
    const engine::components::WorldPosition& position,
    const engine::components::BoundingBox& bbox) const;

  bool isOnSolidGround(const engine::components::BoundingBox& bbox) const;
  bool isTouchingCeiling(const engine::components::BoundingBox& bbox) const;
  bool isTouchingLeftWall(const engine::components::BoundingBox& bbox) const;
  bool isTouchingRightWall(const engine::components::BoundingBox& bbox) const;

private:
  bool testHorizontalSpan(
    const engine::components::BoundingBox& bbox,
    int y,
    data::map::SolidEdge edge) const;
  bool testVerticalSpan(
    const engine::components::BoundingBox& bbox,
    int x,
    data::map::SolidEdge edge) const;

  const data::map::Map* mpMap;
};

}}
