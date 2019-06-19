/* Copyright (C) 2017, Nikolai Wuttke. All rights reserved.
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

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>


namespace rigel::base {

/** Read-only array view type
 *
 * This has almost the same interface as std::array, but doesn't store the data
 * itself. Instead, it just holds a pointer plus the number of elements.
 * This allows creating lightweight views into arrays, which are cheap to copy
 * and store, but can be used like an array.
 *
 * Only allows read access.
 */
template<typename T>
class ArrayView {
public:
  using value_type = T;
  using size_type = std::uint32_t;
  using difference_type = std::int32_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using iterator = pointer;
  using const_iterator = const_pointer;

  ArrayView() = default;
  constexpr ArrayView(const T* pData, const size_type size) noexcept
    : mpData(pData)
    , mSize(size)
  {
  }

  // implicit on purpose
  template <std::size_t N>
  constexpr ArrayView(const std::array<T, N>& array) noexcept // NOLINT
    : mpData(array.data())
    , mSize(static_cast<size_type>(N))
  {
  }

  // implicit on purpose
  template <std::size_t N>
  constexpr ArrayView(const T (&array)[N]) noexcept // NOLINT
    : mpData(array)
    , mSize(static_cast<size_type>(N))
  {
  }

  const_iterator begin() const {
    return mpData;
  }

  const_iterator end() const {
    return mpData + mSize;
  }

  const_iterator cbegin() const {
    return mpData;
  }

  const_iterator cend() const {
    return mpData + mSize;
  }

  size_type size() const {
    return mSize;
  }

  bool empty() const {
    return mSize == 0;
  }

  const_reference operator[](const size_type index) const {
    return mpData[index];
  }

  const_reference at(const size_type index) const {
    using namespace std::string_literals;
    if (index >= mSize) {
      throw std::range_error("Index out of range: "s + std::to_string(index));
    }

    return mpData[index];
  }

  const_reference front() const {
    return *begin();
  }

  const_reference back() const {
    return *end();
  }

  const T* data() const {
    return mpData;
  }

private:
  const T* mpData = nullptr;
  size_type mSize = 0;
};

}
