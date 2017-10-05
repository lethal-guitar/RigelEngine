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

#include "base/array_view.hpp"
#include "base/spatial_types.hpp"
#include "engine/base_components.hpp"


namespace rigel { namespace engine {


namespace components {

struct MovingBody {
  MovingBody(
    const base::Point<float> velocity,
    const bool gravityAffected,
    const bool isPlayer = false,
    const bool ignoreCollisions = false
  )
    : mVelocity(velocity)
    , mGravityAffected(gravityAffected)
    , mIsPlayer(isPlayer)
    , mIgnoreCollisions(ignoreCollisions)
  {
  }

  base::Point<float> mVelocity;
  bool mGravityAffected;
  bool mIsPlayer;

  /** When set, the body will move through walls, but collision events will
   * still be emitted.
   */
  bool mIgnoreCollisions;
};


/** Marker component which is added to all entities that had a collision with
 * the level geometry on the last physics update.
 */
struct CollidedWithWorld {};


/** Marks an entity to participate in world collision
 *
 * Other MovingBody entities will collide against the bounding box of any
 * SolidBody entity as if it were part of the world.
 * */
struct SolidBody {};


namespace parameter_aliases {

using ResetAfterSequence = bool;
using EnableX = bool;

}


struct MovementSequence {
  using VelocityList = base::ArrayView<base::Point<float>>;

  explicit MovementSequence(
    const VelocityList& velocities,
    const bool resetVelocityAfterSequence = false,
    const bool enableX = true)
    : mVelocites(velocities)
    , mResetVelocityAfterSequence(resetVelocityAfterSequence)
    , mEnableX(enableX)
  {
  }

  VelocityList mVelocites;
  decltype(mVelocites)::size_type mCurrentStep = 0;
  bool mResetVelocityAfterSequence = false;
  bool mEnableX = true;
};

}


components::BoundingBox toWorldSpace(
  const components::BoundingBox& bbox, const base::Vector& entityPosition);

}}
