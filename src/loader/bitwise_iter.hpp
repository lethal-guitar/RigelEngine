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

#include <cassert>
#include <cstddef>
#include <iterator>


namespace rigel { namespace loader {

/** Adapter iterator which returns individual bits from a sequence of values
 */
template<typename OriginalIterType>
class BitWiseIterator {
public:
  using value_type =
      typename std::iterator_traits<OriginalIterType>::value_type;

  BitWiseIterator(
    OriginalIterType originalIter,
    OriginalIterType originalEnd,
    bool lsbFirst = false
  )
    : mOriginalIter(originalIter)
    , mOriginalEnd(originalEnd)
    , mBitIndex(0)
    , mLsbFirst(lsbFirst)
    , mIsAtEnd(originalIter == originalEnd)
  {
  }

  bool operator==(const BitWiseIterator& other) {
    return
      other.mOriginalIter == mOriginalIter &&
      other.mOriginalEnd == mOriginalEnd &&
      other.mBitIndex == mBitIndex &&
      other.mLsbFirst == mLsbFirst &&
      other.mIsAtEnd == mIsAtEnd;
  }

  bool operator!=(const BitWiseIterator& other) {
    return !(*this == other);
  }

  BitWiseIterator operator++(int) {
    BitWiseIterator copy(*this);
    ++(*this);
    return copy;
  }

  BitWiseIterator& operator++() {
    assert(!mIsAtEnd);

    mBitIndex++;
    if (mBitIndex == NUM_BITS) {
      mOriginalIter++;
      mBitIndex = 0;

      if (mOriginalIter == mOriginalEnd) {
        mIsAtEnd = true;
      }
    }

    return *this;
  }

  int operator*() {
    assert(!mIsAtEnd);

    const auto bitPack = *mOriginalIter;
    const auto actualBitIndex = mLsbFirst
      ? mBitIndex
      : (NUM_BITS - 1) - mBitIndex;

    const auto bitMask = 1 << actualBitIndex;
    return (bitPack & bitMask) != 0;
  }

private:
  static const auto NUM_BITS = sizeof(value_type)*8;

  OriginalIterType mOriginalIter;
  const OriginalIterType mOriginalEnd;
  std::uint8_t mBitIndex;
  const bool mLsbFirst;
  bool mIsAtEnd;
};

}}
