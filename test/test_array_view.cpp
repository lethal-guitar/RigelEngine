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

#include <algorithm>
#include <array>
#include <string>
#include <vector>

TEST_CASE("Array view constexpr")
{
  SECTION("pointer and size")
  {
    constexpr int expectedSize = 5;
    static constexpr int array[expectedSize]{0, 1, 2, 3, 4};
    constexpr rigel::base::ArrayView<int> view{array, 5};

    static_assert(view.size() == expectedSize);
    static_assert(!view.empty());
    static_assert(view[0] == 0);
  }

  SECTION("c array")
  {
    constexpr auto expectedSize = 5;
    static constexpr int array[expectedSize]{0, 1, 2, 3, 4};
    constexpr rigel::base::ArrayView<int> view{array};

    static_assert(view.size() == expectedSize);
    static_assert(!view.empty());
    static_assert(view[0] == 0);
  }

  SECTION("std array")
  {
    static constexpr std::array<int, 5> array{0, 1, 2, 3, 4};
    constexpr rigel::base::ArrayView<int> view{array};

    static_assert(view.size() == array.size());
    static_assert(!view.empty());
    static_assert(view[0] == 0);
  }
}

TEST_CASE("Array view vector")
{
  const std::vector<int> vec{0, 1, 2, 3, 4};
  const auto expectedSize = vec.size();
  const rigel::base::ArrayView<int> view{vec};

  CHECK(view.size() == expectedSize);
  CHECK(!view.empty());
  CHECK(std::equal(std::begin(vec), std::end(vec), std::begin(view)));
}

TEST_CASE("Array view string")
{
  const std::string string{"hello, rigel!"};
  const auto expectedSize = string.size();
  const rigel::base::ArrayView<char> view{string};

  CHECK(view.size() == expectedSize);
  CHECK(!view.empty());
  CHECK(std::equal(std::begin(string), std::end(string), std::begin(view)));
}

TEST_CASE("Array view CTAD")
{
  SECTION("std vector")
  {
    const std::vector<int> vec{0, 1, 2, 3, 4};
    const rigel::base::ArrayView view{vec};
  }

  SECTION("std string")
  {
    const std::string string{"hello, rigel!"};
    const rigel::base::ArrayView view{string};
  }

  SECTION("std array")
  {
    const std::array<double, 5> array{0.1, 0.2, 0.3, 0.4, 0.5};
    const rigel::base::ArrayView view{array};
  }
}
