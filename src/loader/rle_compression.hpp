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

#include "loader/file_utils.hpp"

#include <optional>


namespace rigel::loader {

/** Expand single RLE word by calling callback for each output byte */
template<typename Callable>
void expandSingleRleWord(
  const std::int8_t marker,
  LeStreamReader& reader,
  Callable callback
) {
  if (marker > 0) {
    const auto byteToRepeat = reader.readU8();
    const auto numRepetitions = marker;
    for (int i=0; i<numRepetitions; ++i) {
      callback(byteToRepeat);
    }
  } else {
    const auto numBytesToCopy = -marker;
    for (int i=0; i<numBytesToCopy; ++i) {
      callback(reader.readU8());
    }
  }
}


/** Decompress RLE data with unknown size - assumes terminating 0 marker */
template<typename Callable>
void decompressRle(
  LeStreamReader& reader,
  Callable callback
) {
  for (;;) {
    const auto marker = reader.readS8();
    if (marker == 0) {
      break;
    }

    expandSingleRleWord(marker, reader, callback);
  }
}


/** Decompress RLE data with known size (number of RLE marker words) */
template<typename Callable>
void decompressRle(
  LeStreamReader& reader,
  const std::size_t numRleWords,
  Callable callback
) {
  for (auto wordsRead = 0u; wordsRead < numRleWords; ++wordsRead) {
    expandSingleRleWord(reader.readS8(), reader, callback);
  }
}


}
