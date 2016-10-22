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

#pragma once

#include <base/spatial_types.hpp>

#include <cstddef>


namespace rigel { namespace data {

struct GameTraits {

  static const int tileSize = 8;
  static const int tileSizeSquared = tileSize*tileSize;

  static const int viewPortWidthPx = 320;
  static const int viewPortHeightPx = 200;
  static const int viewPortWidthTiles =
    viewPortWidthPx / tileSize;
  static const int viewPortHeightTiles =
    viewPortHeightPx / tileSize;

  // The actual in-game viewport starts with an offset and is further reduced
  // to make room for the HUD. The right hand side features another 8px of
  // black border.
  static const base::Vector inGameViewPortOffset;
  static const base::Extents inGameViewPortSize;

  static const int mapViewPortWidthTiles =
    viewPortWidthTiles - 7;
  static const int mapViewPortHeightTiles =
    viewPortHeightTiles - 5;

  static const base::Extents mapViewPortSize;

  static const std::size_t egaPlanes = 4u;
  static const std::size_t maskedEgaPlanes = egaPlanes + 1u;
  static const std::size_t fontEgaPlanes = 2u;
  static const std::size_t pixelsPerEgaByte = 8u;

  static constexpr std::size_t numPlanes(const bool isMasked) {
    return isMasked ? maskedEgaPlanes : egaPlanes;
  }

  static constexpr std::size_t bytesPerTile(const bool isMasked) {
    return (tileSizeSquared / pixelsPerEgaByte) * numPlanes(isMasked);
  }

  static constexpr std::size_t bytesPerFontTile() {
    return (tileSizeSquared / pixelsPerEgaByte) * fontEgaPlanes;
  }

  static const std::size_t mapDataWords = 32750u;

  static constexpr std::size_t mapHeightForWidth(const std::size_t width) {
    return mapDataWords / width;
  }

  struct CZone {
    static const std::size_t numSolidTiles = 1000u;
    static const std::size_t numMaskedTiles = 160u;
    static const std::size_t numTilesTotal = numSolidTiles + numMaskedTiles;

    static const int tileSetImageWidth = viewPortWidthTiles;
    static const int solidTilesImageHeight = viewPortHeightTiles;
    static const int tileSetImageHeight =
      solidTilesImageHeight + numMaskedTiles/tileSetImageWidth;

    static const std::size_t tileBytes =
      egaPlanes * tileSize * sizeof(std::uint8_t);
    static const std::size_t tileBytesMasked =
      maskedEgaPlanes * tileSize * sizeof(std::uint8_t);

    static const std::size_t attributeBytes = sizeof(std::uint16_t);

    static const std::size_t attributeBytesTotal =
      attributeBytes*numSolidTiles + attributeBytes*5*numMaskedTiles;
  };
};


}}
