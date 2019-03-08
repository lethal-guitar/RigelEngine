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

#include "base/spatial_types.hpp"
#include "base/warnings.hpp"
#include "engine/base_components.hpp"
#include "engine/physical_components.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <cstdint>
#include <tuple>


namespace rigel::data::map { class Map; }


namespace rigel { namespace engine {

class CollisionChecker;

/** Implements game physics/world interaction
 *
 * Operates on all entities with MovingBody, BoundingBox and WorldPosition
 * components. The MovingBody component's velocity is used to change the world
 * position, respecting world collision data. If gravityAffected is true,
 * entities will also fall down until they hit solid ground.
 *
 * Entities that collided with the world on the last update() will be tagged
 * with the CollidedWithWorld component.
 *
 * The collision detection is very simple and relies on knowing each entity's
 * previous position. Therefore, entities which are to collide against the
 * world mustn't be moved directly (i.e. by modifying their position), but via
 * setting a velocity and then letting the PhysicsSystem take care of doing the
 * movement. The system can't perform any corrections to entities which are
 * already positioned so that they collide with the world.
 *
 * For directly moving entities, the functions in movement.hpp should be used.
 */
class PhysicsSystem  : public entityx::Receiver<PhysicsSystem> {
public:
  PhysicsSystem(
    const engine::CollisionChecker* pCollisionChecker,
    const data::map::Map* pMap,
    entityx::EventManager* pEvents);

  /** Process currently existing entities
   *
   * Processes physics for all entities with the required components which
   * exist at the time of the call.
   */
  void update(entityx::EntityManager& es);

  /** Process currently existing entities
   *
   * Processes physics for all entities with the required components which
   * exist at the time of the call. Starts collecting entities
   * for the 2nd phase.
   */
  void updatePhase1(entityx::EntityManager& es);

  /** Process entities spawned after phase 1
   *
   * Processes physics for all entities that have been created or assigned
   * the right components after the call to updatePhase1().
   * Stops collecting.
   */
  void updatePhase2(entityx::EntityManager& es);

  void receive(
    const entityx::ComponentAddedEvent<components::MovingBody>& event);
  void receive(
    const entityx::ComponentRemovedEvent<components::MovingBody>& event);

private:
  void applyPhysics(
    entityx::Entity entity,
    components::MovingBody& body,
    components::WorldPosition& position,
    const components::BoundingBox& collisionRect);
  float applyGravity(
    const components::BoundingBox& bbox,
    float currentVelocity);

private:
  std::vector<entityx::Entity> mPhysicsObjectsForPhase2;
  const CollisionChecker* mpCollisionChecker;
  const data::map::Map* mpMap;
  entityx::EventManager* mpEvents;
  bool mShouldCollectForPhase2 = false;
};

}}
