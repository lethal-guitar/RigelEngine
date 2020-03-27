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

#include "data/image.hpp"
#include "loader/byte_buffer.hpp"

#include <array>
#include <cstdint>


namespace rigel::loader {

using Palette16 = std::array<data::Pixel, 16>;
using Palette256 = std::array<data::Pixel, 256>;


extern const Palette16 INGAME_PALETTE;


Palette16 load6bitPalette16(ByteBufferCIter begin, ByteBufferCIter end);


Palette256 load6bitPalette256(ByteBufferCIter begin, ByteBufferCIter end);


inline Palette16 load6bitPalette16(const ByteBuffer& buffer) {
  return load6bitPalette16(buffer.begin(), buffer.end());
}


inline Palette256 load6bitPalette256(const ByteBuffer& buffer) {
  return load6bitPalette256(buffer.begin(), buffer.end());
}

}
