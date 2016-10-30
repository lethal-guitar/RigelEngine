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

#include <data/audio_buffer.hpp>
#include <data/image.hpp>
#include <data/movie.hpp>
#include <data/sound_ids.hpp>
#include <data/tile_set.hpp>
#include <loader/actor_image_package.hpp>
#include <loader/audio_package.hpp>
#include <loader/duke_script_loader.hpp>
#include <loader/cmp_file_package.hpp>
#include <loader/palette.hpp>

#include <string>


namespace rigel { namespace loader {

class ResourceLoader {
public:
  explicit ResourceLoader(const std::string& gamePath);

  data::Image loadTiledFullscreenImage(const std::string& name) const;
  data::Image loadTiledFullscreenImage(
    const std::string& name,
    const Palette16& overridePalette) const;

  data::Image loadStandaloneFullscreenImage(const std::string& name) const;
  loader::Palette16 loadPaletteFromFullScreenImage(
    const std::string& imageName) const;

  data::map::TileSet loadCZone(const std::string& name) const;
  data::Movie loadMovie(const std::string& name) const;
  data::AudioBuffer loadMusic(const std::string& name) const;

  data::AudioBuffer loadSound(const std::string& name) const;

  data::AudioBuffer loadSound(data::SoundId id) const;

  ScriptBundle loadScriptBundle(const std::string& fileName) const;

  loader::CMPFilePackage mFilePackage;
  loader::ActorImagePackage mActorImagePackage;

private:
  std::string mGamePath;
  loader::AudioPackage mAdlibSoundsPackage;
};

}}
