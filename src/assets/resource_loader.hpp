/* Copyright (C) 2016, Nikolai Wuttke. All rights reserved.
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

#include "assets/actor_image_package.hpp"
#include "assets/cmp_file_package.hpp"
#include "assets/duke_script_loader.hpp"
#include "assets/palette.hpp"
#include "base/array_view.hpp"
#include "data/audio_buffer.hpp"
#include "data/image.hpp"
#include "data/movie.hpp"
#include "data/song.hpp"
#include "data/sound_ids.hpp"
#include "data/tile_attributes.hpp"

#include <filesystem>
#include <string>
#include <vector>


namespace rigel::assets
{

constexpr auto ULTRAWIDE_HUD_WIDTH = 560;
constexpr auto ULTRAWIDE_HUD_HEIGHT = 70;
constexpr auto ULTRAWIDE_HUD_INNER_WIDTH = 424;

struct TileSet
{
  data::Image mTiles;
  data::map::TileAttributeDict mAttributes;
};


struct ActorData
{
  struct Frame
  {
    base::Vec2 mDrawOffset;
    base::Extents mLogicalSize;
    data::Image mFrameImage;
  };

  int mDrawIndex;
  std::vector<Frame> mFrames;
};


class ResourceLoader
{
public:
  ResourceLoader(
    std::filesystem::path gamePath,
    bool enableTopLevelMods,
    std::vector<std::filesystem::path> modPaths);

  data::Image loadUiSpriteSheet() const;
  data::Image loadUiSpriteSheet(const data::Palette16& overridePalette) const;

  data::Image loadStandaloneFullscreenImage(std::string_view name) const;
  data::Palette16
    loadPaletteFromFullScreenImage(std::string_view imageName) const;

  ActorData loadActor(
    data::ActorID id,
    const data::Palette16& palette = data::GameTraits::INGAME_PALETTE) const;

  FontData loadFont() const { return mActorImagePackage.loadFont(); }

  int drawIndexFor(data::ActorID id) const
  {
    return mActorImagePackage.drawIndexFor(id);
  }

  data::Image loadAntiPiracyImage() const;

  data::Image loadWideHudFrameImage() const;
  data::Image loadUltrawideHudFrameImage() const;

  data::Image loadBackdrop(std::string_view name) const;
  TileSet loadCZone(std::string_view name) const;
  data::Movie loadMovie(std::string_view name) const;

  data::Song loadMusic(std::string_view name) const;
  bool hasSoundBlasterSound(data::SoundId id) const;
  data::AudioBuffer loadSoundBlasterSound(data::SoundId id) const;

  std::vector<std::filesystem::path>
    replacementSoundPaths(data::SoundId id) const;
  std::vector<std::filesystem::path> replacementMusicBasePaths() const;

  ScriptBundle loadScriptBundle(std::string_view fileName) const;

  ByteBuffer file(std::string_view name) const;
  std::string fileAsText(std::string_view name) const;
  bool hasFile(std::string_view name) const;

private:
  // The invoke_result of the TryLoadFunc is going to be a std::optional<T>,
  // hence we need to unpack the underlying T via the optional's value_type
  template <
    typename TryLoadFunc,
    typename T = typename std::
      invoke_result_t<TryLoadFunc, const std::filesystem::path&>::value_type>
  std::optional<T> tryLoadReplacement(TryLoadFunc&& tryLoad) const;
  std::optional<data::Image>
    tryLoadPngReplacement(std::string_view filename) const;

  data::Image loadEmbeddedImageAsset(
    const char* replacementName,
    base::ArrayView<std::uint8_t> data) const;

  data::Image loadTiledFullscreenImage(std::string_view name) const;
  data::Image loadTiledFullscreenImage(
    std::string_view name,
    const data::Palette16& overridePalette) const;
  data::AudioBuffer loadSound(std::string_view name) const;

  std::filesystem::path mGamePath;
  std::vector<std::filesystem::path> mModPaths;
  bool mEnableTopLevelMods;

  assets::CMPFilePackage mFilePackage;
  assets::ActorImagePackage mActorImagePackage;
};

} // namespace rigel::assets
