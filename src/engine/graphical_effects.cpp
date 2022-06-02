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

#include "graphical_effects.hpp"

#include "data/game_traits.hpp"
#include "renderer/renderer.hpp"

#include <algorithm>
#include <array>
#include <cassert>


namespace rigel::engine
{

namespace
{

constexpr int nextPowerOf2(int number)
{
  // This is a slow and naive implementation, but that's fine.
  // We only use this at compile time and only on small inputs.
  // Readability is more important here than best performance.
  auto result = 1;
  while (result < number)
  {
    result *= 2;
  }
  return result;
}


constexpr auto WATER_MASK_WIDTH = 8;
constexpr auto WATER_MASK_HEIGHT = 8;
constexpr auto WATER_NUM_MASKS = 5;
constexpr auto WATER_ANIM_TEX_WIDTH = WATER_MASK_WIDTH;
constexpr auto WATER_ANIM_TEX_HEIGHT =
  nextPowerOf2(WATER_MASK_HEIGHT * WATER_NUM_MASKS);
constexpr auto WATER_MASK_INDEX_FILLED = 4;


const char* VERTEX_SOURCE_WATER_EFFECT = R"shd(
ATTRIBUTE vec2 position;
ATTRIBUTE vec2 texCoord;

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
  texCoordMaskFrag = vec2(texCoord.x, 1.0 - texCoord.y);

  gl_Position = transformedPos;
}
)shd";


const char* FRAGMENT_SOURCE_WATER_EFFECT = R"shd(
DEFAULT_PRECISION_DECLARATION
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


constexpr auto WATER_EFFECT_TEXTURE_UNIT_NAMES =
  std::array{"textureData", "maskData", "colorMapData"};

const renderer::ShaderSpec WATER_EFFECT_SHADER{
  renderer::VertexLayout::PositionAndTexCoords,
  WATER_EFFECT_TEXTURE_UNIT_NAMES,
  VERTEX_SOURCE_WATER_EFFECT,
  FRAGMENT_SOURCE_WATER_EFFECT};


data::Image createWaterSurfaceAnimImage()
{
  auto pixels = data::PixelBuffer{
    WATER_ANIM_TEX_WIDTH * WATER_ANIM_TEX_HEIGHT,
    base::Color{255, 255, 255, 255}};

  // clang-format off
  const std::array<int, 16> patternCalmSurface{
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1
  };

  const std::array<int, 16> patternWaveRight{
    0, 0, 0, 0, 0, 1, 1, 0,
    1, 0, 0, 1, 1, 1, 1, 1
  };

  const std::array<int, 16> patternWaveLeft{
    0, 1, 1, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 0, 0, 1
  };
  // clang-format on

  auto applyPattern = [&pixels](const auto& pattern, const auto destOffset) {
    std::transform(
      std::begin(pattern),
      std::end(pattern),
      std::begin(pixels) + destOffset,
      [](const int patternValue) {
        const auto value = static_cast<uint8_t>(255 * patternValue);
        return base::Color{value, value, value, value};
      });
  };

  const auto pixelsPerAnimStep = WATER_MASK_WIDTH * WATER_MASK_HEIGHT;

  applyPattern(patternCalmSurface, 0);
  applyPattern(patternWaveRight, pixelsPerAnimStep);
  applyPattern(patternCalmSurface, pixelsPerAnimStep * 2);
  applyPattern(patternWaveLeft, pixelsPerAnimStep * 3);

  return data::Image{
    move(pixels),
    static_cast<size_t>(WATER_ANIM_TEX_WIDTH),
    static_cast<size_t>(WATER_ANIM_TEX_HEIGHT)};
}


data::Image createWaterEffectColorMapImage()
{
  constexpr auto NUM_COLORS = int(data::GameTraits::INGAME_PALETTE.size());
  constexpr auto NUM_ROWS = 2;

  auto pixels = data::PixelBuffer{};
  pixels.reserve(NUM_COLORS * NUM_ROWS);

  // 1st row: Original palette
  std::copy(
    begin(data::GameTraits::INGAME_PALETTE),
    end(data::GameTraits::INGAME_PALETTE),
    std::back_inserter(pixels));

  // 2nd row: Corresponding "under water" colors
  // For the water effect, every palette color is remapped to one
  // of the colors at indices 8 to 11. These colors are different
  // shades of blue and a dark green, which leads to the watery look.
  // The remapping is done by manipulating color indices like this:
  //   water_index = index % 4 + 8
  //
  // In order to create a lookup table for remapping, we therefore
  // need to repeat the colors found at indices 8 to 11 four times,
  // giving us a palette of only "under water" colors.
  constexpr auto WATER_INDEX_START = 8;
  constexpr auto NUM_WATER_INDICES = 4;
  for (auto i = 0; i < NUM_COLORS; ++i)
  {
    const auto index = WATER_INDEX_START + i % NUM_WATER_INDICES;
    pixels.push_back(data::GameTraits::INGAME_PALETTE[index]);
  }

  return data::Image{
    std::move(pixels),
    static_cast<size_t>(NUM_COLORS),
    static_cast<size_t>(NUM_ROWS)};
}

} // namespace


WaterEffectRenderer::WaterEffectRenderer(renderer::Renderer* pRenderer)
  : mpRenderer(pRenderer)
  , mShader(WATER_EFFECT_SHADER)
  , mBatch(&mShader)
  , mWaterSurfaceAnimTexture(pRenderer, createWaterSurfaceAnimImage())
  , mWaterEffectColorMapTexture(pRenderer, createWaterEffectColorMapImage())
{
  pRenderer->setNativeRepeatEnabled(mWaterSurfaceAnimTexture.data(), true);
}


void WaterEffectRenderer::draw(
  const renderer::RenderTargetTexture& backgroundBuffer,
  const base::ArrayView<WaterEffectArea> areas,
  const int surfaceAnimationStep)
{
  assert(surfaceAnimationStep >= 0 && surfaceAnimationStep < 4);

  mBatch.reset();
  mBatch.preAllocateSpace(areas.size());

  auto addArea = [&](
                   const base::Rect<int>& destRect,
                   const int maskIndex,
                   const int areaWidth) {
    const auto maskTexStartY = maskIndex * WATER_MASK_HEIGHT;
    const auto animSourceRect =
      base::Rect<int>{{0, maskTexStartY}, {areaWidth, WATER_MASK_HEIGHT}};

    mBatch.addQuad(
      renderer::toTexCoords(
        animSourceRect, WATER_ANIM_TEX_WIDTH, WATER_ANIM_TEX_HEIGHT),
      destRect);
  };

  for (const auto& areaSpec : areas)
  {
    const auto& area = areaSpec.mArea;

    const auto areaWidth = area.size.width;
    if (areaSpec.mIsAnimated)
    {
      const auto waterSurfaceArea =
        base::Rect<int>{area.topLeft, {areaWidth, WATER_MASK_HEIGHT}};

      addArea(waterSurfaceArea, surfaceAnimationStep, areaWidth);

      auto remainingArea = area;
      remainingArea.topLeft.y += WATER_MASK_HEIGHT;
      remainingArea.size.height -= WATER_MASK_HEIGHT;

      addArea(remainingArea, WATER_MASK_INDEX_FILLED, areaWidth);
    }
    else
    {
      addArea(area, WATER_MASK_INDEX_FILLED, areaWidth);
    }
  }

  mBatch.addTexture(backgroundBuffer.data());
  mBatch.addTexture(mWaterSurfaceAnimTexture.data());
  mBatch.addTexture(mWaterEffectColorMapTexture.data());

  mpRenderer->drawCustomQuadBatch(mBatch.data());
}

} // namespace rigel::engine
