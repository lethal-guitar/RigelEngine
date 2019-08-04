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


namespace rigel::renderer {

WidescreenViewPortInfo determineWidescreenViewPort(const Renderer* pRenderer) {
  // TODO: Eliminate duplication with setupSimpleUpscaling () in game_main.cpp
  const auto [windowWidthInt, windowHeightInt] = pRenderer->windowSize();
  const auto windowWidth = float(windowWidthInt);
  const auto windowHeight = float(windowHeightInt);

  const auto usableWidth = windowWidth > windowHeight
    ? data::GameTraits::aspectRatio * windowHeight
    : windowWidth;

  const auto widthScale = usableWidth / data::GameTraits::viewPortWidthPx;

  const auto tileWidthScaled =
    base::round(data::GameTraits::tileSize * widthScale);
  const auto maxTilesOnScreen =
    pRenderer->windowSize().width / tileWidthScaled;

  const auto widthInPixels =
    static_cast<int>(maxTilesOnScreen * data::GameTraits::tileSize * widthScale);
  const auto paddingPixels =
    pRenderer->windowSize().width - widthInPixels;

  return {
    maxTilesOnScreen,
    widthInPixels,
    paddingPixels / 2
  };
}

}
