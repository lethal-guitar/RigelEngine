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

#include "base/math_tools.hpp"
#include "data/game_traits.hpp"
#include "renderer/renderer.hpp"

#include <algorithm>


namespace rigel::renderer {

ViewPortInfo determineViewPort(const Renderer* pRenderer) {
  const auto windowWidth = float(pRenderer->windowSize().width);
  const auto windowHeight = float(pRenderer->windowSize().height);

  const auto usableWidth = windowWidth > windowHeight
    ? data::GameTraits::aspectRatio * windowHeight
    : windowWidth;
  const auto usableHeight = windowHeight >= windowWidth
    ? 1.0f / data::GameTraits::aspectRatio * windowWidth
    : windowHeight;

  const auto widthScale = usableWidth / data::GameTraits::viewPortWidthPx;
  const auto heightScale = usableHeight / data::GameTraits::viewPortHeightPx;
  const auto offsetX = (windowWidth - usableWidth) / 2.0f;
  const auto offsetY = (windowHeight - usableHeight) / 2.0f;

  return {
    base::Vector{int(offsetX), int(offsetY)},
    base::Size<int>{int(usableWidth), int(usableHeight)},
    base::Point<float>{widthScale, heightScale}
  };
}


WidescreenViewPortInfo determineWidescreenViewPort(const Renderer* pRenderer) {
  // TODO: Eliminate duplication with setupSimpleUpscaling () in game_main.cpp
  const auto [windowWidthInt, windowHeightInt] = pRenderer->windowSize();
  const auto windowWidth = float(windowWidthInt);
  const auto windowHeight = float(windowHeightInt);

  const auto usableWidth = windowWidth > windowHeight
    ? data::GameTraits::aspectRatio * windowHeight
    : windowWidth;

  const auto widthScale = usableWidth / data::GameTraits::viewPortWidthPx;

  const auto tileWidthScaled = data::GameTraits::tileSize * widthScale;
  const auto maxTilesOnScreen =
    base::round(pRenderer->windowSize().width / tileWidthScaled);

  const auto widthInPixels =
    std::min(base::round(maxTilesOnScreen * tileWidthScaled), windowWidthInt);
  const auto paddingPixels =
    pRenderer->windowSize().width - widthInPixels;

  return {
    maxTilesOnScreen,
    widthInPixels,
    paddingPixels / 2
  };
}

}
