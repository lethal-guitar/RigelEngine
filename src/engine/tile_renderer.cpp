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

#include "tile_renderer.hpp"

#include "data/game_traits.hpp"
#include "data/unit_conversions.hpp"

#include <cassert>


namespace rigel { namespace engine {

using namespace data;

TileRenderer::TileRenderer(OwningTexture&& tileSet, Renderer* pRenderer)
  : mTileSetTexture(std::move(tileSet))
  , mpRenderer(pRenderer)
{
}


void TileRenderer::renderTile(
  const int index,
  const int x,
  const int y
) const {
  renderTileGroup(index, x, y, 1, 1);
}


void TileRenderer::renderTileSlice(
  const int baseIndex,
  const base::Vector& tlPosition
) const {
  renderTileGroup(baseIndex, tlPosition.x, tlPosition.y, 1, 2);
}


void TileRenderer::renderTileQuad(
  const int baseIndex,
  const base::Vector& tlPosition
) const {
  renderTileGroup(baseIndex, tlPosition.x, tlPosition.y, 2, 2);
}


void TileRenderer::renderTileDoubleQuad(
  const int baseIndex,
  const base::Vector& tlPosition
) const {
  renderTileGroup(baseIndex, tlPosition.x, tlPosition.y, 4, 2);
}


int TileRenderer::tilesPerRow() const {
  return data::pixelsToTiles(mTileSetTexture.width());
}


void TileRenderer::renderTileGroup(
  const int index,
  const int posX,
  const int posY,
  const int tileSpanX,
  const int tileSpanY
) const {
  const base::Vector tileSetStartPosition{
    index % tilesPerRow(),
    index / tilesPerRow()};

  mTileSetTexture.render(
    mpRenderer,
    tileVectorToPixelVector({posX, posY}),
    {
      tileVectorToPixelVector(tileSetStartPosition),
      tileExtentsToPixelExtents({tileSpanX, tileSpanY})
    });
}

}}
