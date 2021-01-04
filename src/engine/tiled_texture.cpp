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

#include "tiled_texture.hpp"

#include "data/game_traits.hpp"
#include "data/unit_conversions.hpp"

#include <cassert>


namespace rigel::engine {

using namespace data;
using namespace renderer;

TiledTexture::TiledTexture(OwningTexture&& tileSet, Renderer* pRenderer)
  : mTileSetTexture(std::move(tileSet))
  , mpRenderer(pRenderer)
{
}


void TiledTexture::renderTileStretched(
  const int index,
  const base::Rect<int>& destRect
) const {
  mpRenderer->drawTexture(
    mTileSetTexture.data(),
    renderer::toTexCoords(
      sourceRect(index, 1, 1),
      mTileSetTexture.width(),
      mTileSetTexture.height()),
    destRect);
}


void TiledTexture::renderTile(
  const int index,
  const int x,
  const int y
) const {
  renderTileGroup(index, x, y, 1, 1);
}


void TiledTexture::renderTileSlice(
  const int baseIndex,
  const base::Vector& tlPosition
) const {
  renderTileGroup(baseIndex, tlPosition.x, tlPosition.y, 1, 2);
}


void TiledTexture::renderTileQuad(
  const int baseIndex,
  const base::Vector& tlPosition
) const {
  renderTileGroup(baseIndex, tlPosition.x, tlPosition.y, 2, 2);
}


void TiledTexture::renderTileDoubleQuad(
  const int baseIndex,
  const base::Vector& tlPosition
) const {
  renderTileGroup(baseIndex, tlPosition.x, tlPosition.y, 4, 2);
}


int TiledTexture::tilesPerRow() const {
  return data::pixelsToTiles(mTileSetTexture.width());
}


void TiledTexture::renderTileGroup(
  const int index,
  const int posX,
  const int posY,
  const int tileSpanX,
  const int tileSpanY
) const {
  mTileSetTexture.render(
    mpRenderer,
    tileVectorToPixelVector({posX, posY}),
    sourceRect(index, tileSpanX, tileSpanY));
}


base::Rect<int> TiledTexture::sourceRect(
  const int index,
  const int tileSpanX,
  const int tileSpanY
) const {
  const base::Vector tileSetStartPosition{
    index % tilesPerRow(),
    index / tilesPerRow()};
  return {
    tileVectorToPixelVector(tileSetStartPosition),
    tileExtentsToPixelExtents({tileSpanX, tileSpanY})
  };
}

}
