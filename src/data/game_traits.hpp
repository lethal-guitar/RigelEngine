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
#include "data/palette.hpp"

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

  static constexpr int viewportWidthPx = 320;
  static constexpr int viewportHeightPx = 200;
  static constexpr int viewportWidthTiles = viewportWidthPx / tileSize;
  static constexpr int viewportHeightTiles = viewportHeightPx / tileSize;

  static constexpr base::Size viewportSize{viewportWidthPx, viewportHeightPx};

  // The actual in-game viewport starts with an offset and is further reduced
  // to make room for the HUD. The right hand side features another 8px of
  // black border.
  static constexpr base::Vec2 inGameViewportOffset{8, 8};
  static constexpr base::Size inGameViewportSize{
    GameTraits::viewportWidthPx - 16,
    GameTraits::viewportHeightPx - 8};

  static constexpr int mapViewportWidthTiles = viewportWidthTiles - 8;
  static constexpr int mapViewportHeightTiles = viewportHeightTiles - 5;

  static constexpr base::Size mapViewportSize{
    GameTraits::mapViewportWidthTiles,
    GameTraits::mapViewportHeightTiles};

  static constexpr std::size_t egaPlanes = 4u;
  static constexpr std::size_t maskedEgaPlanes = egaPlanes + 1u;
  static constexpr std::size_t fontEgaPlanes = 2u;
  static constexpr std::size_t pixelsPerEgaByte = 8u;

  // This palette is derived from the hardcoded EGA palette in the uncompressed
  // EXE (using unlzexe) at offset 0x1b068 (registered version, might be
  // different in the shareware version). It's very similar to GAMEPAL.PAL, but
  // has some subtle differences, particularly in the blue hues.
  //
  // The original values from the EXE are:
  // 0x00, 0x00, 0x00,  0x10, 0x10, 0x10,  0x20, 0x20, 0x20,  0x30, 0x30, 0x30,
  // 0x20, 0x00, 0x00,  0x30, 0x00, 0x00,  0x40, 0x1C, 0x10,  0x40, 0x40, 0x00,
  // 0x00, 0x10, 0x00,  0x00, 0x00, 0x20,  0x00, 0x00, 0x30,  0x00, 0x00, 0x40,
  // 0x00, 0x20, 0x00,  0x00, 0x30, 0x00,  0x20, 0x10, 0x00,  0x40, 0x40, 0x40

  // clang-format off
  static constexpr Palette16 INGAME_PALETTE{
    data::Pixel{0,     0,   0, 255},
    data::Pixel{60,   60,  60, 255},
    data::Pixel{121, 121, 121, 255},
    data::Pixel{182, 182, 182, 255},
    data::Pixel{121,   0,   0, 255},
    data::Pixel{182,   0,   0, 255},
    data::Pixel{243, 105,  60, 255},
    data::Pixel{243, 243,   0, 255},
    data::Pixel{0,    60,   0, 255},
    data::Pixel{0,     0, 121, 255},
    data::Pixel{0,     0, 182, 255},
    data::Pixel{0,     0, 243, 255},
    data::Pixel{0,   121,   0, 255},
    data::Pixel{0,   182,   0, 255},
    data::Pixel{121,  60,   0, 255},
    data::Pixel{243, 243, 243, 255},
  };
  // clang-format on

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

  static constexpr base::Size menuFontCharacterBitmapSizeTiles{1, 2};

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
    viewportWidthPx / aspectRatio / viewportHeightPx;

  struct CZone
  {
    static constexpr std::size_t numSolidTiles = 1000u;
    static constexpr std::size_t numMaskedTiles = 160u;
    static constexpr std::size_t numTilesTotal = numSolidTiles + numMaskedTiles;

    static constexpr int tileSetImageWidth = viewportWidthTiles;
    static constexpr int solidTilesImageHeight = viewportHeightTiles;
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
