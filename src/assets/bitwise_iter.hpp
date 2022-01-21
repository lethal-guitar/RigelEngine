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

#include <cstddef>
#include <iterator>


namespace rigel::assets
{

/** Adapter iterator which returns individual bits from a sequence of values
 *
 * Fulfills the InputIterator concept. Depending on the underlying original
 * iterator, it can be used in multi-pass algorithms, even though it doesn't
 * satisfy the requirements for ForwardIterator.
 */
template <typename OriginalIterType>
class BitWiseIterator
{
public:
  using value_type = bool;
  using reference = value_type&;
  using pointer = value_type*;
  using difference_type =
    typename std::iterator_traits<OriginalIterType>::difference_type;
  using iterator_category = std::input_iterator_tag;

  explicit BitWiseIterator(OriginalIterType originalIter)
    : mOriginalIter(originalIter)
    , mBitIndex(0)
  {
  }

  bool operator==(const BitWiseIterator& other)
  {
    return other.mOriginalIter == mOriginalIter && other.mBitIndex == mBitIndex;
  }

  bool operator!=(const BitWiseIterator& other) { return !(*this == other); }

  BitWiseIterator operator++(int)
  {
    BitWiseIterator copy(*this);
    ++(*this);
    return copy;
  }

  BitWiseIterator& operator++()
  {
    ++mBitIndex;
    if (mBitIndex == NUM_BITS)
    {
      ++mOriginalIter;
      mBitIndex = 0;
    }

    return *this;
  }

  bool operator*()
  {
    const auto actualBitIndex = (NUM_BITS - 1) - mBitIndex;
    const auto bitMask = 1 << actualBitIndex;

    const auto bitPack = *mOriginalIter;
    return (bitPack & bitMask) != 0;
  }

private:
  static const auto NUM_BITS = sizeof(value_type) * 8;

  OriginalIterType mOriginalIter;
  std::uint8_t mBitIndex;
};

} // namespace rigel::assets
