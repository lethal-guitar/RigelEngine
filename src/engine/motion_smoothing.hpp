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

#include "base/math_tools.hpp"
#include "base/spatial_types.hpp"


namespace rigel::engine
{

inline base::Point<float> lerp(
  const base::Point<float>& a,
  const base::Point<float>& b,
  const float factor)
{
  return {base::lerp(a.x, b.x, factor), base::lerp(a.y, b.y, factor)};
}


inline base::Point<float>
  lerp(const base::Vector& a, const base::Vector& b, const float factor)
{
  return lerp(base::cast<float>(a), base::cast<float>(b), factor);
}


template <typename T>
base::Vector lerpRounded(
  const base::Point<T>& a,
  const base::Point<T>& b,
  const float factor)
{
  const auto lerped = lerp(base::cast<float>(a), base::cast<float>(b), factor);

  return {base::round(lerped.x), base::round(lerped.y)};
}

} // namespace rigel::engine
