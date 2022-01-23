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

#include "shader_code.hpp"

#include <array>


namespace rigel::renderer
{

namespace
{

const char* VERTEX_SOURCE = R"shd(
ATTRIBUTE HIGHP vec2 position;
ATTRIBUTE HIGHP vec2 texCoord;

OUT HIGHP vec2 texCoordFrag;

uniform mat4 transform;

void main() {
  gl_Position = transform * vec4(position, 0.0, 1.0);
  texCoordFrag = vec2(texCoord.x, 1.0 - texCoord.y);
}
)shd";


const char* FRAGMENT_SOURCE_SIMPLE = R"shd(
OUTPUT_COLOR_DECLARATION

IN HIGHP vec2 texCoordFrag;

uniform sampler2D textureData;

void main() {
  OUTPUT_COLOR = TEXTURE_LOOKUP(textureData, texCoordFrag);
}
)shd";


const char* FRAGMENT_SOURCE = R"shd(
OUTPUT_COLOR_DECLARATION

IN HIGHP vec2 texCoordFrag;

uniform sampler2D textureData;
uniform vec4 overlayColor;

uniform vec4 colorModulation;
uniform bool enableRepeat;

void main() {
  HIGHP vec2 texCoords = texCoordFrag;
  if (enableRepeat) {
    texCoords.x = fract(texCoords.x);
    texCoords.y = fract(texCoords.y);
  }

  vec4 baseColor = TEXTURE_LOOKUP(textureData, texCoords);
  vec4 modulated = baseColor * colorModulation;
  float targetAlpha = modulated.a;

  OUTPUT_COLOR =
    vec4(mix(modulated.rgb, overlayColor.rgb, overlayColor.a), targetAlpha);
}
)shd";

const char* VERTEX_SOURCE_SOLID = R"shd(
ATTRIBUTE vec2 position;
ATTRIBUTE vec4 color;

OUT vec4 colorFrag;

uniform mat4 transform;

void main() {
  SET_POINT_SIZE(1.0);
  gl_Position = transform * vec4(position, 0.0, 1.0);
  colorFrag = color;
}
)shd";

const char* FRAGMENT_SOURCE_SOLID = R"shd(
OUTPUT_COLOR_DECLARATION

IN vec4 colorFrag;

void main() {
  OUTPUT_COLOR = colorFrag;
}
)shd";


constexpr auto TEXTURED_QUAD_TEXTURE_UNIT_NAMES = std::array{"textureData"};

} // namespace


const ShaderSpec TEXTURED_QUAD_SHADER{
  VertexLayout::PositionAndTexCoords,
  TEXTURED_QUAD_TEXTURE_UNIT_NAMES,
  VERTEX_SOURCE,
  FRAGMENT_SOURCE};


const ShaderSpec SIMPLE_TEXTURED_QUAD_SHADER{
  VertexLayout::PositionAndTexCoords,
  TEXTURED_QUAD_TEXTURE_UNIT_NAMES,
  VERTEX_SOURCE,
  FRAGMENT_SOURCE_SIMPLE};


const ShaderSpec SOLID_COLOR_SHADER{
  VertexLayout::PositionAndColor,
  {},
  VERTEX_SOURCE_SOLID,
  FRAGMENT_SOURCE_SOLID};

} // namespace rigel::renderer
