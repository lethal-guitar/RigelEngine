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
#include <catch2/catch_test_macros.hpp>
RIGEL_RESTORE_WARNINGS

TEST_CASE("String split")
{
  SECTION("Empty string")
  {
    const std::vector<std::string> v = rigel::strings::split("", ' ');
    const std::vector<std::string> expected{""};
    CHECK(v == expected);
  }

  SECTION("String with no delimiter")
  {
    const std::vector<std::string> v = rigel::strings::split("hello", ' ');
    const std::vector<std::string> expected{"hello"};
    CHECK(v == expected);
  }

  SECTION("String with delimiter")
  {
    const std::vector<std::string> v =
      rigel::strings::split("hello, world", ',');
    const std::vector<std::string> expected{"hello", " world"};
    CHECK(v == expected);
  }

  SECTION("String with other white space delimiters")
  {
    const std::vector<std::string> v =
      rigel::strings::split("hello\nworld", '\n');
    const std::vector<std::string> expected{"hello", "world"};
    CHECK(v == expected);
  }

  SECTION("String with delimiter at the end")
  {
    const std::vector<std::string> v = rigel::strings::split("hello,", ',');
    const std::vector<std::string> expected{"hello", ""};
    CHECK(v == expected);
  }
}

TEST_CASE("String prefix check (startsWith)")
{
  SECTION("Empty string is prefix of itself")
  {
    const bool hasPrefix = rigel::strings::startsWith("", "");
    CHECK(hasPrefix);
  }

  SECTION("Empty string is a prefix for any string")
  {
    const bool hasPrefix = rigel::strings::startsWith("1234", "");
    CHECK(hasPrefix);
  }

  SECTION("String with no prefix")
  {
    const bool hasPrefix = rigel::strings::startsWith("hello", "bye");
    CHECK(!hasPrefix);
  }

  SECTION("String with prefix")
  {
    const bool hasPrefix = rigel::strings::startsWith("1234", "12");
    CHECK(hasPrefix);
  }

  SECTION("String with prefix longer than string")
  {
    const bool hasPrefix = rigel::strings::startsWith("1234", "12345");
    CHECK(!hasPrefix);
  }

  SECTION("String contains the desired substring, but not as a prefix")
  {
    const bool hasPrefix = rigel::strings::startsWith("1234", "234");
    CHECK(!hasPrefix);
  }
}


TEST_CASE("String trim inplace")
{
  SECTION("Trim left")
  {
    SECTION("Empty string still empty after trim left")
    {
      std::string trimmed = "";
      rigel::strings::trimLeft(trimmed);
      CHECK(trimmed == "");
    }

    SECTION("Trim whitespace from the left of the string")
    {
      std::string trimmed = " hello ";
      rigel::strings::trimLeft(trimmed);
      CHECK(trimmed == "hello ");
    }

    SECTION(
      "Trim whitespace from the left of the string but don't change inside it")
    {
      std::string trimmed = " hello world";
      rigel::strings::trimLeft(trimmed);
      CHECK(trimmed == "hello world");
    }

    SECTION(
      "Trim whitespace from the left of the string and don't change the right side")
    {
      std::string trimmed = "hello ";
      rigel::strings::trimLeft(trimmed);
      CHECK(trimmed == "hello ");
    }

    SECTION("Trim all whitespaces from the left of the string")
    {
      std::string trimmed = "  \n\thello ";
      rigel::strings::trimLeft(trimmed);
      CHECK(trimmed == "hello ");
    }

    SECTION("Trims other characters from the left of the string")
    {
      std::string trimmed = "//hello ";
      rigel::strings::trimLeft(trimmed, "/");
      CHECK(trimmed == "hello ");
    }

    SECTION("Empties an all whitespace string")
    {
      std::string trimmed = " \t\n\r";
      rigel::strings::trimLeft(trimmed);
      CHECK(trimmed == "");
    }

    SECTION("Works on r-values")
    {
      const auto trimmed = rigel::strings::trimLeft("\t test");
      CHECK(trimmed == "test");
    }
  }

  SECTION("Trim right")
  {
    SECTION("Empty string still empty after trim right")
    {
      std::string trimmed = "";
      rigel::strings::trimRight(trimmed);
      CHECK(trimmed == "");
    }

    SECTION("Trim whitespace from the right of the string")
    {
      std::string trimmed = " hello ";
      rigel::strings::trimRight(trimmed);
      CHECK(trimmed == " hello");
    }

    SECTION(
      "Trim whitespace from the right of the string but don't change inside it")
    {
      std::string trimmed = "hello world ";
      rigel::strings::trimRight(trimmed);
      CHECK(trimmed == "hello world");
    }

    SECTION(
      "Trim whitespace from the right of the string and don't change the left side")
    {
      std::string trimmed = " hello";
      rigel::strings::trimRight(trimmed);
      CHECK(trimmed == " hello");
    }

    SECTION("Trim all whitespaces from the right of the string")
    {
      std::string trimmed = "  \n\thello\n\t ";
      rigel::strings::trimRight(trimmed);
      CHECK(trimmed == "  \n\thello");
    }

    SECTION("Trims other characters from the right of the string")
    {
      std::string trimmed = "//hello//";
      rigel::strings::trimRight(trimmed, "/");
      CHECK(trimmed == "//hello");
    }

    SECTION("Works on r-values")
    {
      const auto trimmed = rigel::strings::trimRight("test \t");
      CHECK(trimmed == "test");
    }
  }

  SECTION("Trim all")
  {
    SECTION("Empty string still empty after trim")
    {
      std::string trimmed = "";
      rigel::strings::trim(trimmed);
      CHECK(trimmed == "");
    }

    SECTION("Trim whitespace from the right/left of the string")
    {
      std::string trimmed = " hello ";
      rigel::strings::trim(trimmed);
      CHECK(trimmed == "hello");
    }

    SECTION(
      "Trim whitespace from the right/left of the string and don't change the inside")
    {
      std::string trimmed = " hello world ";
      rigel::strings::trim(trimmed);
      CHECK(trimmed == "hello world");
    }

    SECTION("Trims other characters from the string")
    {
      std::string trimmed = "//hello//";
      rigel::strings::trim(trimmed, "/");
      CHECK(trimmed == "hello");
    }

    SECTION("Works on r-values")
    {
      const auto trimmed = rigel::strings::trim(" \ttest \t");
      CHECK(trimmed == "test");
    }
  }
}

TEST_CASE("Changing case")
{
  SECTION("Converts to uppercase")
  {
    const auto converted = rigel::strings::toUppercase("test ExAmPlE, $#32");
    CHECK(converted == "TEST EXAMPLE, $#32");
  }

  SECTION("Converts to lowercase")
  {
    const auto converted = rigel::strings::toLowercase("TEST ExAmPlE, $#32");
    CHECK(converted == "test example, $#32");
  }
}
