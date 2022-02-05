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


namespace rigel::renderer
{

inline QuadVertices createTexturedQuadVertices(
  const TexCoords& sourceRect,
  const base::Rect<int>& destRect)
{
  glm::vec2 posOffset(float(destRect.topLeft.x), float(destRect.topLeft.y));
  glm::vec2 posScale(float(destRect.size.width), float(destRect.size.height));

  const auto left = posOffset.x;
  const auto right = posScale.x + posOffset.x;
  const auto top = posOffset.y;
  const auto bottom = posScale.y + posOffset.y;

  // clang-format off
  return QuadVertices{{
    left,  bottom, sourceRect.left,  sourceRect.bottom,
    left,  top,    sourceRect.left,  sourceRect.top,
    right, bottom, sourceRect.right, sourceRect.bottom,
    right, top,    sourceRect.right, sourceRect.top
  }};
  // clang-format on
}

} // namespace rigel::renderer
