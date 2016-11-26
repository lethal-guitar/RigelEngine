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

#include "resource_loader.hpp"

#include <data/game_traits.hpp>
#include <data/unit_conversions.hpp>
#include <loader/file_utils.hpp>
#include <loader/ega_image_decoder.hpp>
#include <loader/movie_loader.hpp>
#include <loader/music_loader.hpp>
#include <loader/voc_decoder.hpp>
#include <utils/container_tools.hpp>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <map>


namespace rigel { namespace loader {

using namespace data;
using namespace std;


namespace {

const auto FULL_SCREEN_IMAGE_DATA_SIZE =
  (GameTraits::viewPortWidthPx * GameTraits::viewPortHeightPx) /
  (GameTraits::pixelsPerEgaByte / GameTraits::egaPlanes);

}


ResourceLoader::ResourceLoader(const std::string& gamePath)
  : mFilePackage(gamePath + "NUKEM2.CMP")
  , mActorImagePackage(mFilePackage)
  , mGamePath(gamePath)
  , mAdlibSoundsPackage(mFilePackage)
{
}


data::Image ResourceLoader::loadTiledFullscreenImage(
  const std::string& name
) const {
  return loadTiledFullscreenImage(name, INGAME_PALETTE);
}


data::Image ResourceLoader::loadTiledFullscreenImage(
  const std::string& name,
  const Palette16& overridePalette
) const {
  return loadTiledImage(
    mFilePackage.file(name),
    data::GameTraits::viewPortWidthTiles,
    overridePalette,
    false);
}


data::Image ResourceLoader::loadStandaloneFullscreenImage(
  const std::string& name
) const {
  const auto& data = mFilePackage.file(name);
  const auto paletteStart = data.cbegin() + FULL_SCREEN_IMAGE_DATA_SIZE;
  const auto palette = load6bitPalette16(
    paletteStart,
    data.cend());

  auto pixels = decodeSimplePlanarEgaBuffer(
    data.cbegin(),
    data.cbegin() + FULL_SCREEN_IMAGE_DATA_SIZE,
    palette);
  return data::Image(
    std::move(pixels),
    GameTraits::viewPortWidthPx,
    GameTraits::viewPortHeightPx);
}


loader::Palette16 ResourceLoader::loadPaletteFromFullScreenImage(
  const std::string& imageName
) const {
  const auto& data = mFilePackage.file(imageName);
  const auto paletteStart = data.cbegin() + FULL_SCREEN_IMAGE_DATA_SIZE;
  return load6bitPalette16(paletteStart, data.cend());
}


data::map::TileSet ResourceLoader::loadCZone(const std::string& name) const {
  using namespace data;
  using namespace map;

  const auto& data = mFilePackage.file(name);
  LeStreamReader attributeReader(
    data.cbegin(), data.cbegin() + GameTraits::CZone::attributeBytesTotal);

  vector<uint16_t> attributes;
  attributes.reserve(GameTraits::CZone::numTilesTotal);
  for (TileIndex index=0; index<GameTraits::CZone::numTilesTotal; ++index) {
    attributes.push_back(attributeReader.readU16());

    if (index >= GameTraits::CZone::numSolidTiles) {
      attributeReader.skipBytes(sizeof(uint16_t) * 4);
    }
  }

  Image fullImage(
    tilesToPixels(GameTraits::CZone::tileSetImageWidth),
    tilesToPixels(GameTraits::CZone::tileSetImageHeight));

  const auto tilesBegin =
    data.cbegin() + GameTraits::CZone::attributeBytesTotal;
  const auto maskedTilesBegin =
    tilesBegin + GameTraits::CZone::numSolidTiles*GameTraits::CZone::tileBytes;

  const auto solidTilesImage = loadTiledImage(
    tilesBegin,
    maskedTilesBegin,
    GameTraits::CZone::tileSetImageWidth,
    INGAME_PALETTE,
    false);
  const auto maskedTilesImage = loadTiledImage(
    maskedTilesBegin,
    data.cend(),
    GameTraits::CZone::tileSetImageWidth,
    INGAME_PALETTE,
    true);
  fullImage.insertImage(0, 0, solidTilesImage);
  fullImage.insertImage(
    0,
    tilesToPixels(GameTraits::CZone::solidTilesImageHeight),
    maskedTilesImage);

  return {move(fullImage), TileAttributes{move(attributes)}};
}


data::Movie ResourceLoader::loadMovie(const std::string& name) const {
  return loader::loadMovie(loadFile(mGamePath + name));
}


data::AudioBuffer ResourceLoader::loadMusic(const std::string& name) const {
  using namespace chrono;

  const auto before = high_resolution_clock::now();
  const auto buffer = loader::renderImf(mFilePackage.file(name), 44100);
  const auto after = high_resolution_clock::now();

  std::cout
    << "Song rendering time for '" << name << "': "
    << duration_cast<milliseconds>(after - before).count() << " ms\n";

  return buffer;
}


data::AudioBuffer ResourceLoader::loadSound(const data::SoundId id) const {
  static const std::map<data::SoundId, const char*> INTRO_SOUND_MAP{
    {data::SoundId::IntroGunShot, "INTRO3.MNI"},
    {data::SoundId::IntroGunShotLow, "INTRO4.MNI"},
    {data::SoundId::IntroEmptyShellsFalling, "INTRO5.MNI"},
    {data::SoundId::IntroTargetMovingCloser, "INTRO6.MNI"},
    {data::SoundId::IntroTargetStopsMoving, "INTRO7.MNI"},
    {data::SoundId::IntroDukeSpeaks1, "INTRO8.MNI"},
    {data::SoundId::IntroDukeSpeaks2, "INTRO9.MNI"}
  };

  const auto introSoundIter = INTRO_SOUND_MAP.find(id);

  if (introSoundIter != INTRO_SOUND_MAP.end()) {
    return loadSound(introSoundIter->second);
  }

  const auto digitizedSoundFileName =
    string("SB_") + to_string(static_cast<int>(id) + 1) + ".MNI";
  if (mFilePackage.hasFile(digitizedSoundFileName)) {
    return loadSound(digitizedSoundFileName);
  } else {
    return mAdlibSoundsPackage.loadAdlibSound(id);
  }
}


data::AudioBuffer ResourceLoader::loadSound(const std::string& name) const {
  return loader::decodeVoc(mFilePackage.file(name));
}


ScriptBundle ResourceLoader::loadScriptBundle(
  const std::string& fileName
) const {
  const auto data = mFilePackage.file(fileName);
  const auto pBytesAsChars = reinterpret_cast<const char*>(data.data());
  const std::string text(pBytesAsChars, pBytesAsChars + data.size());
  return loader::loadScripts(text);
}

}}

