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
#include <data/high_score_list.hpp>

RIGEL_DISABLE_WARNINGS
#include <catch.hpp>
RIGEL_RESTORE_WARNINGS

using namespace rigel;
using namespace data;
using namespace std;

namespace
{

HighScoreList makeList(std::initializer_list<int> scores)
{
  HighScoreList list;

  size_t i = 0;
  for (const auto score : scores)
  {
    list[i].mScore = score;

    ++i;
    if (i >= list.size())
    {
      break;
    }
  }

  return list;
}

} // namespace

namespace rigel::data
{

ostream& operator<<(ostream& stream, const HighScoreEntry& entry)
{
  stream << entry.mScore;
  return stream;
}


ostream& operator<<(ostream& stream, const HighScoreList& list)
{
  stream << '[';

  bool first = true;
  for (const auto& entry : list)
  {
    if (first)
    {
      first = false;
    }
    else
    {
      stream << ", ";
    }

    stream << entry;
  }

  stream << ']';
  return stream;
}

} // namespace rigel::data


TEST_CASE("High score list")
{
  SECTION("List is sorted in reverse order")
  {
    auto list = makeList({10000, 5000, 3000});
    list[4].mScore = 90000;
    list[0].mScore = 200;

    sort(begin(list), end(list));

    const auto expectedList = makeList({90000, 5000, 3000, 200});

    REQUIRE(list == expectedList);
  }

  auto list = makeList({10000, 9000, 8000, 7000, 6000, 500, 450, 400, 300, 10});

  SECTION("Checking score qualification")
  {
    SECTION("Returns false if score is 0")
    {
      auto listWithZeroes = makeList({10000, 9000, 8000, 0, 0, 0, 0, 0, 0, 0});
      REQUIRE(!scoreQualifiesForHighScoreList(0, listWithZeroes));
    }

    SECTION("Returns false if score is too small")
    {
      REQUIRE(!scoreQualifiesForHighScoreList(5, list));
    }

    SECTION("Returns true if score is larger than highest entry")
    {
      REQUIRE(scoreQualifiesForHighScoreList(20000, list));
    }

    SECTION("Returns true if score is equal to existing score")
    {
      REQUIRE(scoreQualifiesForHighScoreList(7000, list));
    }

    SECTION("Returns true if score fits in between existing entries")
    {
      REQUIRE(scoreQualifiesForHighScoreList(8500, list));
    }
  }

  SECTION("Inserting new score")
  {
    SECTION("Inserting at end replaces last element")
    {
      insertNewScore(200, "", list);

      const auto expected =
        makeList({10000, 9000, 8000, 7000, 6000, 500, 450, 400, 300, 200});

      REQUIRE(list == expected);
    }

    SECTION("Inserting at start shifts remaining elements to the right")
    {
      insertNewScore(12000, "", list);

      const auto expected =
        makeList({12000, 10000, 9000, 8000, 7000, 6000, 500, 450, 400, 300});

      REQUIRE(list == expected);
    }

    SECTION("Inserting in the middle shifts consecutive elements to the right")
    {
      insertNewScore(7500, "", list);

      const auto expected =
        makeList({10000, 9000, 8000, 7500, 7000, 6000, 500, 450, 400, 300});

      REQUIRE(list == expected);
    }
  }
}
