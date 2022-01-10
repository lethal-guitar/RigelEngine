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

#include "upscaling_utils.hpp"

#include "base/math_utils.hpp"
#include "data/game_options.hpp"
#include "data/game_traits.hpp"
#include "renderer/renderer.hpp"

#include <algorithm>


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


base::Size<float>
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

} // namespace


ViewPortInfo determineViewPort(const Renderer* pRenderer)
{
  const auto windowWidth = float(pRenderer->windowSize().width);
  const auto windowHeight = float(pRenderer->windowSize().height);

  const auto [usableWidth, usableHeight] =
    determineUsableSize(windowWidth, windowHeight);

  const auto widthScale = usableWidth / data::GameTraits::viewPortWidthPx;
  const auto heightScale = usableHeight / data::GameTraits::viewPortHeightPx;
  const auto offsetX = (windowWidth - usableWidth) / 2.0f;
  const auto offsetY = (windowHeight - usableHeight) / 2.0f;

  return {
    base::Vec2{int(offsetX), int(offsetY)},
    base::Size<int>{int(usableWidth), int(usableHeight)},
    base::Vec2f{widthScale, heightScale}};
}


bool canUseWidescreenMode(const Renderer* pRenderer)
{
  const auto windowWidth = float(pRenderer->windowSize().width);
  const auto windowHeight = float(pRenderer->windowSize().height);
  return windowWidth / windowHeight > data::GameTraits::aspectRatio;
}


WidescreenViewPortInfo determineWidescreenViewPort(const Renderer* pRenderer)
{
  const auto info = determineViewPort(pRenderer);

  const auto windowWidth = pRenderer->windowSize().width;
  const auto tileWidthScaled = data::GameTraits::tileSize * info.mScale.x;
  const auto maxTilesOnScreen = int(windowWidth / tileWidthScaled);

  const auto widthInPixels =
    std::min(base::round(maxTilesOnScreen * tileWidthScaled), windowWidth);
  const auto paddingPixels = pRenderer->windowSize().width - widthInPixels;

  return {maxTilesOnScreen, widthInPixels, paddingPixels / 2};
}


base::Vec2 scaleVec(const base::Vec2& vec, const base::Vec2f& scale)
{
  return base::Vec2{base::round(vec.x * scale.x), base::round(vec.y * scale.y)};
}


base::Extents scaleSize(const base::Extents& size, const base::Vec2f& scale)
{
  return asSize(scaleVec(asVec(size), scale));
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
    const auto width =
      options.mWidescreenModeOn && canUseWidescreenMode(pRenderer)
      ? determineWidescreenViewPort(pRenderer).mWidthPx
      : data::GameTraits::viewPortWidthPx;
    return RenderTargetTexture{
      pRenderer, width, data::GameTraits::viewPortHeightPx};
  }
}

} // namespace rigel::renderer
