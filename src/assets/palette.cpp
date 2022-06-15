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

#include "assets/file_utils.hpp"
#include "base/warnings.hpp"

#include <iostream>


namespace rigel::assets
{

namespace
{

std::uint8_t extend6bitColorValue(const std::uint8_t value)
{
  // See http://www.shikadi.net/moddingwiki/VGA_Palette for details
  // on the 6-bit to 8-bit conversion
  return static_cast<uint8_t>(std::min(255, value * 256 / 63));
}


template <typename PaletteType, typename PreProcessFunc>
PaletteType load6bitPalette(
  const ByteBufferCIter begin,
  const ByteBufferCIter end,
  PreProcessFunc preProcess)
{
  LeStreamReader reader(begin, end);

  PaletteType palette;
  for (auto& entry : palette)
  {
    entry.r = extend6bitColorValue(preProcess(reader.readU8()));
    entry.g = extend6bitColorValue(preProcess(reader.readU8()));
    entry.b = extend6bitColorValue(preProcess(reader.readU8()));
    entry.a = 255;
  }
  return palette;
}

} // namespace


data::Palette16
  load6bitPalette16(ByteBufferCIter begin, const ByteBufferCIter end)
{
  return load6bitPalette<data::Palette16>(begin, end, [](const auto entry) {
    // Duke Nukem 2 uses a non-standard 6-bit palette format, where the
    // maximum number is 68 instead of 63. This maps Duke 2 palette values to
    // normal 6-bit VGA/EGA values.
    //
    // The reason for the non-standard value range is that the game never
    // directly writes these values to the VGA palette. Instead, it submits
    // new palettes always in conjunction with a fade-in effect. During the
    // fade-in, the palette is initially set to all zeroes. Then, the game
    // sends 15 different palettes, with a delay inbetween, in order to create
    // the fading effect. To do this, it stores the palette values in words
    // instead of bytes, and adds the original palette values to the current
    // values each iteration. This ultimately results in 'value * 15'.
    // Since that would be out of range, the value is then divided by 16 before
    // actually submitting it to the VGA hardware.
    return static_cast<std::uint8_t>(entry * 15 / 16);
  });
}


data::Palette256
  load6bitPalette256(ByteBufferCIter begin, const ByteBufferCIter end)
{
  return load6bitPalette<data::Palette256>(begin, end, [](const auto entry) {
    // 256 color palettes use the standard VGA 6-bit format and need no
    // conversion.
    return entry;
  });
}

} // namespace rigel::assets
