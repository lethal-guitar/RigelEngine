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

#include "renderer.hpp"

#include "data/game_options.hpp"
#include "data/game_traits.hpp"
#include "loader/palette.hpp"
#include "sdl_utils/error.hpp"

RIGEL_DISABLE_WARNINGS
#include <glm/gtc/matrix_transform.hpp>
RIGEL_RESTORE_WARNINGS

#include <array>
#include <algorithm>
#include <iterator>


namespace rigel::renderer {

namespace {

const GLushort QUAD_INDICES[] = { 0, 1, 2, 2, 3, 1 };


constexpr auto WATER_MASK_WIDTH = 8;
constexpr auto WATER_MASK_HEIGHT = 8;
constexpr auto WATER_NUM_MASKS = 5;
constexpr auto WATER_MASK_INDEX_FILLED = 4;


#ifdef RIGEL_USE_GL_ES

const auto SHADER_PREAMBLE = R"shd(
#version 100

#define ATTRIBUTE attribute
#define OUT varying
#define IN varying
#define TEXTURE_LOOKUP texture2D
#define OUTPUT_COLOR gl_FragColor
#define OUTPUT_COLOR_DECLARATION
#define SET_POINT_SIZE(size) gl_PointSize = size;
#define HIGHP highp

precision mediump float;
)shd";

#else

// We generally want to stick to GLSL version 130 (from OpenGL 3.0) in order to
// maximize compatibility with older graphics cards. Unfortunately, Mac OS only
// supports GLSL 150 (from OpenGL 3.2), even when requesting a OpenGL 3.0
// context. Therefore, we use different GLSL versions depending on the
// platform.
#if defined(__APPLE__)
const auto SHADER_PREAMBLE = R"shd(
#version 150

#define ATTRIBUTE in
#define OUT out
#define IN in
#define TEXTURE_LOOKUP texture
#define OUTPUT_COLOR outputColor
#define OUTPUT_COLOR_DECLARATION out vec4 outputColor;
#define SET_POINT_SIZE
#define HIGHP
)shd";
#else
const auto SHADER_PREAMBLE = R"shd(
#version 130

#define ATTRIBUTE in
#define OUT out
#define IN in
#define TEXTURE_LOOKUP texture2D
#define OUTPUT_COLOR outputColor
#define OUTPUT_COLOR_DECLARATION out vec4 outputColor;
#define SET_POINT_SIZE
#define HIGHP
)shd";
#endif

#endif


const auto VERTEX_SOURCE = R"shd(
ATTRIBUTE HIGHP vec2 position;
ATTRIBUTE HIGHP vec2 texCoord;

OUT HIGHP vec2 texCoordFrag;

uniform mat4 transform;

void main() {
  gl_Position = transform * vec4(position, 0.0, 1.0);
  texCoordFrag = vec2(texCoord.x, 1.0 - texCoord.y);
}
)shd";


const auto FRAGMENT_SOURCE_SIMPLE = R"shd(
OUTPUT_COLOR_DECLARATION

IN HIGHP vec2 texCoordFrag;

uniform sampler2D textureData;

void main() {
  OUTPUT_COLOR = TEXTURE_LOOKUP(textureData, texCoordFrag);
}
)shd";


const auto FRAGMENT_SOURCE = R"shd(
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

const auto VERTEX_SOURCE_SOLID = R"shd(
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

const auto FRAGMENT_SOURCE_SOLID = R"shd(
OUTPUT_COLOR_DECLARATION

IN vec4 colorFrag;

void main() {
  OUTPUT_COLOR = colorFrag;
}
)shd";


const auto VERTEX_SOURCE_WATER_EFFECT = R"shd(
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

const auto FRAGMENT_SOURCE_WATER_EFFECT = R"shd(
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


void* toAttribOffset(std::uintptr_t offset) {
  return reinterpret_cast<void*>(offset);
}


glm::vec4 toGlColor(const base::Color& color) {
  return glm::vec4{color.r, color.g, color.b, color.a} / 255.0f;
}


void setScissorBox(
  const base::Rect<int>& clipRect,
  const base::Size<int>& frameBufferSize
) {
  const auto offsetAtBottom = frameBufferSize.height - clipRect.bottom();
  glScissor(
    clipRect.topLeft.x,
    offsetAtBottom - 1,
    clipRect.size.width,
    clipRect.size.height);
}


template <typename Iter>
void fillVertexData(
  float left,
  float right,
  float top,
  float bottom,
  Iter&& destIter,
  const std::size_t offset,
  const std::size_t stride
) {
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
  const std::size_t stride
) {
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
  const std::size_t stride
) {
  fillVertexData(
    coords.left,
    coords.right,
    coords.top,
    coords.bottom,
    std::forward<Iter>(destIter),
    offset,
    stride);
}


data::Image createWaterSurfaceAnimImage() {
  auto pixels = data::PixelBuffer{
    WATER_MASK_WIDTH * WATER_MASK_HEIGHT * WATER_NUM_MASKS,
    base::Color{255, 255, 255, 255}};

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

  auto applyPattern = [&pixels](
    const auto& pattern,
    const auto destOffset
  ) {
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
    static_cast<size_t>(WATER_MASK_WIDTH),
    static_cast<size_t>(WATER_MASK_HEIGHT * WATER_NUM_MASKS)};
}


data::Image createWaterEffectColorMapImage() {
  constexpr auto NUM_COLORS = int(loader::INGAME_PALETTE.size());
  constexpr auto NUM_ROWS = 2;

  auto pixels = data::PixelBuffer{};
  pixels.reserve(NUM_COLORS * NUM_ROWS);

  // 1st row: Original palette
  std::copy(
    begin(loader::INGAME_PALETTE), end(loader::INGAME_PALETTE),
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
  for (auto i = 0; i < NUM_COLORS; ++i) {
    const auto index = WATER_INDEX_START + i % NUM_WATER_INDICES;
    pixels.push_back(loader::INGAME_PALETTE[index]);
  }

  return data::Image{std::move(pixels), NUM_COLORS, NUM_ROWS};
}


auto getSize(SDL_Window* pWindow) {
  int windowWidth = 0;
  int windowHeight = 0;
  SDL_GetWindowSize(pWindow, &windowWidth, &windowHeight);
  return base::Size<int>{windowWidth, windowHeight};
}

}


Renderer::Renderer(SDL_Window* pWindow)
  : mpWindow(pWindow)
  , mTexturedQuadShader(
      SHADER_PREAMBLE,
      VERTEX_SOURCE,
      FRAGMENT_SOURCE,
      {"position", "texCoord"})
  , mSimpleTexturedQuadShader(
      SHADER_PREAMBLE,
      VERTEX_SOURCE,
      FRAGMENT_SOURCE_SIMPLE,
      {"position", "texCoord"})
  , mSolidColorShader(
      SHADER_PREAMBLE,
      VERTEX_SOURCE_SOLID,
      FRAGMENT_SOURCE_SOLID,
      {"position", "color"})
  , mWaterEffectShader(
      SHADER_PREAMBLE,
      VERTEX_SOURCE_WATER_EFFECT,
      FRAGMENT_SOURCE_WATER_EFFECT,
      {"position", "texCoordMask"})
  , mLastUsedShader(0)
  , mLastUsedTexture(0)
  , mRenderMode(RenderMode::SpriteBatch)
  , mCurrentFbo(0)
  , mWindowSize(getSize(pWindow))
  , mMaxWindowSize(
      [pWindow]() {
        SDL_DisplayMode displayMode;
        sdl_utils::check(SDL_GetDesktopDisplayMode(0, &displayMode));
        return base::Size<int>{displayMode.w, displayMode.h};
      }())
  , mCurrentFramebufferSize(mWindowSize)
  , mGlobalTranslation(0.0f, 0.0f)
  , mGlobalScale(1.0f, 1.0f)
{
  using namespace std;

  // General configuration
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  SDL_GL_SetSwapInterval(data::ENABLE_VSYNC_DEFAULT ? 1 : 0);

  // Setup a VBO for streaming data to the GPU, stays bound all the time
  glGenBuffers(1, &mStreamVbo);
  glBindBuffer(GL_ARRAY_BUFFER, mStreamVbo);
  glGenBuffers(1, &mStreamEbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mStreamEbo);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  // One-time setup for water effect shader
  useShaderIfChanged(mWaterEffectShader);
  mWaterEffectShader.setUniform("textureData", 0);
  mWaterEffectShader.setUniform("maskData", 1);
  mWaterEffectShader.setUniform("colorMapData", 2);

  mWaterSurfaceAnimTexture = createTexture(createWaterSurfaceAnimImage());
  mWaterEffectColorMapTexture = createTexture(
    createWaterEffectColorMapImage());

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, mWaterSurfaceAnimTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, mWaterEffectColorMapTexture);
  glActiveTexture(GL_TEXTURE0);

  // One-time setup for textured quad shader
  useShaderIfChanged(mTexturedQuadShader);
  mTexturedQuadShader.setUniform("textureData", 0);

  useShaderIfChanged(mSimpleTexturedQuadShader);
  mSimpleTexturedQuadShader.setUniform("textureData", 0);

  // Remaining setup
  onRenderTargetChanged();

  setColorModulation({255, 255, 255, 255});
}


Renderer::~Renderer() {
  glDeleteBuffers(1, &mStreamVbo);
  glDeleteTextures(1, &mWaterSurfaceAnimTexture);
  glDeleteTextures(1, &mWaterEffectColorMapTexture);
}


void Renderer::setOverlayColor(const base::Color& color) {
  if (color != mLastOverlayColor) {
    submitBatch();

    mLastOverlayColor = color;
    updateShaders();
  }
}


void Renderer::setColorModulation(const base::Color& colorModulation) {
  if (colorModulation != mLastColorModulation) {
    submitBatch();

    mLastColorModulation = colorModulation;
    updateShaders();
  }
}


void Renderer::drawTexture(
  const TextureId texture,
  const TexCoords& sourceRect,
  const base::Rect<int>& destRect,
  const bool repeat
) {
  setRenderModeIfChanged(RenderMode::SpriteBatch);

  if (texture != mLastUsedTexture) {
    submitBatch();

    glBindTexture(GL_TEXTURE_2D, texture);
    mLastUsedTexture = texture;
  }

  if (repeat != mTextureRepeatOn) {
    submitBatch();

    mTextureRepeatOn = repeat;
    updateShaders();
  }

  // x, y, tex_u, tex_v
  GLfloat vertices[4 * (2 + 2)];
  fillVertexPositions(destRect, std::begin(vertices), 0, 4);
  fillTexCoords(sourceRect, std::begin(vertices), 2, 4);

  batchQuadVertices(std::begin(vertices), std::end(vertices), 4u);
}


void Renderer::submitBatch() {
  if (mBatchData.empty()) {
    return;
  }

  auto submitBatchedQuads = [this]() {
    glBufferData(
      GL_ARRAY_BUFFER,
      sizeof(float) * mBatchData.size(),
      mBatchData.data(),
      GL_STREAM_DRAW);
    glBufferData(
      GL_ELEMENT_ARRAY_BUFFER,
      sizeof(GLushort) * mBatchIndices.size(),
      mBatchIndices.data(),
      GL_STREAM_DRAW);
    glDrawElements(
      GL_TRIANGLES,
      GLsizei(mBatchIndices.size()),
      GL_UNSIGNED_SHORT,
      nullptr);
  };

  switch (mRenderMode) {
    case RenderMode::SpriteBatch:
    case RenderMode::WaterEffect:
      submitBatchedQuads();
      break;

    case RenderMode::Points:
      glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(float) * mBatchData.size(),
        mBatchData.data(),
        GL_STREAM_DRAW);
      glDrawArrays(GL_POINTS, 0, GLsizei(mBatchData.size() / 6));
      break;

    case RenderMode::NonTexturedRender:
      // No batching yet for NonTexturedRender
      assert(false);
      break;
  }

  mBatchData.clear();
  mBatchIndices.clear();
}


void Renderer::drawFilledRectangle(
  const base::Rect<int>& rect,
  const base::Color& color
) {
  // Note: No batching for now
  setRenderModeIfChanged(RenderMode::NonTexturedRender);

  // x, y, r, g, b, a
  GLfloat vertices[4 * (2 + 4)];
  fillVertexPositions(rect, std::begin(vertices), 0, 6);

  const auto colorVec = toGlColor(color);
  for (auto vertex = 0; vertex < 4; ++vertex) {
    for (auto component = 0; component < 4; ++component) {
      vertices[2 + vertex*6 + component] = colorVec[component];
    }
  }

  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


void Renderer::drawRectangle(
  const base::Rect<int>& rect,
  const base::Color& color
) {
  // Note: No batching for now, drawRectangle is only used for debugging at
  // the moment
  setRenderModeIfChanged(RenderMode::NonTexturedRender);

  const auto left = float(rect.left());
  const auto right = float(rect.right());
  const auto top = float(rect.top());
  const auto bottom = float(rect.bottom());

  const auto colorVec = toGlColor(color);
  float vertices[] = {
    left, top, colorVec.r, colorVec.g, colorVec.b, colorVec.a,
    left, bottom, colorVec.r, colorVec.g, colorVec.b, colorVec.a,
    right, bottom, colorVec.r, colorVec.g, colorVec.b, colorVec.a,
    right, top, colorVec.r, colorVec.g, colorVec.b, colorVec.a,
    left, top, colorVec.r, colorVec.g, colorVec.b, colorVec.a
  };

  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
  glDrawArrays(GL_LINE_STRIP, 0, 5);
}


void Renderer::drawLine(
  const int x1,
  const int y1,
  const int x2,
  const int y2,
  const base::Color& color
) {
  // Note: No batching for now, drawLine is only used for debugging at the
  // moment
  setRenderModeIfChanged(RenderMode::NonTexturedRender);

  const auto colorVec = toGlColor(color);

  float vertices[] = {
    float(x1), float(y1), colorVec.r, colorVec.g, colorVec.b, colorVec.a,
    float(x2), float(y2), colorVec.r, colorVec.g, colorVec.b, colorVec.a
  };

  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
  glDrawArrays(GL_LINE_STRIP, 0, 2);
}


void Renderer::drawPoint(
  const base::Vector& position,
  const base::Color& color
) {
  setRenderModeIfChanged(RenderMode::Points);

  float vertices[] = {
    float(position.x),
    float(position.y),
    color.r / 255.0f,
    color.g / 255.0f,
    color.b / 255.0f,
    color.a / 255.0f
  };
  mBatchData.insert(
    std::end(mBatchData), std::begin(vertices), std::end(vertices));
}


void Renderer::drawWaterEffect(
  const base::Rect<int>& area,
  const TextureId texture,
  std::optional<int> surfaceAnimationStep
) {
  assert(
    !surfaceAnimationStep ||
    (*surfaceAnimationStep >= 0 && *surfaceAnimationStep < 4));

  using namespace std;

  const auto areaWidth = area.size.width;
  auto drawWater = [&, this](
    const base::Rect<int>& destRect,
    const int maskIndex
  ) {
    const auto maskTexStartY = maskIndex * WATER_MASK_HEIGHT;
    const auto animSourceRect = base::Rect<int>{
      {0, maskTexStartY},
      {areaWidth, WATER_MASK_HEIGHT}
    };

    // x, y, mask_u, mask_v
    GLfloat vertices[4 * (2 + 2)];
    fillVertexPositions(destRect, std::begin(vertices), 0, 4);
    fillTexCoords(
      toTexCoords(
        animSourceRect, WATER_MASK_WIDTH, WATER_MASK_HEIGHT * WATER_NUM_MASKS),
      std::begin(vertices),
      2,
      4);

    batchQuadVertices(std::begin(vertices), std::end(vertices), 4);
  };

  setRenderModeIfChanged(RenderMode::WaterEffect);

  if (mLastUsedTexture != texture) {
    submitBatch();
    glBindTexture(GL_TEXTURE_2D, texture);
    mLastUsedTexture = texture;
  }

  if (surfaceAnimationStep) {
    const auto waterSurfaceArea = base::Rect<int>{
      area.topLeft,
      {areaWidth, WATER_MASK_HEIGHT}
    };

    drawWater(waterSurfaceArea, *surfaceAnimationStep);

    auto remainingArea = area;
    remainingArea.topLeft.y += WATER_MASK_HEIGHT;
    remainingArea.size.height -= WATER_MASK_HEIGHT;

    drawWater(remainingArea, WATER_MASK_INDEX_FILLED);
  } else {
    drawWater(area, WATER_MASK_INDEX_FILLED);
  }
}


void Renderer::setGlobalTranslation(const base::Vector& translation) {
  const auto glTranslation = glm::vec2{translation.x, translation.y};
  if (glTranslation != mGlobalTranslation) {
    submitBatch();

    mGlobalTranslation = glTranslation;
    updateProjectionMatrix();
  }
}


base::Vector Renderer::globalTranslation() const {
  return base::Vector{
    static_cast<int>(mGlobalTranslation.x),
    static_cast<int>(mGlobalTranslation.y)};
}


void Renderer::setGlobalScale(const base::Point<float>& scale) {
  const auto glScale = glm::vec2{scale.x, scale.y};
  if (glScale != mGlobalScale) {
    submitBatch();

    mGlobalScale = glScale;
    updateProjectionMatrix();
  }
}


base::Point<float> Renderer::globalScale() const {
  return {mGlobalScale.x, mGlobalScale.y};
}


void Renderer::setClipRect(const std::optional<base::Rect<int>>& clipRect) {
  if (clipRect == mClipRect) {
    return;
  }

  submitBatch();

  mClipRect = clipRect;
  if (mClipRect) {
    glEnable(GL_SCISSOR_TEST);

    setScissorBox(*clipRect, mCurrentFramebufferSize);
  } else {
    glDisable(GL_SCISSOR_TEST);
  }
}


std::optional<base::Rect<int>> Renderer::clipRect() const {
  return mClipRect;
}


Renderer::RenderTarget Renderer::currentRenderTarget() const {
  return {mCurrentFramebufferSize, mCurrentFbo};
}


void Renderer::setRenderTarget(const RenderTarget& target) {
  if (target.mFbo == mCurrentFbo) {
    return;
  }

  submitBatch();

  if (!target.isDefault()) {
    mCurrentFramebufferSize = target.mSize;
    mCurrentFbo = target.mFbo;
  } else {
    mCurrentFramebufferSize.width = mWindowSize.width;
    mCurrentFramebufferSize.height = mWindowSize.height;
    mCurrentFbo = 0;
  }

  onRenderTargetChanged();
}


void Renderer::swapBuffers() {
  assert(mCurrentFbo == 0);

  submitBatch();
  SDL_GL_SwapWindow(mpWindow);

  const auto actualWindowSize = getSize(mpWindow);
  if (mWindowSize != actualWindowSize) {
    mWindowSize = actualWindowSize;
    mCurrentFramebufferSize.width = mWindowSize.width;
    mCurrentFramebufferSize.height = mWindowSize.height;
    onRenderTargetChanged();
  }
}


void Renderer::clear(const base::Color& clearColor) {
  const auto glColor = toGlColor(clearColor);
  glClearColor(glColor.r, glColor.g, glColor.b, glColor.a);
  glClear(GL_COLOR_BUFFER_BIT);
}


template <typename VertexIter>
void Renderer::batchQuadVertices(
  VertexIter&& dataBegin,
  VertexIter&& dataEnd,
  const std::size_t attributesPerVertex
) {
  using namespace std;

  const auto currentIndex = GLushort(mBatchData.size() / attributesPerVertex);

  GLushort indices[6];
  transform(
    begin(QUAD_INDICES),
    end(QUAD_INDICES),
    begin(indices),
    [&](const GLushort index) -> GLushort {
      return index + currentIndex;
    });

  // TODO: Limit maximum batch size
  mBatchData.insert(
    mBatchData.end(),
    forward<VertexIter>(dataBegin),
    forward<VertexIter>(dataEnd));
  mBatchIndices.insert(mBatchIndices.end(), begin(indices), end(indices));
}


void Renderer::setRenderModeIfChanged(const RenderMode mode) {
  if (mRenderMode != mode) {
    submitBatch();

    mRenderMode = mode;
    updateShaders();
  }
}


void Renderer::updateShaders() {
  switch (mRenderMode) {
    case RenderMode::SpriteBatch:
      if (
        mTextureRepeatOn ||
        mLastOverlayColor != base::Color{} ||
        mLastColorModulation != base::Color{255, 255, 255, 255}
      ) {
        useShaderIfChanged(mTexturedQuadShader);
        mTexturedQuadShader.setUniform("transform", mProjectionMatrix);
        mTexturedQuadShader.setUniform("enableRepeat", mTextureRepeatOn);
        mTexturedQuadShader.setUniform(
          "colorModulation", toGlColor(mLastColorModulation));
        mTexturedQuadShader.setUniform(
          "overlayColor", toGlColor(mLastOverlayColor));
      } else {
        useShaderIfChanged(mSimpleTexturedQuadShader);
        mSimpleTexturedQuadShader.setUniform("transform", mProjectionMatrix);
      }

      glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(float) * 4,
        toAttribOffset(0));
      glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(float) * 4,
        toAttribOffset(2 * sizeof(float)));
      break;

    case RenderMode::Points:
    case RenderMode::NonTexturedRender:
      useShaderIfChanged(mSolidColorShader);
      mSolidColorShader.setUniform("transform", mProjectionMatrix);
      glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(float) * 6,
        toAttribOffset(0));
      glVertexAttribPointer(
        1,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(float) * 6,
        toAttribOffset(2 * sizeof(float)));
      break;

    case RenderMode::WaterEffect:
      useShaderIfChanged(mWaterEffectShader);
      mWaterEffectShader.setUniform("transform", mProjectionMatrix);
      glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(float) * 4,
        toAttribOffset(0));
      glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(float) * 4,
        toAttribOffset(2 * sizeof(float)));
      break;
  }
}


Renderer::RenderTargetHandles Renderer::createRenderTargetTexture(
  const int width,
  const int height
) {
  const auto textureHandle =
    createGlTexture(GLsizei(width), GLsizei(height), nullptr);
  glBindTexture(GL_TEXTURE_2D, textureHandle);

  GLuint fboHandle;
  glGenFramebuffers(1, &fboHandle);
  glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);
  glFramebufferTexture2D(
    GL_FRAMEBUFFER,
    GL_COLOR_ATTACHMENT0,
    GL_TEXTURE_2D,
    textureHandle,
    0);

  glBindFramebuffer(GL_FRAMEBUFFER, mCurrentFbo);
  glBindTexture(GL_TEXTURE_2D, mLastUsedTexture);

  return {textureHandle, fboHandle};
}


auto Renderer::createTexture(const data::Image& image) -> TextureId {
  // OpenGL wants pixel data in bottom-up format, so transform it accordingly
  std::vector<std::uint8_t> pixelData;
  pixelData.resize(image.width() * image.height() * 4);
  for (std::size_t y = 0; y < image.height(); ++y) {
    const auto sourceRow = image.height() - (y + 1);
    const auto yOffsetSource = image.width() * sourceRow;
    const auto yOffset = y * image.width() * 4;

    for (std::size_t x = 0; x < image.width(); ++x) {
      const auto& pixel = image.pixelData()[x + yOffsetSource];
      pixelData[x*4 +     yOffset] = pixel.r;
      pixelData[x*4 + 1 + yOffset] = pixel.g;
      pixelData[x*4 + 2 + yOffset] = pixel.b;
      pixelData[x*4 + 3 + yOffset] = pixel.a;
    }
  }

  return createGlTexture(
    GLsizei(image.width()),
    GLsizei(image.height()),
    pixelData.data());
}


GLuint Renderer::createGlTexture(
  const GLsizei width,
  const GLsizei height,
  const GLvoid* const pData
) {
  GLuint handle = 0;
  glGenTextures(1, &handle);

  glBindTexture(GL_TEXTURE_2D, handle);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGBA,
    width,
    height,
    0,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    pData);
  glBindTexture(GL_TEXTURE_2D, mLastUsedTexture);

  return handle;
}



void Renderer::useShaderIfChanged(Shader& shader) {
  if (shader.handle() != mLastUsedShader) {
    shader.use();
    mLastUsedShader = shader.handle();
  }
}


void Renderer::onRenderTargetChanged() {
  glBindFramebuffer(GL_FRAMEBUFFER, mCurrentFbo);
  glViewport(0, 0, mCurrentFramebufferSize.width, mCurrentFramebufferSize.height);

  updateProjectionMatrix();

  if (mClipRect) {
    setScissorBox(*mClipRect, mCurrentFramebufferSize);
  }
}


void Renderer::updateProjectionMatrix() {
  const auto projection = glm::ortho(
    0.0f,
    float(mCurrentFramebufferSize.width),
    float(mCurrentFramebufferSize.height),
    0.0f);

  mProjectionMatrix = glm::scale(
    glm::translate(projection, glm::vec3(mGlobalTranslation, 0.0f)),
    glm::vec3(mGlobalScale, 1.0f));

  updateShaders();
}

}
