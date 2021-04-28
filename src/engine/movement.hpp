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

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

namespace rigel::data::map
{
class Map;
}


namespace rigel::engine
{

class CollisionChecker;


/** Walk entity by the given amount if possible.
 *
 * The entity must have a WorldPosition and a BoundingBox.
 * walk() will try to change the entity's position based on the orientation
 * given, and return true if it succeeded, false otherwise.
 * For the move to succeed, the new position must still be on solid ground
 * (i.e. no walking off the edge of a platform) and there must be no
 * collisions with the world.
 */
bool walk(
  const CollisionChecker& collisionChecker,
  entityx::Entity entity,
  components::Orientation orientation);

/** As above, but for walking on the ceiling */
bool walkOnCeiling(
  const CollisionChecker& collisionChecker,
  entityx::Entity entity,
  components::Orientation orientation);


enum class MovementResult
{
  Completed,
  MovedPartially,
  Failed
};


MovementResult moveHorizontally(
  const CollisionChecker& collisionChecker,
  entityx::Entity entity,
  int amount);

MovementResult moveVertically(
  const CollisionChecker& collisionChecker,
  entityx::Entity entity,
  int amount);

MovementResult moveHorizontallyWithStairStepping(
  const CollisionChecker& collisionChecker,
  entityx::Entity entity,
  int amount);


void applyConveyorBeltMotion(
  const CollisionChecker& collisionChecker,
  const data::map::Map& map,
  entityx::Entity entity);

} // namespace rigel::engine
