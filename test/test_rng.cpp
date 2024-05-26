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

#include <base/warnings.hpp>
#include <engine/random_number_generator.hpp>

RIGEL_DISABLE_WARNINGS
#include <catch2/catch_test_macros.hpp>
RIGEL_RESTORE_WARNINGS

#include <algorithm>

TEST_CASE("Random number generator works the same as the original")
{
  rigel::engine::RandomNumberGenerator rng;
  SECTION("Values are generated in this particular order")
  {
    SECTION("First generated value is 8 (the 1st one in the array)")
    {
      CHECK(rng.gen() == 8u);
    }
    SECTION("The period of the RNG is the original array's size")
    {
      const auto firstRandomValue = rng.gen();
      for (unsigned i = 0u; i < rigel::engine::RANDOM_NUMBER_TABLE.size() - 1;
           ++i)
      {
        (void)rng.gen(); // ignore these
      }
      const auto randomValueAfterPeriod = rng.gen();
      CHECK(firstRandomValue == randomValueAfterPeriod);
    }
    SECTION("Test RNG generates in the same order")
    {
      std::vector<int> randomNumbers(
        rigel::engine::RANDOM_NUMBER_TABLE.size() * 2,
        std::numeric_limits<int>::min());
      auto beg = std::begin(randomNumbers);
      auto end = std::end(randomNumbers);
      std::generate(beg, end, [&rng]() { return rng.gen(); });
      // since we start from the 1st index in the original array, rotate once
      std::rotate(
        randomNumbers.rbegin(),
        randomNumbers.rbegin() + 1,
        randomNumbers.rend());
      auto half =
        std::next(std::begin(randomNumbers), randomNumbers.size() / 2);
      const auto halfDistance = std::distance(beg, half);
      CHECK(halfDistance == std::distance(half, end));
      CHECK(
        static_cast<size_t>(halfDistance) ==
        rigel::engine::RANDOM_NUMBER_TABLE.size());
      CHECK(std::equal(beg, half, half));
    }
  }
}
