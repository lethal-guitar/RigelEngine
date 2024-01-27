/* Copyright (C) 2022, Nikolai Wuttke. All rights reserved.
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

#include <filesystem>
#include <string>
#include <tuple>
#include <vector>

struct Filesystem
{
  using IsDirectoryFuncT =
    bool (*)(const std::filesystem::path& p, std::error_code& ec);
  using ExistsFuncT =
    bool (*)(const std::filesystem::path& p, std::error_code& ec) noexcept;
  using DirectoryIterateCallbackFuncT =
    bool (*)(const std::filesystem::path& p, void* user_data) noexcept;
  using DirectoryIterateFuncT = void (*)(
    const std::filesystem::path& p,
    std::filesystem::directory_options options,
    std::error_code& ec,
    DirectoryIterateCallbackFuncT callback,
    void* user_data) noexcept;

  IsDirectoryFuncT f_is_directory;
  ExistsFuncT f_exists;
  DirectoryIterateFuncT f_directory_iterate;
};

extern const Filesystem gNativeFilesystemHandle;

namespace rigel::data
{

constexpr auto MODS_PATH = "mods";


struct ModStatus
{
  // Refers to mAvailableMods
  int mIndex;
  bool mIsEnabled;

  friend bool operator==(const ModStatus& lhs, const ModStatus& rhs)
  {
    return std::tie(lhs.mIndex, lhs.mIsEnabled) ==
      std::tie(rhs.mIndex, rhs.mIsEnabled);
  }

  friend bool operator!=(const ModStatus& lhs, const ModStatus& rhs)
  {
    return !(lhs == rhs);
  }
};


class ModLibrary
{
public:
  ModLibrary() = default;
  ModLibrary(
    std::filesystem::path gamePath,
    std::vector<std::string> availableMods,
    std::vector<ModStatus> initialSelection);

  void updateGamePath(std::filesystem::path gamePath);
  void rescan(const Filesystem& fsHandle = gNativeFilesystemHandle);

  [[nodiscard]] std::vector<std::filesystem::path> enabledModPaths() const;
  [[nodiscard]] const std::string& modDirName(int index) const;

  [[nodiscard]] const std::vector<std::string>& currentlyAvailableMods() const;
  [[nodiscard]] const std::vector<ModStatus>& currentSelection() const;
  void replaceSelection(std::vector<ModStatus> newSelection);

  [[nodiscard]] bool fetchAndClearSelectionChangedFlag();
  void clearSelectionChangedFlag();

private:
  std::vector<std::string> mAvailableMods;
  std::vector<ModStatus> mModSelection;
  std::filesystem::path mGamePath;
  bool mHasChanged = false;
};

} // namespace rigel::data
