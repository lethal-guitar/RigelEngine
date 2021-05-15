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


namespace rigel::data
{

constexpr auto NUM_EPISODES = 4;
constexpr auto NUM_LEVELS_PER_EPISODE = 8;

enum class Difficulty
{
  Easy = 0,
  Medium = 1,
  Hard = 2
};


struct GameSessionId
{
  GameSessionId() = default;
  GameSessionId(const int episode, const int level, const Difficulty difficulty)
    : mEpisode(episode)
    , mLevel(level)
    , mDifficulty(difficulty)
  {
  }

  bool needsRegisteredVersion() const { return mEpisode > 0; }

  int mEpisode = 0;
  int mLevel = 0;
  Difficulty mDifficulty = Difficulty::Medium;
};


constexpr bool isBossLevel(const int level)
{
  return level == NUM_LEVELS_PER_EPISODE - 1;
}


} // namespace rigel::data
