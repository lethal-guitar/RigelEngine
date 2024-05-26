/* Copyright (C) 2019, Nikolai Wuttke. All rights reserved.
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

#include <base/warnings.hpp>
#include <frontend/json_utils.hpp>

RIGEL_DISABLE_WARNINGS
#include <catch2/catch_test_macros.hpp>
RIGEL_RESTORE_WARNINGS


using namespace rigel;

TEST_CASE("JSON merging")
{
  auto makeExampleObject = []() {
    auto result = nlohmann::json{};
    result["valueOne"] = 200;
    result["valueTwo"] = "Hi";
    result["valueThree"] = true;

    auto nested = nlohmann::json::object();
    nested["setting"] = 2.0f;
    nested["doublyNested"] = nlohmann::json::object();
    nested["doublyNested"]["stuff"] = nullptr;
    result["nestedObject"] = nested;

    auto nestedArray = nlohmann::json::array();
    nestedArray.push_back("Hi");
    nestedArray.push_back("Hey");

    auto nestedInArray = nlohmann::json{};
    nestedInArray["someValue"] = 20;
    nestedArray.push_back(nestedInArray);
    result["nestedArray"] = nestedArray;

    return result;
  };

  const auto base = makeExampleObject();

  SECTION("Empty extension has no effect")
  {
    const auto merged = merge(base, nlohmann::json::object());

    CHECK(merged == base);
  }

  SECTION("Unrelated properties in base are unaffected by extension")
  {
    auto extension = nlohmann::json{};
    extension["anotherProp"] = 42;

    auto merged = merge(base, extension);

    merged.erase("anotherProp");
    CHECK(merged == base);
  }

  SECTION("Properties from extension are added to base")
  {
    auto extension = nlohmann::json{};
    extension["anotherProp"] = 42;

    const auto merged = merge(base, extension);

    CHECK(merged["anotherProp"].get<int>() == 42);
  }

  SECTION("Properties from extension overwrite their counterparts in base")
  {
    auto extension = nlohmann::json{};
    extension["valueOne"] = 42;

    auto merged = merge(base, extension);

    CHECK(merged["valueOne"].get<int>() == 42);

    SECTION("Other properties retain their value")
    {
      merged["valueOne"] = base["valueOne"];

      CHECK(merged == base);
    }
  }

  SECTION(
    "Properties from nested object in extension overwrite counterparts in base")
  {
    auto nestedObject = nlohmann::json{};
    nestedObject["setting"] = 3.0f;
    auto extension = nlohmann::json{};
    extension["nestedObject"] = nestedObject;

    auto merged = merge(base, extension);

    CHECK(merged["nestedObject"]["setting"].get<float>() == 3.0f);

    SECTION("Other properties retain their value")
    {
      merged["nestedObject"]["setting"] = base["nestedObject"]["setting"];

      CHECK(merged == base);
    }
  }

  SECTION(
    "Properties from object in array in extension overwrite counterparts in base")
  {
    auto nestedObject = nlohmann::json{};
    nestedObject["someValue"] = 24;
    auto extensionArray = base["nestedArray"];
    extensionArray[2] = nestedObject;
    auto extension = nlohmann::json{};
    extension["nestedArray"] = extensionArray;

    auto merged = merge(base, extension);

    CHECK(merged["nestedArray"][2]["someValue"].get<int>() == 24);

    SECTION("Other properties retain their value")
    {
      merged["nestedArray"][2]["someValue"] =
        base["nestedArray"][2]["someValue"];

      CHECK(merged == base);
    }
  }

  SECTION("Array of primitives in extension overwrites counterpart in base")
  {
    auto objectWithValues = nlohmann::json{};
    objectWithValues["values"] = nlohmann::json::array();
    objectWithValues["values"].push_back(1);
    objectWithValues["values"].push_back("Test");
    objectWithValues["values"].push_back(false);

    auto extension = objectWithValues;
    extension["values"] = nlohmann::json::array();
    extension["values"].push_back("testing1");
    extension["values"].push_back("testing2");

    auto merged = merge(objectWithValues, extension);

    auto expectedValues = extension["values"];
    CHECK(merged["values"] == expectedValues);
  }

  SECTION(
    "Array of primitives in base remains unchanged when not present in extension")
  {
    auto objectWithValues = nlohmann::json{};
    objectWithValues["values"] = nlohmann::json::array();
    objectWithValues["values"].push_back(1);
    objectWithValues["values"].push_back("Test");
    objectWithValues["values"].push_back(false);

    auto extension = nlohmann::json{};
    extension["something"] = 1.0f;

    auto merged = merge(objectWithValues, extension);

    auto expectedValues = objectWithValues["values"];
    CHECK(merged["values"] == expectedValues);
  }
}
