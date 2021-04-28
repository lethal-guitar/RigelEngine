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

#include <base/string_utils.hpp>
#include <base/warnings.hpp>

RIGEL_DISABLE_WARNINGS
#include <catch.hpp>
RIGEL_RESTORE_WARNINGS

TEST_CASE("String split") {
  SECTION("Empty string") {
    const std::vector<std::string> v = rigel::strings::split("", ' ');
    const std::vector<std::string> expected{""};
    CHECK(v == expected);
  }
  SECTION("String with no delimiter") {
    const std::vector<std::string> v = rigel::strings::split("hello", ' ');
    const std::vector<std::string> expected{"hello"};
    CHECK(v == expected);
  }
  SECTION("String with delimiter") {
    const std::vector<std::string> v = rigel::strings::split("hello, world", ',');
    const std::vector<std::string> expected{"hello", " world"};
    CHECK(v == expected);
  }
  SECTION("String with other white space delimiters") {
    const std::vector<std::string> v = rigel::strings::split("hello\nworld", '\n');
    const std::vector<std::string> expected{"hello", "world"};
    CHECK(v == expected);
  }
  SECTION("String with delimiter at the end") {
    const std::vector<std::string> v = rigel::strings::split("hello,", ',');
    const std::vector<std::string> expected{"hello", ""};
    CHECK(v == expected);
  }
}
