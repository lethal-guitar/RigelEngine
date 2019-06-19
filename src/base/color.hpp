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

#include <cstdint>
#include <tuple>


namespace rigel::base {

struct Color {
  constexpr Color() = default;
  constexpr Color(std::uint8_t r_, std::uint8_t g_, std::uint8_t b_, std::uint8_t a_) noexcept
    : r(r_)
    , g(g_)
    , b(b_)
    , a(a_)
  {
  }

  bool operator==(const Color& other) const noexcept {
    return std::tie(r, g, b, a) == std::tie(other.r, other.g, other.b, other.a);
  }

  bool operator!=(const Color& other) const noexcept {
    return !(*this == other);
  }

  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;
  std::uint8_t a = 0;
};

}
