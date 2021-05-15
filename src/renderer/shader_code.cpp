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


namespace rigel::renderer
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
  vec2 texCoords = texCoordFrag;
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


const char* VERTEX_SOURCE_WATER_EFFECT = R"shd(
ATTRIBUTE vec2 position;
ATTRIBUTE vec2 texCoordMask;

OUT vec2 texCoordFrag;
OUT vec2 texCoordMaskFrag;

uniform mat4 transform;

void main() {
  SET_POINT_SIZE(1.0);
  vec4 transformedPos = transform * vec4(position, 0.0, 1.0);

  // Applying the transform gives us a position in normalized device
  // coordinates (from -1.0 to 1.0). For sampling the render target texture,
  // we need texture coordinates in the range 0.0 to 1.0, however.
  // Therefore, we transform the position from normalized device coordinates
  // into the 0.0 to 1.0 range by adding 1 and dividing by 2.
  //
  // We assume that the texture is as large as the screen, therefore sampling
  // with the resulting tex coords should be equivalent to reading the pixel
  // located at 'position'.
  texCoordFrag = (transformedPos.xy + vec2(1.0, 1.0)) / 2.0;
  texCoordMaskFrag = vec2(texCoordMask.x, 1.0 - texCoordMask.y);

  gl_Position = transformedPos;
}
)shd";

const char* FRAGMENT_SOURCE_WATER_EFFECT = R"shd(
OUTPUT_COLOR_DECLARATION

IN vec2 texCoordFrag;
IN vec2 texCoordMaskFrag;

uniform sampler2D textureData;
uniform sampler2D maskData;
uniform sampler2D colorMapData;


vec3 paletteColor(int index) {
  // 1st row of the color map contains the original palette. Because the
  // texture is stored up-side down, y-coordinate 0.5 actually corresponds to
  // the upper row of pixels.
  return TEXTURE_LOOKUP(colorMapData, vec2(float(index) / 16.0, 0.5)).rgb;
}


vec3 remappedColor(int index) {
  // 2nd row contains the remapped "water" palette
  return TEXTURE_LOOKUP(colorMapData, vec2(float(index) / 16.0, 0.0)).rgb;
}


vec4 applyWaterEffect(vec4 color) {
  // The original game runs in a palette-based video mode, where the frame
  // buffer stores indices into a palette of 16 colors instead of directly
  // storing color values. The water effect is implemented as a modification
  // of these index values in the frame buffer.
  // To replicate it, we first have to transform our RGBA color values into
  // indices, by searching the palette for a matching color. With the index,
  // we then look up the corresponding "under water" color.
  // It would also be possible to perform the index manipulation here in the
  // shader and then do another palette lookup to get the result. But due to
  // precision problems on the Raspberry Pi which would cause visual glitches
  // with that approach, we do it via lookup table instead.
  int index = 0;
  for (int i = 0; i < 16; ++i) {
    if (color.rgb == paletteColor(i)) {
      index = i;
    }
  }

  return vec4(remappedColor(index), color.a);
}

void main() {
  vec4 color = TEXTURE_LOOKUP(textureData, texCoordFrag);
  vec4 mask = TEXTURE_LOOKUP(maskData, texCoordMaskFrag);
  float maskValue = mask.r;
  OUTPUT_COLOR = mix(color, applyWaterEffect(color), maskValue);
}
)shd";

} // namespace rigel::renderer
