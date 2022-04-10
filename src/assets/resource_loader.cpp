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

#include "assets/ega_image_decoder.hpp"
#include "assets/file_utils.hpp"
#include "assets/movie_loader.hpp"
#include "assets/music_loader.hpp"
#include "assets/png_image.hpp"
#include "assets/voc_decoder.hpp"
#include "base/container_utils.hpp"
#include "data/game_traits.hpp"
#include "data/unit_conversions.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <regex>

namespace fs = std::filesystem;


namespace rigel::assets
{

using namespace data;


namespace
{

const auto ANTI_PIRACY_SCREEN_FILENAME = "LCR.MNI";

const auto FULL_SCREEN_IMAGE_DATA_SIZE =
  (GameTraits::viewportWidthPx * GameTraits::viewportHeightPx) /
  (GameTraits::pixelsPerEgaByte / GameTraits::egaPlanes);


// When loading assets, the game will first check if a file with an expected
// name exists at the replacements path, and if it does, it will load this file
// and use it instead of the asset from the original data file (NUKEM2.CMP).
//
// At the moment, this is implemented for sprites/actors, backdrops, and
// tilesets. The expected format for replacement files is:
//
//   backdrop<num>.png
//
//   tileset<num>.png
//
//   actor<actor_id>_frame<animation_frame>.png
//
// Where <num>, <actor_id> and <animation_frame> should be replaced with the
// corresponding numbers. For example, to replace the images used for the
// "blue guard" enemy, files named "actor159_frame0.png" up to
// "actor159_frame12.png" should be provided.
//
// For tilesets and backdrops, <num> should be the same number as in the
// original asset filename. E.g. to replace CZONE1.MNI, provide a file named
// tileset1.png, etc.
//
// The files can contain full 32-bit RGBA values, there are no limitations.
const auto ASSET_REPLACEMENTS_PATH = "asset_replacements";


std::string replacementSpriteImageName(const int id, const int frame)
{
  return "actor" + std::to_string(id) + "_frame" + std::to_string(frame) +
    ".png";
}


std::optional<data::Image> loadReplacementTilesetIfPresent(
  const fs::path& resourcePath,
  std::string_view name)
{
  using namespace std::literals;

  std::regex tilesetNameRegex{"^CZONE([0-9A-Z])\\.MNI$", std::regex::icase};
  std::match_results<std::string_view::const_iterator> matches;

  if (
    !std::regex_match(name.begin(), name.end(), matches, tilesetNameRegex) ||
    matches.size() != 2)
  {
    return {};
  }

  const auto number = matches[1].str();
  const auto replacementName = "tileset"s + number + ".png";
  const auto replacementPath = resourcePath / replacementName;

  return loadPng(replacementPath.u8string());
}


int asSoundIndex(const data::SoundId id)
{
  return static_cast<int>(id) + 1;
}


int asIntroSoundIndex(const data::SoundId id)
{
  return static_cast<int>(id) - static_cast<int>(data::SoundId::IntroGunShot) +
    3;
}


std::string digitizedSoundFilenameForId(const data::SoundId soundId)
{
  using namespace std::string_literals;

  if (data::isIntroSound(soundId))
  {
    return "INTRO"s + std::to_string(asIntroSoundIndex(soundId)) + ".MNI";
  }

  return "SB_"s + std::to_string(asSoundIndex(soundId)) + ".MNI";
}

} // namespace


ResourceLoader::ResourceLoader(
  std::filesystem::path gamePath,
  bool enableTopLevelMods,
  std::vector<fs::path> modPaths)
  : mGamePath(std::move(gamePath))
  , mModPaths(std::move(modPaths))
  , mEnableTopLevelMods(enableTopLevelMods)
  , mFilePackage((mGamePath / "NUKEM2.CMP").u8string())
  , mActorImagePackage(
      file(ActorImagePackage::IMAGE_DATA_FILE),
      file(ActorImagePackage::ACTOR_INFO_FILE))
{
}


template <typename TryLoadFunc, typename T>
std::optional<T> ResourceLoader::tryLoadReplacement(TryLoadFunc&& tryLoad) const
{
  for (auto iPath = mModPaths.rbegin(); iPath != mModPaths.rend(); ++iPath)
  {
    if (auto oReplacement = tryLoad(*iPath))
    {
      return *oReplacement;
    }
  }

  if (mEnableTopLevelMods)
  {
    if (auto oReplacement = tryLoad(mGamePath / ASSET_REPLACEMENTS_PATH))
    {
      return *oReplacement;
    }
  }

  return {};
}


std::optional<data::Image>
  ResourceLoader::tryLoadPngReplacement(std::string_view filename) const
{
  return tryLoadReplacement([filename](const fs::path& path) {
    return loadPng((path / filename).u8string());
  });
}


data::Image ResourceLoader::loadUiSpriteSheet() const
{
  const auto oReplacement = tryLoadPngReplacement("status.png");
  return oReplacement ? *oReplacement
                      : loadUiSpriteSheet(data::GameTraits::INGAME_PALETTE);
}


data::Image ResourceLoader::loadUiSpriteSheet(
  const data::Palette16& overridePalette) const
{
  return loadTiledFullscreenImage("STATUS.MNI", overridePalette);
}


data::Image
  ResourceLoader::loadTiledFullscreenImage(std::string_view name) const
{
  return loadTiledFullscreenImage(name, data::GameTraits::INGAME_PALETTE);
}


data::Image ResourceLoader::loadTiledFullscreenImage(
  std::string_view name,
  const data::Palette16& overridePalette) const
{
  return loadTiledImage(
    file(name),
    data::GameTraits::viewportWidthTiles,
    overridePalette,
    data::TileImageType::Unmasked);
}


data::Image
  ResourceLoader::loadStandaloneFullscreenImage(std::string_view name) const
{
  const auto& data = file(name);
  const auto paletteStart = data.begin() + FULL_SCREEN_IMAGE_DATA_SIZE;
  const auto palette = load6bitPalette16(paletteStart, data.end());

  auto pixels = decodeSimplePlanarEgaBuffer(
    data.begin(), data.begin() + FULL_SCREEN_IMAGE_DATA_SIZE, palette);
  return data::Image(
    std::move(pixels),
    GameTraits::viewportWidthPx,
    GameTraits::viewportHeightPx);
}


data::Image ResourceLoader::loadAntiPiracyImage() const
{
  using namespace std;

  // For some reason, the anti-piracy screen is in a different format than all
  // the other full-screen images. It first defines a 256-color VGA palette,
  // then defines the pixel data in linear format.
  //
  // See http://www.shikadi.net/moddingwiki/Duke_Nukem_II_Full-screen_Images
  const auto& data = file(ANTI_PIRACY_SCREEN_FILENAME);
  const auto iImageStart = begin(data) + 256 * 3;
  const auto palette = load6bitPalette256(begin(data), iImageStart);

  data::PixelBuffer pixels;
  pixels.reserve(GameTraits::viewportWidthPx * GameTraits::viewportHeightPx);
  transform(
    iImageStart,
    end(data),
    back_inserter(pixels),
    [&palette](const auto indexedPixel) { return palette[indexedPixel]; });
  return data::Image(
    move(pixels), GameTraits::viewportWidthPx, GameTraits::viewportHeightPx);
}


data::Palette16 ResourceLoader::loadPaletteFromFullScreenImage(
  std::string_view imageName) const
{
  const auto& data = file(imageName);
  const auto paletteStart = data.begin() + FULL_SCREEN_IMAGE_DATA_SIZE;
  return load6bitPalette16(paletteStart, data.end());
}


ActorData ResourceLoader::loadActor(
  data::ActorID id,
  const data::Palette16& palette) const
{
  const auto& actorInfo = mActorImagePackage.loadActorInfo(id);

  auto images = utils::transformed(
    actorInfo.mFrames, [&, frame = 0](const auto& frameHeader) mutable {
      const auto imageName =
        replacementSpriteImageName(static_cast<int>(id), frame);
      ++frame;

      const auto oReplacement = tryLoadPngReplacement(imageName);
      return ActorData::Frame{
        frameHeader.mDrawOffset,
        frameHeader.mSizeInTiles,
        oReplacement ? *oReplacement
                     : mActorImagePackage.loadImage(frameHeader, palette)};
    });

  return ActorData{actorInfo.mDrawIndex, std::move(images)};
}


data::Image ResourceLoader::loadBackdrop(std::string_view name) const
{
  using namespace std::literals;

  std::regex backdropNameRegex{"^DROP([0-9]+)\\.MNI$", std::regex::icase};
  std::match_results<std::string_view::const_iterator> matches;

  if (
    std::regex_match(name.begin(), name.end(), matches, backdropNameRegex) &&
    matches.size() == 2)
  {
    const auto number = matches[1].str();
    const auto replacementName = "backdrop"s + number + ".png";

    if (const auto oReplacement = tryLoadPngReplacement(replacementName))
    {
      return *oReplacement;
    }
  }

  return loadTiledFullscreenImage(name);
}


TileSet ResourceLoader::loadCZone(std::string_view name) const
{
  using namespace data;
  using namespace map;
  using T = data::TileImageType;

  const auto& data = file(name);
  LeStreamReader attributeReader(
    data.begin(), data.begin() + GameTraits::CZone::attributeBytesTotal);

  vector<uint16_t> attributes;
  attributes.reserve(GameTraits::CZone::numTilesTotal);
  for (TileIndex index = 0; index < GameTraits::CZone::numTilesTotal; ++index)
  {
    attributes.push_back(attributeReader.readU16());

    if (index >= GameTraits::CZone::numSolidTiles)
    {
      attributeReader.skipBytes(sizeof(uint16_t) * 4);
    }
  }

  const auto oReplacementImage =
    tryLoadReplacement([name](const fs::path& path) {
      return loadReplacementTilesetIfPresent(path, name);
    });

  if (oReplacementImage)
  {
    return {
      std::move(*oReplacementImage), TileAttributeDict{std::move(attributes)}};
  }

  Image fullImage(
    tilesToPixels(GameTraits::CZone::tileSetImageWidth),
    tilesToPixels(GameTraits::CZone::tileSetImageHeight));

  const auto tilesBegin = data.begin() + GameTraits::CZone::attributeBytesTotal;
  const auto maskedTilesBegin = tilesBegin +
    GameTraits::CZone::numSolidTiles * GameTraits::CZone::tileBytes;

  const auto solidTilesImage = loadTiledImage(
    tilesBegin,
    maskedTilesBegin,
    GameTraits::CZone::tileSetImageWidth,
    data::GameTraits::INGAME_PALETTE,
    T::Unmasked);
  const auto maskedTilesImage = loadTiledImage(
    maskedTilesBegin,
    data.end(),
    GameTraits::CZone::tileSetImageWidth,
    data::GameTraits::INGAME_PALETTE,
    T::Masked);
  fullImage.insertImage(0, 0, solidTilesImage);
  fullImage.insertImage(
    0,
    tilesToPixels(GameTraits::CZone::solidTilesImageHeight),
    maskedTilesImage);

  return {std::move(fullImage), TileAttributeDict{std::move(attributes)}};
}


data::Movie ResourceLoader::loadMovie(std::string_view name) const
{
  // We don't use tryLoadReplacement here, because we don't look for movies
  // in the top-level path.
  for (auto iPath = mModPaths.rbegin(); iPath != mModPaths.rend(); ++iPath)
  {
    const auto moddedFile = *iPath / fs::u8path(name);
    if (fs::exists(moddedFile))
    {
      return assets::loadMovie(loadFile(moddedFile));
    }
  }

  return assets::loadMovie(loadFile(mGamePath / fs::u8path(name)));
}


data::Song ResourceLoader::loadMusic(std::string_view name) const
{
  return assets::loadSong(file(name));
}


bool ResourceLoader::hasSoundBlasterSound(const data::SoundId id) const
{
  return hasFile(digitizedSoundFilenameForId(id));
}


data::AudioBuffer
  ResourceLoader::loadSoundBlasterSound(const data::SoundId id) const
{
  const auto digitizedSoundFileName = digitizedSoundFilenameForId(id);
  if (hasFile(digitizedSoundFileName))
  {
    return loadSound(digitizedSoundFileName);
  }

  return {};
}


std::vector<std::filesystem::path>
  ResourceLoader::replacementSoundPaths(data::SoundId id) const
{
  using namespace std::literals;

  const auto expectedName =
    "sound"s + std::to_string(static_cast<int>(id) + 1) + ".wav";

  auto result = std::vector<std::filesystem::path>{};
  result.reserve(mModPaths.size());

  std::transform(
    mModPaths.rbegin(),
    mModPaths.rend(),
    std::back_inserter(result),
    [&](const auto& path) { return path / expectedName; });

  if (mEnableTopLevelMods)
  {
    result.push_back(mGamePath / ASSET_REPLACEMENTS_PATH / expectedName);
  }

  return result;
}


std::vector<std::filesystem::path>
  ResourceLoader::replacementMusicBasePaths() const
{
  auto result =
    std::vector<std::filesystem::path>{mModPaths.rbegin(), mModPaths.rend()};

  if (mEnableTopLevelMods)
  {
    result.push_back(mGamePath / ASSET_REPLACEMENTS_PATH);
  }
  return result;
}


data::AudioBuffer ResourceLoader::loadSound(std::string_view name) const
{
  return assets::decodeVoc(file(name));
}


ScriptBundle ResourceLoader::loadScriptBundle(std::string_view fileName) const
{
  return assets::loadScripts(fileAsText(fileName));
}


ByteBuffer ResourceLoader::file(std::string_view name) const
{
  // TODO: Eliminate duplication with tryLoadReplacement?
  for (auto iPath = mModPaths.rbegin(); iPath != mModPaths.rend(); ++iPath)
  {
    const auto unpackedFilePath = *iPath / fs::u8path(name);
    if (fs::exists(unpackedFilePath))
    {
      return loadFile(unpackedFilePath);
    }
  }

  if (mEnableTopLevelMods)
  {
    const auto unpackedFilePath = mGamePath / fs::u8path(name);
    if (fs::exists(unpackedFilePath))
    {
      return loadFile(unpackedFilePath);
    }
  }

  return mFilePackage.file(name);
}


std::string ResourceLoader::fileAsText(std::string_view name) const
{
  return asText(file(name));
}


bool ResourceLoader::hasFile(std::string_view name) const
{
  // TODO: Eliminate duplication with tryLoadReplacement?
  for (auto iPath = mModPaths.rbegin(); iPath != mModPaths.rend(); ++iPath)
  {
    const auto unpackedFilePath = *iPath / fs::u8path(name);
    if (fs::exists(unpackedFilePath))
    {
      return true;
    }
  }

  if (mEnableTopLevelMods)
  {
    const auto unpackedFilePath = mGamePath / fs::u8path(name);
    if (fs::exists(unpackedFilePath))
    {
      return true;
    }
  }

  return mFilePackage.hasFile(name);
}

} // namespace rigel::assets
