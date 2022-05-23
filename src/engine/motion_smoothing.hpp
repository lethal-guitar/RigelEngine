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

#include "base/math_utils.hpp"
#include "base/spatial_types.hpp"
#include "base/warnings.hpp"
#include "data/unit_conversions.hpp"
#include "engine/base_components.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS


namespace rigel::engine
{

inline void discardInterpolation(entityx::Entity entity)
{
  if (entity.has_component<components::InterpolateMotion>())
  {
    entity.component<components::InterpolateMotion>()->mPreviousPosition =
      *entity.component<components::WorldPosition>();
  }
}


inline void enableInterpolation(entityx::Entity entity)
{
  const auto& currentPosition = *entity.component<components::WorldPosition>();
  entity.assign<components::InterpolateMotion>(currentPosition);
}


inline base::Vec2f
  lerp(const base::Vec2f& a, const base::Vec2f& b, const float factor)
{
  return {base::lerp(a.x, b.x, factor), base::lerp(a.y, b.y, factor)};
}


inline base::Vec2f
  lerp(const base::Vec2& a, const base::Vec2& b, const float factor)
{
  return lerp(base::cast<float>(a), base::cast<float>(b), factor);
}


template <typename T>
base::Vec2 lerpRounded(
  const base::Vec2T<T>& a,
  const base::Vec2T<T>& b,
  const float factor)
{
  const auto lerped = lerp(base::cast<float>(a), base::cast<float>(b), factor);

  return {base::round(lerped.x), base::round(lerped.y)};
}


inline base::Vec2 interpolatedPixelPosition(
  const base::Vec2& a,
  const base::Vec2& b,
  const float interpolationFactor)
{
  return lerpRounded(
    data::tilesToPixels(a), data::tilesToPixels(b), interpolationFactor);
}


inline base::Vec2 interpolatedPixelPosition(
  const entityx::Entity entity,
  const float interpolationFactor)
{
  const auto currentPosition =
    *entity.component<const components::WorldPosition>();

  if (entity.has_component<components::InterpolateMotion>())
  {
    const auto& previousPosition =
      entity.component<const components::InterpolateMotion>()
        ->mPreviousPosition;
    return interpolatedPixelPosition(
      previousPosition, currentPosition, interpolationFactor);
  }

  return data::tilesToPixels(currentPosition);
}

} // namespace rigel::engine
