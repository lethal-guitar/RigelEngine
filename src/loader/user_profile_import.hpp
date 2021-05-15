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

#include "data/high_score_list.hpp"
#include "data/saved_game.hpp"

#include <array>
#include <optional>


namespace rigel::loader
{

struct GameOptions
{
  using ScanCode = std::uint8_t;

  ScanCode mUpKeybinding;
  ScanCode mDownKeybinding;
  ScanCode mLeftKeybinding;
  ScanCode mRightKeybinding;
  ScanCode mJumpKeybinding;
  ScanCode mFireKeybinding;

  data::Difficulty mDifficulty;

  bool mSoundBlasterSoundsOn;
  bool mAdlibSoundsOn;
  bool mPcSpeakersSoundsOn;
  bool mMusicOn;

  std::uint8_t mGameSpeedIndex;
};


data::SaveSlotArray loadSavedGames(const std::string& gamePath);

std::array<data::HighScoreList, data::NUM_EPISODES>
  loadHighScoreLists(const std::string& gamePath);

std::optional<GameOptions> loadOptions(const std::string& gamePath);
} // namespace rigel::loader
