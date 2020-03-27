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

#include "data/game_options.hpp"
#include "data/high_score_list.hpp"
#include "data/saved_game.hpp"
#include "loader/byte_buffer.hpp"

#include <filesystem>
#include <string>
#include <optional>


namespace rigel {

constexpr auto USER_PROFILE_BASE_NAME = "UserProfile_v2";
constexpr auto USER_PROFILE_FILE_EXTENSION = ".rigel";

/** Store for user specific data
 *
 * The user profile stores data like saved games, high score lists, and game
 * options. It knows how to serialize that data into a file on disk, so that it
 * can persist between game sessions. To load the stored user profile, call
 * loadUserProfile().
 *
 * The public members of this class represent all the data that will be saved
 * in the user profile file. Loading the profile using the aforementioned
 * function will fill these members with data accordingly. You can call
 * saveToDisk() at any time, and it will serialize the state of these members
 * into the file.
 *
 * When changing any of the types used for the public members, or any of the
 * types used within one of those types, you need to adapt the serialization
 * and deserialization code in the implementation of this class!
 */
class UserProfile {
public:
  UserProfile() = default;
  UserProfile(
    const std::filesystem::path& profilePath,
    loader::ByteBuffer originalJson = {});

  void saveToDisk();

  /** Returns true if the profile contains saved games and/or high scores */
  bool hasProgressData() const;

  data::SaveSlotArray mSaveSlots;
  data::HighScoreListArray mHighScoreLists;
  data::GameOptions mOptions;

  std::optional<std::filesystem::path> mGamePath;

private:
  std::optional<std::filesystem::path> mProfilePath;
  loader::ByteBuffer mOriginalJson;
};


UserProfile createEmptyUserProfile();


/** Load existing profile from disk
 *
 * This function looks for an existing user profile file in the location
 * returned by createOrGetPreferencesPath(). If it finds a file, it will load
 * it and return the corresponding UserProfile object.
 * Note that the name of the profile file is an implementation detail of this
 * function, and you normally don't need to care.
 */
std::optional<UserProfile> loadUserProfile();

/** Import original game's profile data
 *
 * Imports saved games, high score lists, and some options from the original
 * Duke Nukem II formats found at the given game path. Overwrites the contents
 * of the passed in profile, so best used on an empty one.
 */
void importOriginalGameProfileData(
  UserProfile& profile, const std::string& gamePath);

/** Return path for storing preferences
 *
 * Returns path to a directory which can be used to store user-specific data
 * and settings. The exact path depends on the platform/operating system, but
 * is guaranteed to have write permissions, and will typically be located
 * somewhere under the user's home directory.
 *
 * The function will create a new directory if it doesn't already exist. If the
 * path cannot be determined due to an error, and empty optional will be
 * returned instead.
 */
std::optional<std::filesystem::path> createOrGetPreferencesPath();

}
