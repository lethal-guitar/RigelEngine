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

#include "map_renderer.hpp"

#include "base/math_tools.hpp"
#include "data/game_traits.hpp"

#include <cfenv>
#include <iostream>


namespace rigel::engine {

using namespace std;
using namespace data;

using data::map::BackdropScrollMode;


namespace {

const auto ANIM_STATES = 4;
const auto FAST_ANIM_FRAME_DELAY = 1;
const auto SLOW_ANIM_FRAME_DELAY = 2;
const auto PARALLAX_FACTOR = 4;


base::Vector wrapBackgroundOffset(base::Vector offset) {
  return {
    offset.x % GameTraits::viewPortWidthPx,
    offset.y % GameTraits::viewPortHeightPx
  };
}

}

MapRenderer::MapRenderer(
  renderer::Renderer* pRenderer,
  const data::map::Map* pMap,
  MapRenderData&& renderData
)
  : mpRenderer(pRenderer)
  , mpMap(pMap)
  , mTileSetTexture(
      renderer::OwningTexture(pRenderer, renderData.mTileSetImage),
      pRenderer)
  , mBackdropTexture(mpRenderer, renderData.mBackdropImage)
  , mScrollMode(renderData.mBackdropScrollMode)
{
  if (renderData.mSecondaryBackdropImage) {
    mAlternativeBackdropTexture = renderer::OwningTexture(
      mpRenderer, *renderData.mSecondaryBackdropImage);
  }
}


void MapRenderer::switchBackdrops() {
  std::swap(mBackdropTexture, mAlternativeBackdropTexture);
}


void MapRenderer::renderBackground(const base::Vector& cameraPosition) {
  renderMapTiles(cameraPosition, DrawMode::Background);
}


void MapRenderer::renderForeground(const base::Vector& cameraPosition) {
  renderMapTiles(cameraPosition, DrawMode::Foreground);
}


void MapRenderer::renderBackdrop(const base::Vector& cameraPosition) {
  base::Vector offset;

  const auto parallaxBoth = mScrollMode == BackdropScrollMode::ParallaxBoth;
  const auto parallaxHorizontal =
    mScrollMode == BackdropScrollMode::ParallaxHorizontal || parallaxBoth;

  const auto autoScrollX = mScrollMode == BackdropScrollMode::AutoHorizontal;
  const auto autoScrollY = mScrollMode == BackdropScrollMode::AutoVertical;

  if (parallaxHorizontal || parallaxBoth) {
    offset = wrapBackgroundOffset({
      parallaxHorizontal ? cameraPosition.x*PARALLAX_FACTOR : 0,
      parallaxBoth ? cameraPosition.y*PARALLAX_FACTOR : 0
    });
  } else if (autoScrollX || autoScrollY) {
    // TODO Currently this only works right when running at 60 FPS.
    // It should be time-based instead, but it's trickier to get it smooth
    // then.
    const double speedFactor = autoScrollX ? 2.0 : 1.0;
    std::fesetround(FE_TONEAREST);
    const auto offsetPixels = base::round(mElapsedFrames60Fps / speedFactor);
    ++mElapsedFrames60Fps;

    if (autoScrollX) {
      offset.x = offsetPixels % GameTraits::viewPortWidthPx;
    } else {
      const auto baseOffset = offsetPixels % GameTraits::viewPortHeightPx;
      offset.y = GameTraits::viewPortHeightPx - baseOffset;
    }
  }

  const auto offsetForDrawing = offset * -1;
  const auto offsetForRepeating = base::Vector{
    GameTraits::viewPortWidthPx - offset.x,
    GameTraits::viewPortHeightPx - offset.y};

  // TODO: This can be simplified by using texture wrap mode (GL_REPEAT)
  mBackdropTexture.render(mpRenderer, offsetForDrawing);
  if (!autoScrollY) {
    mBackdropTexture.render(
      mpRenderer,
      base::Vector{offsetForRepeating.x, offsetForDrawing.y});
  }

  if (parallaxBoth || autoScrollY) {
    mBackdropTexture.render(
      mpRenderer,
      base::Vector{offsetForDrawing.x, offsetForRepeating.y});
  }

  if (parallaxBoth) {
    mBackdropTexture.render(mpRenderer, offsetForRepeating);
  }
}


void MapRenderer::renderMapTiles(
  const base::Vector& cameraPosition,
  const DrawMode drawMode
) {
  for (int layer=0; layer<2; ++layer) {
    for (int y=0; y<GameTraits::mapViewPortHeightTiles; ++y) {
      for (int x=0; x<GameTraits::mapViewPortWidthTiles; ++x) {
        const auto col = x + cameraPosition.x;
        const auto row = y + cameraPosition.y;
        if (col >= mpMap->width() || row >= mpMap->height()) {
          continue;
        }

        const auto tileIndex = mpMap->tileAt(layer, col, row);
        const auto isForeground =
          mpMap->attributeDict().attributes(tileIndex).isForeGround();
        const auto shouldRenderForeground = drawMode == DrawMode::Foreground;

        if (isForeground != shouldRenderForeground) {
          continue;
        }

        renderTile(tileIndex, x, y);
      }
    }
  }
}


void MapRenderer::updateAnimatedMapTiles() {
  ++mElapsedFrames;
}


void MapRenderer::renderSingleTile(
  const data::map::TileIndex index,
  const base::Vector& position,
  const base::Vector& cameraPosition
) {
  const auto screenPosition = position - cameraPosition;
  renderTile(index, screenPosition.x, screenPosition.y);
}


void MapRenderer::renderTile(
  const data::map::TileIndex tileIndex,
  const int x,
  const int y
) {
  // Tile index 0 is used to represent a transparent tile, i.e. the backdrop
  // should be visible. Therefore, don't draw if the index is 0.
  if (tileIndex != 0) {
    const auto tileIndexToDraw = animatedTileIndex(tileIndex);
    mTileSetTexture.renderTile(tileIndexToDraw, x, y);
  }
}


map::TileIndex MapRenderer::animatedTileIndex(
  const map::TileIndex tileIndex
) const {
  if (mpMap->attributeDict().attributes(tileIndex).isAnimated()) {
    const auto fastAnimOffset =
      (mElapsedFrames / FAST_ANIM_FRAME_DELAY) % ANIM_STATES;
    const auto slowAnimOffset =
      (mElapsedFrames / SLOW_ANIM_FRAME_DELAY) % ANIM_STATES;

    const auto isFastAnim =
      mpMap->attributeDict().attributes(tileIndex).isFastAnimation();
    return tileIndex + (isFastAnim ? fastAnimOffset : slowAnimOffset);
  } else {
    return tileIndex;
  }
}

}
