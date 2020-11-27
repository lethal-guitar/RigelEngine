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

#include "user_profile_import.hpp"

#include "data/game_session_data.hpp"
#include "data/tutorial_messages.hpp"
#include "loader/file_utils.hpp"

#include <algorithm>


namespace rigel::loader {

namespace {

std::array<std::string, data::NUM_SAVE_SLOTS> loadNameList(
  const std::string& filename
) {
  std::array<std::string, data::NUM_SAVE_SLOTS> result;

  const auto data = loadFile(filename);
  auto reader = LeStreamReader{data};

  for (auto& name : result) {
    name = readFixedSizeString(reader, 18);
  }

  return result;
}


data::TutorialMessageState readTutorialMessageFlags(LeStreamReader& reader) {
  data::TutorialMessageState state;

  reader.skipBytes(4u);
  for (int i = 0; i < data::NUM_TUTORIAL_MESSAGES; ++i) {
    const auto hasBeenShown = reader.readU8() != 0;
    if (hasBeenShown) {
      state.markAsShown(static_cast<data::TutorialMessageId>(i));
    }
  }

  reader.skipBytes(5u);

  return state;
}


data::Difficulty readDifficulty(LeStreamReader& reader) {
  const auto difficultyIndex = std::clamp<std::uint16_t>(reader.readU16(), 1, 3);
  return static_cast<data::Difficulty>(difficultyIndex - 1);
}


data::SavedGame loadSavedGame(
  const std::string& filename,
  std::string saveSlotName
) {
  static_assert(
    static_cast<int>(data::WeaponType::Normal) == 0 &&
    static_cast<int>(data::WeaponType::Laser) == 1 &&
    static_cast<int>(data::WeaponType::Rocket) == 2 &&
    static_cast<int>(data::WeaponType::FlameThrower) == 3);

  const auto data = loadFile(filename);
  auto reader = LeStreamReader{data};

  const auto weaponIndex = std::min<std::uint16_t>(reader.readU16(), 3);
  const auto weapon = static_cast<data::WeaponType>(weaponIndex);
  reader.skipBytes(sizeof(std::uint16_t));
  const auto ammo = std::min<std::uint16_t>(
    reader.readU16(),
    weapon == data::WeaponType::FlameThrower
      ? data::MAX_AMMO_FLAME_THROWER
      : data::MAX_AMMO);
  const auto difficulty = readDifficulty(reader);
  const auto episode =
    std::min<std::uint16_t>(reader.readU16(), data::NUM_EPISODES - 1);
  const auto level =
    std::min<std::uint16_t>(reader.readU16(), data::NUM_LEVELS_PER_EPISODE - 1);
  const auto tutorialMessagesAlreadySeen = readTutorialMessageFlags(reader);
  const auto score = std::min<std::uint32_t>(reader.readU32(), data::MAX_SCORE);

  return {
    data::GameSessionId{episode, level, difficulty},
    tutorialMessagesAlreadySeen,
    std::move(saveSlotName),
    weapon,
    static_cast<int>(ammo),
    static_cast<int>(score)
  };
}


data::HighScoreList loadHighScoreList(const std::string& filename) {
  using std::begin;
  using std::end;
  using std::sort;

  data::HighScoreList list;

  const auto data = loadFile(filename);
  auto reader = LeStreamReader{data};

  for (auto i = 0u; i < data::NUM_HIGH_SCORE_ENTRIES; ++i) {
    list[i].mName = readFixedSizeString(reader, 15);
    list[i].mScore = std::min<std::uint32_t>(reader.readU32(), data::MAX_SCORE);
  }

  sort(begin(list), end(list));
  return list;
}

}


data::SaveSlotArray loadSavedGames(const std::string& gamePath) {
  std::array<std::optional<data::SavedGame>, data::NUM_SAVE_SLOTS> result;

  try {
    const auto nameList = loadNameList(gamePath + "NUKEM2.-NM");

    for (auto i = 0u; i < data::NUM_SAVE_SLOTS; ++i) {
      const auto saveSlotFile = gamePath + "NUKEM2.-S" + std::to_string(i + 1);
      try {
        result[i] = loadSavedGame(saveSlotFile, nameList[i]);
      } catch (const std::exception&) {
      }
    }
  } catch (const std::exception&) {
  }

  return result;
}


std::array<data::HighScoreList, data::NUM_EPISODES> loadHighScoreLists(
  const std::string& gamePath
) {
  using namespace std::literals;

  std::array<data::HighScoreList, data::NUM_EPISODES> result;

  for (auto i = 0; i < data::NUM_EPISODES; ++i) {
    try {
      result[i] = loadHighScoreList(
        gamePath + "NUKEM2.-V"s + std::to_string(i + 1));
    } catch (const std::exception&) {
    }
  }

  return result;
}


std::optional<GameOptions> loadOptions(const std::string& gamePath) {
  static constexpr std::uint16_t MAX_SCAN_CODE = 88;

  auto asScanCode = [](const std::uint16_t rawScanCode) {
    return static_cast<GameOptions::ScanCode>(
      std::min(rawScanCode, MAX_SCAN_CODE));
  };


  try {
    GameOptions result;

    const auto data = loadFile(gamePath + "NUKEM2.-GT");
    auto reader = LeStreamReader{data};

    result.mUpKeybinding = asScanCode(reader.readU16());
    result.mDownKeybinding = asScanCode(reader.readU16());
    result.mLeftKeybinding = asScanCode(reader.readU16());
    result.mRightKeybinding = asScanCode(reader.readU16());
    result.mJumpKeybinding = asScanCode(reader.readU16());
    result.mFireKeybinding = asScanCode(reader.readU16());

    result.mDifficulty = readDifficulty(reader);

    result.mSoundBlasterSoundsOn = static_cast<bool>(reader.readU16());
    result.mAdlibSoundsOn = static_cast<bool>(reader.readU16());
    result.mPcSpeakersSoundsOn = static_cast<bool>(reader.readU16());
    result.mMusicOn = static_cast<bool>(reader.readU16());

    // Skip over joystick calibration data
    reader.skipBytes(12);

    result.mGameSpeedIndex = static_cast<std::uint8_t>(
      std::min<std::uint16_t>(reader.readU16(), 7));

    return result;
  } catch (const std::exception&) {
  }

  return {};
}

}
