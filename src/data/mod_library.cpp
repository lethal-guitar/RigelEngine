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

#include "mod_library.hpp"

#include "base/container_utils.hpp"

#include <loguru.hpp>

#include <algorithm>
#include <cassert>
#include <limits>
#include <unordered_map>
#include <utility>


static void nativeFilesystemIterate(
  const std::filesystem::path& p,
  std::filesystem::directory_options options,
  std::error_code& ec,
  Filesystem::DirectoryIterateCallbackFuncT callback,
  void* userData) noexcept
{
  namespace fs = std::filesystem;
  auto dir_it = fs::directory_iterator(p, options, ec);
  if (ec)
  {
    return;
  }

  for (const auto& dir_entry : dir_it)
  {
    if (!callback(dir_entry.path(), userData))
    {
      return;
    }
  }
}

const Filesystem gNativeFilesystemHandle{
  &std::filesystem::is_directory,
  &std::filesystem::exists,
  &nativeFilesystemIterate};

namespace rigel::data
{

namespace
{

bool isNonEmptyDirectory(
  const std::filesystem::path& entry,
  const Filesystem& fsHandle)
{
  namespace fs = std::filesystem;
  std::error_code err;
  if (!fsHandle.f_is_directory(entry, err) || err)
  {
    return false;
  }

  if (!fsHandle.f_exists(entry, err) || err)
  {
    return false;
  }

  bool foundOne = false;
  fsHandle.f_directory_iterate(
    entry,
    fs::directory_options::none,
    err,
    [](const std::filesystem::path& p, void* userData) noexcept -> bool
    {
      *static_cast<bool*>(userData) = true;
      // once we've found one, we can just return, it's surely non empty
      return false;
    },
    &foundOne);

  return !err && !foundOne;
}

} // namespace


ModLibrary::ModLibrary(
  std::filesystem::path gamePath,
  std::vector<std::string> availableMods,
  std::vector<ModStatus> initialSelection)
  : mAvailableMods(std::move(availableMods))
  , mModSelection(std::move(initialSelection))
  , mGamePath(std::move(gamePath))
{
  assert(availableMods.size() == initialSelection.size());
}


void ModLibrary::updateGamePath(std::filesystem::path gamePath)
{
  const auto pathHasChanged = gamePath != mGamePath;
  mGamePath = std::move(gamePath);

  if (pathHasChanged)
  {
    rescan();
  }
}


void ModLibrary::rescan(const Filesystem& fsHandle)
{
  namespace fs = std::filesystem;

  LOG_SCOPE_FUNCTION(INFO);

  // List all sub-directories of the "mods" directory. Each one is considered
  // a mod.
  LOG_F(INFO, "Listing mod directories");
  auto newAvailableMods = std::vector<std::string>{};

  std::error_code err;
  struct Context
  {
    std::vector<std::string>* pNewAvailableMods = nullptr;
    const Filesystem* pFsHandle = nullptr;
  } ctx{&newAvailableMods, &fsHandle};

  fsHandle.f_directory_iterate(
    mGamePath / MODS_PATH,
    fs::directory_options::skip_permission_denied,
    err,
    [](const std::filesystem::path& p, void* userData) noexcept -> bool
    {
      auto ctx =
        static_cast<Context*>(userData);
      if (isNonEmptyDirectory(p, *(ctx->pFsHandle)))
      {
        ctx->pNewAvailableMods->push_back(p.filename().u8string());
      }
      return true;
    },
    &ctx);

  LOG_F(INFO, "Found %d mods", int(newAvailableMods.size()));

  // No prior selection - create default selection and early out
  if (mModSelection.empty())
  {
    LOG_F(INFO, "No previous mod library, creating default selection");
    mModSelection.reserve(newAvailableMods.size());

    for (auto i = 0; i < int(newAvailableMods.size()); ++i)
    {
      mModSelection.push_back({i, false});
    }

    mAvailableMods = std::move(newAvailableMods);
    return;
  }

  // We have a prior selection, we need to consolidate it with any new/deleted
  // entries.
  LOG_F(INFO, "Updating library");

  // First, transform our data into a form that's easier to work with.
  struct ModConfig
  {
    std::string mDirName;
    int mDesiredIndex;
    int mIndexInNewList;
    bool mIsEnabled;
  };

  std::unordered_map<std::string, ModConfig> previousConfiguration;

  {
    auto index = 0;
    for (const auto& status : mModSelection)
    {
      const auto& dirName = mAvailableMods[status.mIndex];
      previousConfiguration.insert(
        {dirName, {dirName, index, -1, status.mIsEnabled}});
      ++index;
    }
  }

  // Now build up the new configuration based on the new mods found.
  // Any mods that exist in previousConfiguration, but not in newAvailableMods
  // will be droppped since we don't add them to newConfiguration.
  std::vector<ModConfig> newConfiguration;
  newConfiguration.reserve(newAvailableMods.size());

  constexpr auto PLACEHOLDER = std::numeric_limits<int>::max();

  {
    auto index = 0;
    for (const auto& mod : newAvailableMods)
    {
      if (auto iEntry = previousConfiguration.find(mod);
          iEntry != previousConfiguration.end())
      {
        auto entry = iEntry->second;
        entry.mIndexInNewList = index;

        // If we already have an entry for this mod, copy it
        newConfiguration.push_back(entry);
      }
      else
      {
        // Otherwise, put the newly added mod at the end, and disable it
        // initially
        newConfiguration.push_back({mod, PLACEHOLDER, index, false});
      }

      ++index;
    }
  }

  // Sort by desired index to restore the sort order defined by our previous
  // configuration
  std::sort(
    newConfiguration.begin(),
    newConfiguration.end(),
    [](const auto& lhs, const auto& rhs) {
      return lhs.mDesiredIndex < rhs.mDesiredIndex;
    });

  // Assign correct indices for newly added mods (they still have the
  // placeholder for sorting, which is int max)
  auto iFirstIncorrectIndex = std::lower_bound(
    newConfiguration.begin(),
    newConfiguration.end(),
    PLACEHOLDER,
    [](const ModConfig& config, const int value) {
      return config.mDesiredIndex < value;
    });

  if (iFirstIncorrectIndex != newConfiguration.end())
  {
    auto indexToAssign = iFirstIncorrectIndex != newConfiguration.begin()
      ? std::prev(iFirstIncorrectIndex)->mDesiredIndex + 1
      : 0;

    std::for_each(
      iFirstIncorrectIndex,
      newConfiguration.end(),
      [&indexToAssign](ModConfig& config) {
        config.mDesiredIndex = indexToAssign;
        ++indexToAssign;
      });
  }

  // Now, we can transform newConfiguration back into the original form.
  const auto previousEnabledPaths = enabledModPaths();

  mModSelection = utils::transformed(newConfiguration, [](const auto& config) {
    return ModStatus{config.mIndexInNewList, config.mIsEnabled};
  });
  mAvailableMods = std::move(newAvailableMods);

  mHasChanged |= previousEnabledPaths != enabledModPaths();
}


std::vector<std::filesystem::path> ModLibrary::enabledModPaths() const
{
  namespace fs = std::filesystem;

  // TODO: Maybe express this in a nicer way using ranges?
  // It's essentially a filter -> transform
  std::vector<fs::path> enabledMods;

  for (const auto& mod : mModSelection)
  {
    if (mod.mIsEnabled)
    {
      enabledMods.push_back(
        mGamePath / MODS_PATH / fs::u8path(mAvailableMods[mod.mIndex]));
    }
  }

  return enabledMods;
}


const std::string& ModLibrary::modDirName(const int index) const
{
  return mAvailableMods[index];
}


const std::vector<std::string>& ModLibrary::currentlyAvailableMods() const
{
  return mAvailableMods;
}


const std::vector<ModStatus>& ModLibrary::currentSelection() const
{
  return mModSelection;
}


void ModLibrary::replaceSelection(std::vector<ModStatus> newSelection)
{
  mHasChanged |= mModSelection != newSelection;
  mModSelection = std::move(newSelection);
}


bool ModLibrary::fetchAndClearSelectionChangedFlag()
{
  return std::exchange(mHasChanged, false);
}


void ModLibrary::clearSelectionChangedFlag()
{
  mHasChanged = false;
}

} // namespace rigel::data
