/* Copyright (C) 2019, Nikolai Wuttke. All rights reserved.
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

#include "upscaling.hpp"

#include "base/math_utils.hpp"
#include "data/game_traits.hpp"
#include "renderer/renderer.hpp"
#include "renderer/shader_code.hpp"
#include "renderer/viewport_utils.hpp"

#include <algorithm>


namespace rigel::renderer
{

namespace
{

constexpr auto PIXEL_PERFECT_SCALE_X = 5;
constexpr auto PIXEL_PERFECT_SCALE_Y = 6;

// This shader implements a single-pass sharp bilinear filter. The end
// result is the same as if we would first render to an intermediate buffer
// with pixel-perfect scaling, and then sample that bilinearly. But we
// don't actually use an additional render pass, instead we modify the
// texture coordinates to achieve the same result as if we would be
// sampling from a pre-scaled intermediate buffer.
//
// This is based on
// https://github.com/rsn8887/Sharp-Bilinear-Shaders/blob/58ef94a8f96548d660ab46ee76dd6322ac1a7adb/Copy_To_RetroPie/shaders/sharp-bilinear-simple.glsl
const char* FRAGMENT_SOURCE = R"shd(
DEFAULT_PRECISION_DECLARATION
OUTPUT_COLOR_DECLARATION

IN HIGHP vec2 texCoordFrag;

uniform sampler2D textureData;
uniform vec2 textureSize;
uniform vec2 preScaleFactor;

void main() {
  HIGHP vec2 pxCoords = texCoordFrag * textureSize;

  vec2 regionRange = 0.5 - 0.5 / preScaleFactor;
  vec2 alpha = fract(pxCoords) - 0.5;
  vec2 adjustedAlpha = (alpha - clamp(alpha, -regionRange, regionRange));
  vec2 offset = adjustedAlpha * preScaleFactor + 0.5;
  HIGHP vec2 adjustedPxCoords = floor(pxCoords) + offset;

  HIGHP vec2 adjustedTexCoords = adjustedPxCoords / textureSize;

  OUTPUT_COLOR = TEXTURE_LOOKUP(textureData, adjustedTexCoords);
}
)shd";

constexpr auto TEXTURE_UNIT_NAMES = std::array{"textureData"};

const ShaderSpec SHARP_BILINEAR_SHADER{
  VertexLayout::PositionAndTexCoords,
  TEXTURE_UNIT_NAMES,
  STANDARD_VERTEX_SOURCE,
  FRAGMENT_SOURCE};


base::SizeF
  determineUsableSize(const float windowWidth, const float windowHeight)
{
  auto quantize = [](const float value) {
    return float(int(value) - int(value) % 8);
  };

  const auto actualAspectRatioIsWiderThanTarget =
    windowWidth / windowHeight > data::GameTraits::aspectRatio;
  if (actualAspectRatioIsWiderThanTarget)
  {
    const auto evenHeight = quantize(windowHeight);
    return {data::GameTraits::aspectRatio * evenHeight, evenHeight};
  }
  else
  {
    return {
      quantize(windowWidth),
      quantize(1.0f / data::GameTraits::aspectRatio * windowWidth)};
  }
}


void setupRenderingViewport(
  renderer::Renderer* pRenderer,
  const bool perElementUpscaling)
{
  if (perElementUpscaling)
  {
    const auto [offset, size, scale] = renderer::determineViewport(pRenderer);
    pRenderer->setGlobalScale(scale);
    pRenderer->setGlobalTranslation(offset);
    pRenderer->setClipRect(base::Rect<int>{offset, size});
  }
  else
  {
    pRenderer->setClipRect(base::Rect<int>{{}, data::GameTraits::viewportSize});
  }
}

} // namespace


ViewportInfo determineViewport(const Renderer* pRenderer)
{
  const auto windowWidth = float(pRenderer->windowSize().width);
  const auto windowHeight = float(pRenderer->windowSize().height);

  const auto [usableWidth, usableHeight] =
    determineUsableSize(windowWidth, windowHeight);

  const auto widthScale = usableWidth / data::GameTraits::viewportWidthPx;
  const auto heightScale = usableHeight / data::GameTraits::viewportHeightPx;
  const auto offsetX = (windowWidth - usableWidth) / 2.0f;
  const auto offsetY = (windowHeight - usableHeight) / 2.0f;

  return {
    base::Vec2{int(offsetX), int(offsetY)},
    base::SizeT<int>{int(usableWidth), int(usableHeight)},
    base::Vec2f{widthScale, heightScale}};
}


bool canUseWidescreenMode(const Renderer* pRenderer)
{
  const auto windowWidth = float(pRenderer->windowSize().width);
  const auto windowHeight = float(pRenderer->windowSize().height);
  return windowWidth / windowHeight > data::GameTraits::aspectRatio;
}


bool canUsePixelPerfectScaling(
  const Renderer* pRenderer,
  const data::GameOptions& options)
{
  if (!options.mAspectRatioCorrectionEnabled)
  {
    return true;
  }

  const auto pixelPerfectBufferWidth =
    determineLowResBufferWidth(pRenderer, options.widescreenModeActive());
  return pRenderer->windowSize().width >=
    pixelPerfectBufferWidth * PIXEL_PERFECT_SCALE_X &&
    pRenderer->windowSize().height >=
    data::GameTraits::viewportHeightPx * PIXEL_PERFECT_SCALE_Y;
}


WidescreenViewportInfo determineWidescreenViewport(const Renderer* pRenderer)
{
  const auto info = determineViewport(pRenderer);

  const auto windowWidth = pRenderer->windowSize().width;
  const auto tileWidthScaled = data::GameTraits::tileSize * info.mScale.x;
  const auto maxTilesOnScreen = int(windowWidth / tileWidthScaled);

  const auto widthInPixels =
    std::min(base::round(maxTilesOnScreen * tileWidthScaled), windowWidth);
  const auto paddingPixels = pRenderer->windowSize().width - widthInPixels;

  return {maxTilesOnScreen, widthInPixels, paddingPixels / 2};
}


int determineLowResBufferWidth(
  const Renderer* pRenderer,
  const bool widescreenModeWanted)
{
  if (widescreenModeWanted && canUseWidescreenMode(pRenderer))
  {
    const auto scale = determineViewport(pRenderer).mScale.x;
    const auto fullWidth = determineWidescreenViewport(pRenderer).mWidthPx;
    return base::round(fullWidth / scale);
  }

  return data::GameTraits::viewportWidthPx;
}


RenderTargetTexture createFullscreenRenderTarget(
  Renderer* pRenderer,
  const data::GameOptions& options)
{
  if (options.mPerElementUpscalingEnabled)
  {
    return RenderTargetTexture{
      pRenderer, pRenderer->windowSize().width, pRenderer->windowSize().height};
  }
  else
  {
    return RenderTargetTexture{
      pRenderer,
      determineLowResBufferWidth(pRenderer, options.widescreenModeActive()),
      data::GameTraits::viewportHeightPx};
  }
}


base::Vec2 offsetTo4by3WithinWidescreen(
  Renderer* pRenderer,
  const data::GameOptions& options)
{
  const auto viewportInfo = determineViewport(pRenderer);
  if (options.mPerElementUpscalingEnabled)
  {
    return viewportInfo.mOffset;
  }

  return scaleVec(
    viewportInfo.mOffset,
    {1.0f / viewportInfo.mScale.x, 1.0f / viewportInfo.mScale.y});
}


UpscalingBuffer::UpscalingBuffer(
  Renderer* pRenderer,
  const data::GameOptions& options)
  : mRenderTarget(renderer::createFullscreenRenderTarget(pRenderer, options))
  , mSharpBilinearShader(SHARP_BILINEAR_SHADER)
  , mpRenderer(pRenderer)
  , mFilter(options.mUpscalingFilter)
{
}

[[nodiscard]] base::ScopeGuard
  UpscalingBuffer::bindAndClear(const bool perElementUpscaling)
{
  auto saved = mRenderTarget.bind();
  mpRenderer->clear();

  setupRenderingViewport(mpRenderer, perElementUpscaling);
  return saved;
}


void UpscalingBuffer::clear()
{
  const auto saved = mRenderTarget.bindAndReset();
  mpRenderer->clear();
}


void UpscalingBuffer::present(
  const bool isWidescreenFrame,
  const bool perElementUpscaling)
{
  using UF = data::UpscalingFilter;

  // We use OpenGL's blending here instead of the renderer's color modulation,
  // because we don't need to implement the modulation feature in our custom
  // sharp bilinear shader if we do it that way.
  glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
  glBlendColor(0.0f, 0.0f, 0.0f, std::clamp(mAlphaMod / 255.0f, 0.0f, 1.0f));
  auto blendFuncGuard =
    base::defer([]() { glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); });

  if (perElementUpscaling)
  {
    mpRenderer->clear();
    mRenderTarget.render(0, 0);
    mpRenderer->submitBatch();
    return;
  }

  const auto windowWidth = float(mpRenderer->windowSize().width);
  const auto windowHeight = float(mpRenderer->windowSize().height);

  auto setUpViewport = [&](
                         const int textureWidth,
                         const int textureHeight,
                         const base::Vec2f& scale) {
    const auto usableWidth = textureWidth * scale.x;
    const auto usableHeight = textureHeight * scale.y;
    const auto offsetX = (windowWidth - usableWidth) / 2.0f;
    const auto offsetY = (windowHeight - usableHeight) / 2.0f;

    mpRenderer->setGlobalTranslation({int(offsetX), int(offsetY)});
    mpRenderer->setGlobalScale(scale);
  };


  mpRenderer->clear();

  auto saved = renderer::saveState(mpRenderer);

  if (mFilter == UF::PixelPerfect)
  {
    const auto usedWidth = isWidescreenFrame
      ? mRenderTarget.width()
      : data::GameTraits::viewportWidthPx;

    const auto maxIntegerScaleFactor = float(std::min(
      mpRenderer->windowSize().width / usedWidth,
      mpRenderer->windowSize().height / mRenderTarget.height()));
    const auto scale = mAspectRatioCorrection
      ? base::Vec2f{PIXEL_PERFECT_SCALE_X, PIXEL_PERFECT_SCALE_Y}
      : base::Vec2f{maxIntegerScaleFactor, maxIntegerScaleFactor};
    setUpViewport(usedWidth, mRenderTarget.height(), scale);
    mRenderTarget.render(0, 0);
  }
  else
  {
    const auto info = renderer::determineViewport(mpRenderer);
    mpRenderer->setGlobalScale(info.mScale);

    if (isWidescreenFrame)
    {
      const auto offset =
        renderer::determineWidescreenViewport(mpRenderer).mLeftPaddingPx;
      mpRenderer->setGlobalTranslation({offset, 0});
    }
    else
    {
      mpRenderer->setGlobalTranslation(info.mOffset);
    }

    if (mFilter == UF::SharpBilinear)
    {
      drawWithCustomShader(
        mpRenderer, mRenderTarget, {0, 0}, mSharpBilinearShader);
    }
    else
    {
      mRenderTarget.render(0, 0);
    }
  }

  mpRenderer->submitBatch();
}


void UpscalingBuffer::setAlphaMod(const std::uint8_t alphaMod)
{
  mAlphaMod = alphaMod;
}


void UpscalingBuffer::updateConfiguration(const data::GameOptions& options)
{
  mAspectRatioCorrection = options.mAspectRatioCorrectionEnabled;

  mRenderTarget = createFullscreenRenderTarget(mpRenderer, options);

  if (options.mPerElementUpscalingEnabled)
  {
    mFilter = data::UpscalingFilter::None;
  }
  else
  {
    const auto pixelPerfectScalingWanted =
      options.mUpscalingFilter == data::UpscalingFilter::PixelPerfect;
    const auto pixelPerfectScalingPossible =
      canUsePixelPerfectScaling(mpRenderer, options);

    // Fall back to sharp bilinear if pixel perfect scaling isn't possible
    if (pixelPerfectScalingWanted)
    {
      mFilter = pixelPerfectScalingPossible
        ? data::UpscalingFilter::PixelPerfect
        : data::UpscalingFilter::SharpBilinear;
    }
    else
    {
      mFilter = options.mUpscalingFilter;
    }
  }

  mpRenderer->setFilteringEnabled(
    mRenderTarget.data(),
    mFilter == data::UpscalingFilter::Bilinear ||
      mFilter == data::UpscalingFilter::SharpBilinear);

  if (mFilter == data::UpscalingFilter::SharpBilinear)
  {
    const auto textureSize =
      glm::vec2{mRenderTarget.width(), mRenderTarget.height()};
    const auto preScaleFactor =
      glm::vec2{PIXEL_PERFECT_SCALE_X, PIXEL_PERFECT_SCALE_Y};

    const auto guard = useTemporarily(mSharpBilinearShader);
    mSharpBilinearShader.setUniform("textureSize", textureSize);
    mSharpBilinearShader.setUniform("preScaleFactor", preScaleFactor);
  }
}

} // namespace rigel::renderer
