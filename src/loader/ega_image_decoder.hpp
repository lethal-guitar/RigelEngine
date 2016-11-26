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

#include "loader/byte_buffer.hpp"
#include "loader/palette.hpp"
#include "data/image.hpp"


namespace rigel { namespace loader {

data::PixelBuffer decodeSimplePlanarEgaBuffer(
  ByteBufferCIter begin,
  ByteBufferCIter end,
  const Palette16& palette);


data::Image loadTiledImage(
  ByteBufferCIter begin,
  ByteBufferCIter end,
  std::size_t widthInTiles,
  const Palette16& palette,
  bool isMasked);


inline data::Image loadTiledImage(
  const ByteBuffer& data,
  std::size_t widthInTiles,
  const Palette16& palette,
  bool isMasked = false
) {
  return loadTiledImage(
    data.cbegin(),
    data.cend(),
    widthInTiles,
    palette,
    isMasked);
}

data::Image loadTiledFontBitmap(
  ByteBufferCIter begin,
  ByteBufferCIter end,
  std::size_t widthInTiles
);

}}
