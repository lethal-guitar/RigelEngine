/* Copyright (C) 2021, Nikolai Wuttke. All rights reserved.
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
#include "engine/physical_components.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <optional>


namespace rigel::data::map
{
class Map;
}


namespace rigel::engine
{

class CollisionChecker;

struct PhysicsCollisionInfo
{
  bool mLeft;
  bool mRight;
  bool mTop;
  bool mBottom;
};

std::optional<PhysicsCollisionInfo> applyPhysics(
  const CollisionChecker& collisionChecker,
  const data::map::Map& map,
  entityx::Entity entity,
  components::MovingBody& body,
  components::WorldPosition& position,
  const components::BoundingBox& collisionRect);

float applyGravity(
  const CollisionChecker& collisionChecker,
  const components::BoundingBox& bbox,
  float currentVelocity);

} // namespace rigel::engine
