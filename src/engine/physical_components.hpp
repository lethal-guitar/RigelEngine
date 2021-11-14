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
#include "base/warnings.hpp"
#include "engine/base_components.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/Entity.h>
RIGEL_RESTORE_WARNINGS


namespace rigel::engine
{


namespace components
{

namespace parameter_aliases
{

using Velocity = base::Vec2T<float>;
using GravityAffected = bool;
using IgnoreCollisions = bool;
using ResetAfterSequence = bool;
using EnableX = bool;

} // namespace parameter_aliases


struct MovingBody
{
  MovingBody(
    const base::Vec2T<float> velocity,
    const bool gravityAffected,
    const bool ignoreCollisions = false)
    : mVelocity(velocity)
    , mGravityAffected(gravityAffected)
    , mIgnoreCollisions(ignoreCollisions)
  {
  }

  base::Vec2T<float> mVelocity;
  bool mGravityAffected;

  /** When set, the body will move through walls, but collision events will
   * still be emitted.
   */
  bool mIgnoreCollisions;
  bool mIsActive = true;
};


/** Marker component which is added to all entities that had a collision with
 * the level geometry on the last physics update.
 */
struct CollidedWithWorld
{
};


/** Marks an entity to participate in world collision
 *
 * Other MovingBody entities will collide against the bounding box of any
 * SolidBody entity as if it were part of the world.
 * */
struct SolidBody
{
};


struct MovementSequence
{
  using VelocityList = base::ArrayView<base::Vec2T<float>>;

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

} // namespace components


namespace events
{

struct CollidedWithWorld
{
  entityx::Entity mEntity;
  bool mCollidedLeft;
  bool mCollidedRight;
  bool mCollidedTop;
  bool mCollidedBottom;
};

} // namespace events


components::BoundingBox toWorldSpace(
  const components::BoundingBox& bbox,
  const base::Vector& entityPosition);

} // namespace rigel::engine
