/* Copyright (C) 2018, Nikolai Wuttke. All rights reserved.
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

#include "level_hints.hpp"

#include <algorithm>


namespace rigel::data {

std::optional<std::string> LevelHints::getHint(
  const int episode,
  const int level
) const {
  using namespace std;

  auto iHint = find_if(begin(mHints), end(mHints),
    [&](const Hint& hint) {
      return hint.mEpisode == episode && hint.mLevel == level;
    });

  return iHint != end(mHints)
    ? std::optional<std::string>{iHint->mMessage}
    : std::nullopt;
}

}
