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

#include "base/spatial_types.hpp"

#include <cstddef>


namespace rigel::data
{

enum class TileImageType
{
  Unmasked,
  Masked
};


struct GameTraits
{

  static constexpr int tileSize = 8;
  static constexpr int tileSizeSquared = tileSize * tileSize;

  static constexpr int viewPortWidthPx = 320;
  static constexpr int viewPortHeightPx = 200;
  static constexpr int viewPortWidthTiles = viewPortWidthPx / tileSize;
  static constexpr int viewPortHeightTiles = viewPortHeightPx / tileSize;

  static constexpr base::Extents viewPortSize{
    viewPortWidthPx,
    viewPortHeightPx};

  // The actual in-game viewport starts with an offset and is further reduced
  // to make room for the HUD. The right hand side features another 8px of
  // black border.
  static constexpr base::Vector inGameViewPortOffset{8, 8};
  static constexpr base::Extents inGameViewPortSize{
    GameTraits::viewPortWidthPx - 16,
    GameTraits::viewPortHeightPx - 8};

  static constexpr int mapViewPortWidthTiles = viewPortWidthTiles - 8;
  static constexpr int mapViewPortHeightTiles = viewPortHeightTiles - 5;

  static constexpr base::Extents mapViewPortSize{
    GameTraits::mapViewPortWidthTiles,
    GameTraits::mapViewPortHeightTiles};

  static constexpr std::size_t egaPlanes = 4u;
  static constexpr std::size_t maskedEgaPlanes = egaPlanes + 1u;
  static constexpr std::size_t fontEgaPlanes = 2u;
  static constexpr std::size_t pixelsPerEgaByte = 8u;

  static constexpr std::size_t numPlanes(const TileImageType type)
  {
    return type == TileImageType::Masked ? maskedEgaPlanes : egaPlanes;
  }

  static constexpr std::size_t bytesPerTile(const TileImageType type)
  {
    return (tileSizeSquared / pixelsPerEgaByte) * numPlanes(type);
  }

  static constexpr std::size_t bytesPerFontTile()
  {
    return (tileSizeSquared / pixelsPerEgaByte) * fontEgaPlanes;
  }

  static constexpr base::Extents menuFontCharacterBitmapSizeTiles{1, 2};

  static constexpr std::size_t mapDataWords = 32750u;

  static constexpr std::size_t mapHeightForWidth(const std::size_t width)
  {
    return mapDataWords / width;
  }

  static constexpr int minDrawOrder = -1;
  static constexpr int maxDrawOrder = 4;

  // The game's original 320x200 resolution would give us a 16:10 aspect ratio
  // when using square pixels, but monitors of the time had a 4:3 aspect ratio,
  // and that's what the game's graphics were designed for (very noticeable e.g.
  // with the earth in the Apogee logo). CRTs are not limited to square pixels,
  // and the monitor would stretch the 320x200 into the right shape for a 4:3
  // picture.
  static constexpr auto aspectRatio = 4.0f / 3.0f;
  static constexpr auto aspectCorrectionStretchFactor =
    viewPortWidthPx / aspectRatio / viewPortHeightPx;

  struct CZone
  {
    static constexpr std::size_t numSolidTiles = 1000u;
    static constexpr std::size_t numMaskedTiles = 160u;
    static constexpr std::size_t numTilesTotal = numSolidTiles + numMaskedTiles;

    static constexpr int tileSetImageWidth = viewPortWidthTiles;
    static constexpr int solidTilesImageHeight = viewPortHeightTiles;
    static constexpr int tileSetImageHeight =
      solidTilesImageHeight + numMaskedTiles / tileSetImageWidth;

    static constexpr std::size_t tileBytes =
      egaPlanes * tileSize * sizeof(std::uint8_t);
    static constexpr std::size_t tileBytesMasked =
      maskedEgaPlanes * tileSize * sizeof(std::uint8_t);

    static constexpr std::size_t attributeBytes = sizeof(std::uint16_t);

    static constexpr std::size_t attributeBytesTotal =
      attributeBytes * numSolidTiles + attributeBytes * 5 * numMaskedTiles;
  };

  static constexpr auto musicPlaybackRate = 280;
};


} // namespace rigel::data
