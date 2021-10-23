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

#include "data/audio_buffer.hpp"
#include "data/image.hpp"
#include "data/movie.hpp"
#include "data/song.hpp"
#include "data/sound_ids.hpp"
#include "data/tile_attributes.hpp"
#include "loader/actor_image_package.hpp"
#include "loader/audio_package.hpp"
#include "loader/cmp_file_package.hpp"
#include "loader/duke_script_loader.hpp"
#include "loader/palette.hpp"

#include <filesystem>
#include <string>


namespace rigel::loader
{

struct TileSet
{
  data::Image mTiles;
  data::map::TileAttributeDict mAttributes;
};


class ResourceLoader
{
public:
  explicit ResourceLoader(const std::string& gamePath);

  data::Image loadTiledFullscreenImage(std::string_view name) const;
  data::Image loadTiledFullscreenImage(
    std::string_view name,
    const Palette16& overridePalette) const;

  data::Image loadStandaloneFullscreenImage(std::string_view name) const;
  loader::Palette16
    loadPaletteFromFullScreenImage(std::string_view imageName) const;

  data::Image loadAntiPiracyImage() const;

  data::Image loadBackdrop(std::string_view name) const;
  TileSet loadCZone(std::string_view name) const;
  data::Movie loadMovie(std::string_view name) const;

  data::Song loadMusic(std::string_view name) const;
  bool hasSoundBlasterSound(data::SoundId id) const;
  data::AudioBuffer loadAdlibSound(data::SoundId id) const;
  data::AudioBuffer loadPreferredSound(data::SoundId id) const;
  std::filesystem::path replacementSoundPath(data::SoundId id) const;
  std::filesystem::path replacementMusicBasePath() const;

  ScriptBundle loadScriptBundle(std::string_view fileName) const;

  ByteBuffer file(std::string_view name) const;
  std::string fileAsText(std::string_view name) const;
  bool hasFile(std::string_view name) const;

private:
  data::AudioBuffer loadSound(std::string_view name) const;

  std::filesystem::path mGamePath;
  loader::CMPFilePackage mFilePackage;

public:
  loader::ActorImagePackage mActorImagePackage;

private:
  loader::AudioPackage mAdlibSoundsPackage;
};

} // namespace rigel::loader
