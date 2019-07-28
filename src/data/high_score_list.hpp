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

#pragma once

#include "data/game_session_data.hpp"

#include <array>
#include <string>
#include <tuple>


namespace rigel::data {

constexpr auto NUM_HIGH_SCORE_ENTRIES = 10u;

struct HighScoreEntry {
  std::string mName;
  int mScore = 0;
};


inline bool operator<(const HighScoreEntry& lhs, const HighScoreEntry& rhs) {
  // Sort by highest score first
  return rhs.mScore < lhs.mScore;
}


inline bool operator==(const HighScoreEntry& lhs, const HighScoreEntry& rhs) {
  return std::tie(lhs.mName, lhs.mScore) == std::tie(rhs.mName, rhs.mScore);
}


inline bool operator!=(const HighScoreEntry& lhs, const HighScoreEntry& rhs) {
  return !(lhs == rhs);
}


using HighScoreList = std::array<HighScoreEntry, NUM_HIGH_SCORE_ENTRIES>;
using HighScoreListArray = std::array<HighScoreList, NUM_EPISODES>;

bool scoreQualifiesForHighScoreList(int score, const HighScoreList& list);

void insertNewScore(
  int score,
  const std::string& name,
  HighScoreList& list);

}
