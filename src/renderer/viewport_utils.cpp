/* Copyright (C) 2022, Nikolai Wuttke. All rights reserved.
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

#include "renderer/viewport_utils.hpp"


namespace rigel::renderer
{

namespace
{

auto asVec(const base::Size<int>& size)
{
  return base::Vec2{size.width, size.height};
}


auto asSize(const base::Vec2& vec)
{
  return base::Size{vec.x, vec.y};
}

} // namespace


base::Vec2 scaleVec(const base::Vec2& vec, const base::Vec2f& scale)
{
  return base::Vec2{base::round(vec.x * scale.x), base::round(vec.y * scale.y)};
}


base::Extents scaleSize(const base::Extents& size, const base::Vec2f& scale)
{
  return asSize(scaleVec(asVec(size), scale));
}

} // namespace rigel::renderer
