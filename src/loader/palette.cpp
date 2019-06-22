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

#include "palette.hpp"

#include "base/warnings.hpp"
#include "loader/file_utils.hpp"

#include <iostream>


namespace rigel::loader {

namespace {

std::uint8_t extend6bitColorValue(const std::uint8_t value) {
  // See http://www.shikadi.net/moddingwiki/VGA_Palette for details
  // on the 6-bit to 8-bit conversion
  return static_cast<uint8_t>(std::min(255, (value*255) / 63));
}


template<typename PaletteType, typename PreProcessFunc>
PaletteType load6bitPalette(
  const ByteBufferCIter begin,
  const ByteBufferCIter end,
  PreProcessFunc preProcess
) {
  LeStreamReader reader(begin, end);

  PaletteType palette;
  for (auto& entry : palette) {
    entry.r = extend6bitColorValue(preProcess(reader.readU8()));
    entry.g = extend6bitColorValue(preProcess(reader.readU8()));
    entry.b = extend6bitColorValue(preProcess(reader.readU8()));
    entry.a = 255;
  }
  return palette;
}

}

// This palette is derived from the hardcoded EGA palette in the uncompressed
// EXE (using unlzexe) at offset 0x1b038 (registered version, might be
// different in the shareware version). It's very similar to GAMEPAL.PAL, but
// has some subtle differences, particularly in the blue hues.
//
// The original values from the EXE are:
// 0x00, 0x00, 0x00,  0x10, 0x10, 0x10,  0x20, 0x20, 0x20,  0x30, 0x30, 0x30,
// 0x20, 0x00, 0x00,  0x30, 0x00, 0x00,  0x40, 0x1C, 0x10,  0x40, 0x40, 0x00,
// 0x00, 0x10, 0x00,  0x00, 0x00, 0x20,  0x00, 0x00, 0x30,  0x00, 0x00, 0x40,
// 0x00, 0x20, 0x00,  0x00, 0x30, 0x00,  0x20, 0x10, 0x00,  0x40, 0x40, 0x40
const Palette16 INGAME_PALETTE{
  data::Pixel{0,     0,   0, 255},
  data::Pixel{60,   60,  60, 255},
  data::Pixel{121, 121, 121, 255},
  data::Pixel{182, 182, 182, 255},
  data::Pixel{121,   0,   0, 255},
  data::Pixel{182,   0,   0, 255},
  data::Pixel{242, 105,  60, 255},
  data::Pixel{242, 242,   0, 255},
  data::Pixel{0,    60,   0, 255},
  data::Pixel{0,     0, 121, 255},
  data::Pixel{0,     0, 182, 255},
  data::Pixel{0,     0, 242, 255},
  data::Pixel{0,   121,   0, 255},
  data::Pixel{0,   182,   0, 255},
  data::Pixel{121,  60,   0, 255},
  data::Pixel{242, 242, 242, 255}
};


Palette16 load6bitPalette16(ByteBufferCIter begin, const ByteBufferCIter end) {
  return load6bitPalette<Palette16>(begin, end, [](const auto entry) {
    // Duke Nukem 2 uses a non-standard 6-bit palette format, where the maximum
    // number is 68 instead of 63. This maps Duke 2 palette values to normal
    // 6-bit VGA/EGA values.
    //
    // See http://www.shikadi.net/moddingwiki/Duke_Nukem_II_Palette_Formats
    const auto baseColorValue = entry - 1;
    return static_cast<std::uint8_t>(
      std::max(0, baseColorValue - baseColorValue/16));
  });
}


Palette256 load6bitPalette256(
  ByteBufferCIter begin,
  const ByteBufferCIter end
) {
  return load6bitPalette<Palette256>(begin, end, [](const auto entry) {
    // 256 color palettes use the standard VGA 6-bit format and need no
    // conversion.
    return entry;
  });
}

}
