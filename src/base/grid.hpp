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

#include <vector>


namespace rigel::base {

template<typename ValueT>
class Grid {
public:
  Grid() = default;
  Grid(const std::size_t width, const std::size_t height)
    : mStorage(width*height)
    , mWidth(width)
    , mHeight(height)
  {
  }

  const ValueT& valueAt(const std::size_t x, const std::size_t y) const {
    return mStorage[x + y*mWidth];
  }

  void setValueAt(const std::size_t x, const std::size_t y, ValueT value) {
    mStorage[x + y*mWidth] = value;
  }

  const ValueT& valueAtWithDefault(
    const std::size_t x,
    const std::size_t y,
    const ValueT& defaultValue
  ) const {
    if (x >= mWidth || y >= mHeight) {
      return defaultValue;
    }
    return valueAt(x, y);
  }

  std::size_t width() const {
    return mWidth;
  }

  std::size_t height() const {
    return mHeight;
  }

private:
  std::vector<ValueT> mStorage;
  const std::size_t mWidth = 0;
  const std::size_t mHeight = 0;
};

}
