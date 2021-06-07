/* Copyright (C) 2021, Nikolai Wuttke. All rights reserved.
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

#include <base/array_view.hpp>
#include <base/warnings.hpp>

RIGEL_DISABLE_WARNINGS
#include <catch.hpp>
RIGEL_RESTORE_WARNINGS

#include <array>
#include <vector>

TEST_CASE("Array view constexpr")
{
  SECTION("pointer and size")
  {
    static constexpr auto expectedSize = 5;
    static constexpr int arr[expectedSize]{0, 1, 2, 3, 4};
    static constexpr rigel::base::ArrayView<int> arrv{&arr[0], 5};
    static_assert(arrv.size() == 5);
    static_assert(!arrv.empty());
    static_assert(arrv[0] == 0);
  }
  SECTION("c array")
  {
    static constexpr auto expectedSize = 5;
    static constexpr int arr[expectedSize]{0, 1, 2, 3, 4};
    static constexpr rigel::base::ArrayView<int> arrv{arr};
    static_assert(arrv.size() == 5);
    static_assert(!arrv.empty());
    static_assert(arrv[0] == 0);
  }
  SECTION("std array")
  {
    static constexpr std::array<int, 5> arr{0, 1, 2, 3, 4};
    static constexpr rigel::base::ArrayView<int> arrv{arr};
    static_assert(arrv.size() == arr.size());
    static_assert(!arrv.empty());
    static_assert(arrv[0] == 0);
  }
}

TEST_CASE("Array view vector")
{
  const std::vector<int> vec{0, 1, 2, 3, 4};
  const auto expectedSize = vec.size();
  const rigel::base::ArrayView<int> arrv{vec};
  CHECK(arrv.size() == expectedSize);
  CHECK(!arrv.empty());
  CHECK(*arrv.begin() == 0);
}
