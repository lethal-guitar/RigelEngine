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

#pragma once

#include "base/warnings.hpp"
#include "renderer/renderer_support.hpp"

RIGEL_DISABLE_WARNINGS
#include <glm/vec2.hpp>
RIGEL_RESTORE_WARNINGS

#include <iterator>


namespace rigel::renderer
{

template <typename Iter>
void fillVertexData(
  float left,
  float right,
  float top,
  float bottom,
  Iter&& destIter,
  const std::size_t offset,
  const std::size_t stride)
{
  using namespace std;
  advance(destIter, offset);

  const auto innerStride = stride - 2;

  *destIter++ = left;
  *destIter++ = bottom;
  advance(destIter, innerStride);

  *destIter++ = left;
  *destIter++ = top;
  advance(destIter, innerStride);

  *destIter++ = right;
  *destIter++ = bottom;
  advance(destIter, innerStride);

  *destIter++ = right;
  *destIter++ = top;
  advance(destIter, innerStride);
}


template <typename Iter>
void fillVertexPositions(
  const base::Rect<int>& rect,
  Iter&& destIter,
  const std::size_t offset,
  const std::size_t stride)
{
  glm::vec2 posOffset(float(rect.topLeft.x), float(rect.topLeft.y));
  glm::vec2 posScale(float(rect.size.width), float(rect.size.height));

  const auto left = posOffset.x;
  const auto right = posScale.x + posOffset.x;
  const auto top = posOffset.y;
  const auto bottom = posScale.y + posOffset.y;

  fillVertexData(
    left, right, top, bottom, std::forward<Iter>(destIter), offset, stride);
}


template <typename Iter>
void fillTexCoords(
  const TexCoords& coords,
  Iter&& destIter,
  const std::size_t offset,
  const std::size_t stride)
{
  fillVertexData(
    coords.left,
    coords.right,
    coords.top,
    coords.bottom,
    std::forward<Iter>(destIter),
    offset,
    stride);
}

} // namespace rigel::renderer
