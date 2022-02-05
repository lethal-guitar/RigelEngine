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

#include "base/array_view.hpp"
#include "base/spatial_types.hpp"

#include <cstdint>


namespace rigel::renderer
{

class Shader;


// This is the minimum number supported by GL ES 2.0/WebGL.
constexpr auto MAX_MULTI_TEXTURES = 8;


using TextureId = std::uint32_t;

/** Texture coordinates for Renderer::drawTexture()
 *
 * Values should be in range [0.0, 1.0] - unless texture repeat is
 * enabled. Use the toTexCoords() helper function to create these from
 * a source rectangle.
 */
struct TexCoords
{
  float left;
  float top;
  float right;
  float bottom;
};


/** Convert a source rect to texture coordinates
 *
 * Renderer::drawTexture() expects normalized texture coordinates,
 * but most of the time, it's easier to work with image-specific
 * coordinates, like e.g. "from 8,8 to 32,64". This helper function
 * converts from the latter to the former.
 */
inline TexCoords toTexCoords(
  const base::Rect<int>& sourceRect,
  const int texWidth,
  const int texHeight)
{
  const auto left = sourceRect.topLeft.x / float(texWidth);
  const auto top = sourceRect.topLeft.y / float(texHeight);
  const auto width = sourceRect.size.width / float(texWidth);
  const auto height = sourceRect.size.height / float(texHeight);
  const auto right = left + width;
  const auto bottom = top + height;

  return {left, top, right, bottom};
}


// 4 * (x, y, u, v)
using QuadVertices = std::array<float, 4 * (2 + 2)>;


struct CustomQuadBatchData
{
  base::ArrayView<TextureId> mTextures;
  base::ArrayView<float> mVertexBuffer;
  const Shader* mpShader;
};

} // namespace rigel::renderer
