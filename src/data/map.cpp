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

#include "map.hpp"

#include "data/game_traits.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <utility>


namespace rigel::data::map {

using namespace std;


Map::Map(
  const int widthInTiles,
  const int heightInTiles,
  TileAttributeDict attributes
)
  : mLayers({
      TileArray(widthInTiles*heightInTiles, 0),
      TileArray(widthInTiles*heightInTiles, 0)})
  , mWidthInTiles(static_cast<size_t>(widthInTiles))
  , mHeightInTiles(static_cast<size_t>(heightInTiles))
  , mAttributes(std::move(attributes))
{
  assert(widthInTiles >= 0);
  assert(heightInTiles >= 0);
}


map::TileIndex Map::tileAt(
  const int layer,
  const int x,
  const int y
) const {
  return tileRefAt(layer, x, y);
}


void Map::setTileAt(
  const int layer,
  const int x,
  const int y,
  TileIndex index
) {
  if (index >= GameTraits::CZone::numTilesTotal) {
    throw invalid_argument("Tile index too large for tile set");
  }
  tileRefAt(layer, x, y) = index;
}


void Map::clearSection(
  const int x,
  const int y,
  const int width,
  const int height
) {
  for (auto row = y; row < y+height; ++row) {
    for (auto col = x; col < x + width; ++col) {
      setTileAt(0, col, row, 0);
      setTileAt(1, col, row, 0);
    }
  }
}


const TileAttributeDict& Map::attributeDict() const {
  return mAttributes;
}


TileAttributes Map::attributes(const int x, const int y) const {
  if (
    static_cast<std::size_t>(x) >= mWidthInTiles ||
    static_cast<std::size_t>(y) >= mHeightInTiles
  ) {
    // Outside of the map doesn't have any attributes set
    return TileAttributes{};
  }

  if (tileAt(0, x, y) != 0 && tileAt(1, x, y) != 0) {
    // "Composite" tiles (content on both layers) are ignored for attribute
    // checking
    return TileAttributes{};
  }

  if (tileAt(1, x, y) != 0) {
    return TileAttributes{mAttributes.attributes(tileAt(1, x, y))};
  }

  return TileAttributes{mAttributes.attributes(tileAt(0, x, y))};
}


CollisionData Map::collisionData(const int x, const int y) const {
  if (static_cast<std::size_t>(x) >= mWidthInTiles) {
    // Left/right edge of the map are always solid
    return CollisionData::fullySolid();
  }

  if (static_cast<std::size_t>(y) >= mHeightInTiles) {
    // Bottom/top edge of the map are never solid
    return CollisionData{};
  }

  if (tileAt(0, x, y) != 0 && tileAt(1, x, y) != 0) {
    // "Composite" tiles (content on both layers) are ignored for collision
    // checking
    return CollisionData{};
  }

  const auto data1 = mAttributes.collisionData(tileAt(0, x, y));
  const auto data2 = mAttributes.collisionData(tileAt(1, x, y));
  return CollisionData{data1, data2};
}


const map::TileIndex& Map::tileRefAt(
  const int layerS,
  const int xS,
  const int yS
) const {
  const auto layer = static_cast<size_t>(layerS);
  const auto x = static_cast<size_t>(xS);
  const auto y = static_cast<size_t>(yS);

  if (layer >= mLayers.size()) {
    throw invalid_argument("Layer index out of bounds");
  }
  if (x >= mWidthInTiles) {
    throw invalid_argument("X coord out of bounds");
  }
  if (y >= mHeightInTiles) {
    throw invalid_argument("Y coord out of bounds");
  }
  return mLayers[layer][x + y*mWidthInTiles];
}


map::TileIndex& Map::tileRefAt(
  const int layer,
  const int x,
  const int y
) {
  return const_cast<map::TileIndex&>(
    static_cast<const Map&>(*this).tileRefAt(layer, x, y));
}


}
