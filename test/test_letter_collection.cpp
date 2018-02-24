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

#include <catch.hpp>
#include "utils.hpp"

#include <data/player_model.hpp>
#include <utils/container_tools.hpp>

#include <vector>

using namespace rigel;
using namespace data;

using namespace std;

using LT = CollectableLetterType;
using S = PlayerModel::LetterCollectionState;
using ExpectedState = pair<LT, S>;


namespace {

vector<LT> getLetters(const vector<ExpectedState>& expectations) {
  return utils::transformed(expectations, [](const auto& pair) {
    return pair.first;
  });
}


vector<S> getStates(const vector<ExpectedState>& expectations) {
  return utils::transformed(expectations, [](const auto& pair) {
    return pair.second;
  });
}


vector<S> collectLetters(const vector<LT>& letters) {
  PlayerModel model;

  return utils::transformed(letters, [&model](const auto letter) {
    return model.addLetter(letter);
  });
}

}


TEST_CASE("Collectable letters") {
  auto testIt = [](const vector<ExpectedState>& dataSet) {
    CHECK(getStates(dataSet) == collectLetters(getLetters(dataSet)));
  };

  SECTION("In order") {
    testIt({
      {LT::N, S::Incomplete},
      {LT::U, S::Incomplete},
      {LT::K, S::Incomplete},
      {LT::E, S::Incomplete},
      {LT::M, S::InOrder}
    });
  }

  SECTION("In order, except last two") {
    testIt({
      {LT::N, S::Incomplete},
      {LT::U, S::Incomplete},
      {LT::K, S::Incomplete},
      {LT::M, S::Incomplete},
      {LT::E, S::WrongOrder}
    });
  }

  SECTION("Reverse order") {
    testIt({
      {LT::M, S::Incomplete},
      {LT::E, S::Incomplete},
      {LT::K, S::Incomplete},
      {LT::U, S::Incomplete},
      {LT::N, S::WrongOrder}
    });
  }

  SECTION("Random order") {
    testIt({
      {LT::K, S::Incomplete},
      {LT::N, S::Incomplete},
      {LT::U, S::Incomplete},
      {LT::M, S::Incomplete},
      {LT::E, S::WrongOrder}
    });
  }
}
