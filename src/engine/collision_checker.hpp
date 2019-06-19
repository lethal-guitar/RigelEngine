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
#include "engine/physical_components.hpp"

#include <vector>

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS


namespace rigel::engine {

class CollisionChecker : public entityx::Receiver<CollisionChecker> {
public:
  CollisionChecker(
    const data::map::Map* pMap,
    entityx::EntityManager& entities,
    entityx::EventManager& eventManager);

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

  void receive(
    const entityx::ComponentAddedEvent<components::SolidBody>& event);
  void receive(
    const entityx::ComponentRemovedEvent<components::SolidBody>& event);

private:
  bool testHorizontalSpan(
    const engine::components::BoundingBox& bbox,
    int y,
    data::map::SolidEdge edge) const;
  bool testVerticalSpan(
    const engine::components::BoundingBox& bbox,
    int x,
    data::map::SolidEdge edge) const;
  bool testSolidBodyCollision(
    const engine::components::BoundingBox& bbox) const;

  std::vector<entityx::Entity> mSolidBodies;
  const data::map::Map* mpMap;
};

}
