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

#include "user_profile.hpp"

#include "base/warnings.hpp"
#include "loader/file_utils.hpp"
#include "loader/user_profile_import.hpp"

RIGEL_DISABLE_WARNINGS
#include <nlohmann/json.hpp>
#include <SDL_filesystem.h>
RIGEL_RESTORE_WARNINGS

#include <iostream>
#include <filesystem>
#include <fstream>


namespace rigel {

namespace data {

NLOHMANN_JSON_SERIALIZE_ENUM(Difficulty, {
  {Difficulty::Easy, "Easy"},
  {Difficulty::Medium, "Medium"},
  {Difficulty::Hard, "Hard"},
})

NLOHMANN_JSON_SERIALIZE_ENUM(WeaponType, {
  {WeaponType::Normal, "Normal"},
  {WeaponType::Laser, "Laser"},
  {WeaponType::Rocket, "Rocket"},
  {WeaponType::FlameThrower, "FlameThrower"},
})


NLOHMANN_JSON_SERIALIZE_ENUM(TutorialMessageId, {
  {TutorialMessageId::FoundRapidFire, "FoundRapidFire"},
  {TutorialMessageId::FoundHealthMolecule, "FoundHealthMolecule"},
  {TutorialMessageId::FoundRegularWeapon, "FoundRegularWeapon"},
  {TutorialMessageId::FoundLaser, "FoundLaser"},
  {TutorialMessageId::FoundFlameThrower, "FoundFlameThrower"},
  {TutorialMessageId::FoundRocketLauncher, "FoundRocketLauncher"},
  {TutorialMessageId::EarthQuake, "EarthQuake"},
  {TutorialMessageId::FoundBlueKey, "FoundBlueKey"},
  {TutorialMessageId::FoundAccessCard, "FoundAccessCard"},
  {TutorialMessageId::FoundSpaceShip, "FoundSpaceShip"},
  {TutorialMessageId::FoundLetterN, "FoundLetterN"},
  {TutorialMessageId::FoundLetterU, "FoundLetterU"},
  {TutorialMessageId::FoundLetterK, "FoundLetterK"},
  {TutorialMessageId::FoundLetterE, "FoundLetterE"},
  {TutorialMessageId::KeyNeeded, "KeyNeeded"},
  {TutorialMessageId::AccessCardNeeded, "AccessCardNeeded"},
  {TutorialMessageId::CloakNeeded, "CloakNeeded"},
  {TutorialMessageId::RadarsStillFunctional, "RadarsStillFunctional"},
  {TutorialMessageId::HintGlobeNeeded, "HintGlobeNeeded"},
  {TutorialMessageId::FoundTurboLift, "FoundTurboLift"},
  {TutorialMessageId::FoundTeleporter, "FoundTeleporter"},
  {TutorialMessageId::LettersCollectedRightOrder, "LettersCollectedRightOrder"},
  {TutorialMessageId::FoundSoda, "FoundSoda"},
  {TutorialMessageId::FoundForceField, "FoundForceField"},
  {TutorialMessageId::FoundDoor, "FoundDoor"},
})

}


namespace {

constexpr auto PREF_PATH_ORG_NAME = "lethal-guitar";
constexpr auto PREF_PATH_APP_NAME = "Rigel Engine";


data::GameOptions importOptions(const loader::GameOptions& originalOptions) {
  data::GameOptions result;

  result.mSoundOn =
    originalOptions.mSoundBlasterSoundsOn ||
    originalOptions.mAdlibSoundsOn ||
    originalOptions.mPcSpeakersSoundsOn;
  result.mMusicOn = originalOptions.mMusicOn;

  return result;
}


UserProfile loadProfile(const std::string& profileFile) {
  UserProfile profile{profileFile};
  profile.loadFromDisk();
  return profile;
}


UserProfile importProfile(
  const std::string& profileFile,
  const std::string& gamePath
) {
  UserProfile profile{profileFile};

  profile.mSaveSlots = loader::loadSavedGames(gamePath);
  profile.mHighScoreLists = loader::loadHighScoreLists(gamePath);

  if (const auto options = loader::loadOptions(gamePath)) {
    profile.mOptions = importOptions(*options);
  }

  profile.saveToDisk();
  return profile;
}


nlohmann::json serialize(const data::TutorialMessageState& messageState) {
  auto serialized = nlohmann::json::array();

  for (int i = 0; i < data::NUM_TUTORIAL_MESSAGES; ++i) {
    const auto value = static_cast<data::TutorialMessageId>(i);
    if (messageState.hasBeenShown(value)) {
      serialized.push_back(value);
    }
  }

  return serialized;
}


nlohmann::json serialize(const data::SavedGame& savedGame) {
  using json = nlohmann::json;

  json serialized;
  serialized["episode"] = savedGame.mSessionId.mEpisode;
  serialized["level"] = savedGame.mSessionId.mLevel;
  serialized["difficulty"] = savedGame.mSessionId.mDifficulty;
  serialized["tutorialMessagesAlreadySeen"] =
    serialize(savedGame.mTutorialMessagesAlreadySeen);
  serialized["name"] = savedGame.mName;
  serialized["weapon"] = savedGame.mWeapon;
  serialized["ammo"] = savedGame.mAmmo;
  serialized["score"] = savedGame.mScore;
  return serialized;
}


nlohmann::json serialize(const data::HighScoreEntry& entry) {
  using json = nlohmann::json;

  json serialized;
  serialized["name"] = entry.mName;
  serialized["score"] = entry.mScore;
  return serialized;
}


nlohmann::json serialize(const data::GameOptions& options) {
  using json = nlohmann::json;

  json serialized;
  serialized["enableVsync"] = options.mEnableVsync;
  serialized["musicVolume"] = options.mMusicVolume;
  serialized["soundVolume"] = options.mSoundVolume;
  serialized["musicOn"] = options.mMusicOn;
  serialized["soundOn"] = options.mSoundOn;
  return serialized;
}


data::SavedGame deserializeSavedGame(const nlohmann::json& json) {
  using namespace data;

  // TODO: Does it make sense to share the clamping/validation code with the
  // user profile importer?
  data::SavedGame result;
  result.mSessionId.mEpisode = std::clamp(
    json.at("episode").get<int>(), 0, NUM_EPISODES - 1);
  result.mSessionId.mLevel = std::clamp(
    json.at("level").get<int>(), 0, NUM_LEVELS_PER_EPISODE - 1);
  result.mSessionId.mDifficulty = json.at("difficulty").get<data::Difficulty>();

  const auto& messageIds = json.at("tutorialMessagesAlreadySeen");
  for (const auto& messageId : messageIds) {
    result.mTutorialMessagesAlreadySeen.markAsShown(
      messageId.get<data::TutorialMessageId>());
  }

  result.mName = json.at("name").get<std::string>();
  result.mWeapon = json.at("weapon").get<data::WeaponType>();

  const auto maxAmmo = result.mWeapon == WeaponType::FlameThrower
    ? MAX_AMMO_FLAME_THROWER
    : MAX_AMMO;
  result.mAmmo = std::clamp(json.at("ammo").get<int>(), 0, maxAmmo);
  result.mScore = std::clamp(json.at("score").get<int>(), 0, MAX_SCORE);
  return result;
}


data::HighScoreEntry deserializeHighScoreEntry(const nlohmann::json& json) {
  data::HighScoreEntry result;

  result.mName = json.at("name").get<std::string>();
  result.mScore = std::clamp(json.at("score").get<int>(), 0, data::MAX_SCORE);

  return result;
}


template <typename TargetType>
void extractValueIfExists(
  const char* key,
  TargetType& value,
  const nlohmann::json& json
) {
  if (json.contains(key)) {
    try {
      value = json.at(key).get<TargetType>();
    } catch (const std::exception&) {
    }
  }
}


data::GameOptions deserializeGameOptions(const nlohmann::json& json) {
  data::GameOptions result;

  extractValueIfExists("enableVsync", result.mEnableVsync, json);
  extractValueIfExists("musicVolume", result.mMusicVolume, json);
  extractValueIfExists("soundVolume", result.mSoundVolume, json);
  extractValueIfExists("musicOn", result.mMusicOn, json);
  extractValueIfExists("soundOn", result.mSoundOn, json);

  return result;
}


void saveToFile(const loader::ByteBuffer& buffer, const std::string& filename) {
  std::ofstream file(filename, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "WARNING: Failed to store user profile\n";
    return;
  }

  file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
}

}


UserProfile::UserProfile(const std::string& profilePath)
  : mProfilePath(profilePath)
{
}


void UserProfile::saveToDisk() {
  if (!mProfilePath) {
    return;
  }

  using json = nlohmann::json;

  json serializedProfile;

  // TODO: Refactor this long function into sub-functions

  auto serializedSaveSlots = json::array();
  for (const auto& slot : mSaveSlots) {
    if (slot) {
      serializedSaveSlots.push_back(serialize(*slot));
    } else {
      serializedSaveSlots.push_back(nullptr);
    }
  }

  serializedProfile["saveSlots"] = serializedSaveSlots;

  auto serializedHighScoreLists = json::array();
  for (const auto& list : mHighScoreLists) {
    auto serializedList = json::array();
    for (const auto& entry : list) {
      serializedList.push_back(serialize(entry));
    }

    serializedHighScoreLists.push_back(serializedList);
  }

  serializedProfile["highScoreLists"] = serializedHighScoreLists;

  serializedProfile["options"] = serialize(mOptions);

  const auto buffer = json::to_msgpack(serializedProfile);
  saveToFile(buffer, *mProfilePath);
}


void UserProfile::loadFromDisk() {
  using std::begin;
  using std::end;
  using std::sort;

  if (!mProfilePath) {
    return;
  }

  mSaveSlots.fill(std::nullopt);
  mHighScoreLists.fill(data::HighScoreList{});

  try {
    const auto buffer = loader::loadFile(*mProfilePath);
    const auto serializedProfile = nlohmann::json::from_msgpack(buffer);

    const auto serializedSaveSlots = serializedProfile.at("saveSlots");

    // TODO: Refactor this long function into sub-functions
    {
      std::size_t i = 0;
      for (const auto& serializedSlot : serializedSaveSlots) {
        if (!serializedSlot.is_null()) {
          mSaveSlots[i] = deserializeSavedGame(serializedSlot);
        }
        ++i;
        if (i >= mSaveSlots.size()) {
          break;
        }
      }
    }

    const auto serializedHighScoreLists =
      serializedProfile.at("highScoreLists");

    {
      std::size_t i = 0;
      for (const auto& serializedList : serializedHighScoreLists) {
        {
          std::size_t j = 0;
          for (const auto& serializedEntry : serializedList) {
            mHighScoreLists[i][j] = deserializeHighScoreEntry(serializedEntry);

            ++j;
            if (j >= data::NUM_HIGH_SCORE_ENTRIES) {
              break;
            }
          }
        }

        sort(begin(mHighScoreLists[i]), end(mHighScoreLists[i]));

        ++i;
        if (i >= mHighScoreLists.size()) {
          break;
        }
      }
    }

    if (serializedProfile.contains("options")) {
      mOptions = deserializeGameOptions(serializedProfile.at("options"));
    }
  } catch (const std::exception& ex) {
    std::cerr << "WARNING: Failed to load user profile\n";
    std::cerr << ex.what() << '\n';
  }
}


UserProfile loadOrCreateUserProfile(const std::string& gamePath) {
  namespace fs = std::filesystem;

  auto deleter = [](char* path) { SDL_free(path); };
  const auto pPreferencesDirName = std::unique_ptr<char, decltype(deleter)>{
    SDL_GetPrefPath(PREF_PATH_ORG_NAME, PREF_PATH_APP_NAME), deleter};

  if (!pPreferencesDirName) {
    std::cerr << "WARNING: Cannot open user preferences directory\n";
    return {};
  }

  const auto preferencesDirName = std::string{pPreferencesDirName.get()};

  const auto profileFilePath =
    fs::u8path(preferencesDirName) / "UserProfile.rigel";

  if (fs::exists(profileFilePath)) {
    return loadProfile(profileFilePath.u8string());
  } else {
    return importProfile(profileFilePath.u8string(), gamePath);
  }
}

}
