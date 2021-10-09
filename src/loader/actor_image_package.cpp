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

#include "actor_image_package.hpp"

#include "base/container_utils.hpp"
#include "base/string_utils.hpp"
#include "data/game_traits.hpp"
#include "data/unit_conversions.hpp"
#include "loader/ega_image_decoder.hpp"
#include "loader/file_utils.hpp"
#include "loader/png_image.hpp"

#include <cassert>
#include <fstream>
#include <utility>


namespace rigel::loader
{

using namespace std;
using data::ActorID;
using data::GameTraits;


namespace
{


std::string replacementImagePath(
  const std::string& basePath,
  const int id,
  const int frame)
{
  return basePath + "/actor" + std::to_string(id) + "_frame" +
    std::to_string(frame) + ".png";
}

} // namespace


ActorImagePackage::ActorImagePackage(
  ByteBuffer imageData,
  const ByteBuffer& actorInfoData,
  std::optional<std::string> maybeImageReplacementsPath)
  : mImageData(std::move(imageData))
  , mMaybeReplacementsPath(std::move(maybeImageReplacementsPath))
{
  LeStreamReader actorInfoReader(actorInfoData);
  const auto numEntries = actorInfoReader.peekU16();

  mDrawIndexById.reserve(numEntries);
  for (uint16_t index = 0; index < numEntries; ++index)
  {
    const auto offset = actorInfoReader.readU16();

    LeStreamReader entryReader(actorInfoData);
    entryReader.skipBytes(offset * sizeof(uint16_t));

    const auto numFrames = entryReader.readU16();
    const auto drawIndex = entryReader.readS16();

    mDrawIndexById.push_back(drawIndex);

    vector<ActorFrameHeader> frameHeaders;
    for (size_t frame = 0; frame < numFrames; ++frame)
    {
      base::Vector drawOffset{entryReader.readS16(), entryReader.readS16()};

      const auto height = entryReader.readU16();
      const auto width = entryReader.readU16();
      base::Extents size{width, height};

      const auto imageDataOffset = entryReader.readU32();
      entryReader.skipBytes(4); // padding

      frameHeaders.emplace_back(
        ActorFrameHeader{drawOffset, size, imageDataOffset});
    }

    if (!frameHeaders.empty())
    {
      mHeadersById.emplace(
        ActorID(index), ActorHeader{drawIndex, move(frameHeaders)});
    }
  }

  if (mMaybeReplacementsPath)
  {
    std::ifstream sizeOverrideFile{
      *mMaybeReplacementsPath + "/actor_size_overrides.txt"};

    if (sizeOverrideFile.is_open())
    {
      for (std::string line; std::getline(sizeOverrideFile, line);)
      {
        const auto keyAndValue = strings::split(line, ':');
        if (keyAndValue.size() != 2)
        {
          continue;
        }

        const auto keyParts = strings::split(keyAndValue[0], ' ');
        const auto valueParts =
          strings::split(strings::trim(keyAndValue[1]), ' ');
        if (keyParts.size() != 2 || valueParts.size() != 4)
        {
          continue;
        }

        const auto id = std::stoi(keyParts[0]);
        const auto frame = std::stoi(keyParts[1]);
        const auto x = std::stoi(valueParts[0]);
        const auto y = std::stoi(valueParts[1]);
        const auto w = std::stoi(valueParts[2]);
        const auto h = std::stoi(valueParts[3]);

        mSizeOverrides.insert(
          {IdAndFrame{static_cast<data::ActorID>(id), frame},
           base::Rect<int>{{x, y}, {w, h}}});
      }
    }
  }
}


ActorData
  ActorImagePackage::loadActor(const ActorID id, const Palette16& palette) const
{
  // Font has to be loaded using loadFont()
  assert(id != data::ActorID::Menu_font_grayscale);

  const auto it = mHeadersById.find(id);
  if (it == mHeadersById.end())
  {
    throw invalid_argument(
      "loadActor(): No actor at this ID " +
      std::to_string(static_cast<int>(id)));
  }

  const auto& header = it->second;
  return ActorData{header.mDrawIndex, loadFrameImages(id, header, palette)};
}


std::vector<ActorData::Frame> ActorImagePackage::loadFrameImages(
  const data::ActorID id,
  const ActorHeader& header,
  const Palette16& palette) const
{
  return utils::transformed(
    header.mFrames, [&, this, frame = 0](const auto& frameHeader) mutable {
      const auto iSizeOverride = mSizeOverrides.find({id, frame});
      const auto offset = iSizeOverride != mSizeOverrides.end()
        ? iSizeOverride->second.topLeft
        : frameHeader.mDrawOffset;
      const auto size = iSizeOverride != mSizeOverrides.end()
        ? iSizeOverride->second.size
        : frameHeader.mSizeInTiles;

      auto maybeReplacement = mMaybeReplacementsPath
        ? loadPng(replacementImagePath(
            *mMaybeReplacementsPath, static_cast<int>(id), frame))
        : std::nullopt;
      ++frame;

      return ActorData::Frame{
        offset,
        size,
        maybeReplacement ? *maybeReplacement : loadImage(frameHeader, palette)};
    });
}


data::Image ActorImagePackage::loadImage(
  const ActorFrameHeader& frameHeader,
  const Palette16& palette) const
{
  using T = data::TileImageType;

  const auto width = frameHeader.mSizeInTiles.width;
  const auto height = frameHeader.mSizeInTiles.height;

  const auto dataSize = height * width * GameTraits::bytesPerTile(T::Masked);
  if (frameHeader.mFileOffset + dataSize > mImageData.size())
  {
    throw invalid_argument("Not enough data");
  }

  const auto dataStart = mImageData.begin() + frameHeader.mFileOffset;
  return loadTiledImage(
    dataStart, dataStart + dataSize, width, palette, T::Masked);
}


FontData ActorImagePackage::loadFont() const
{
  const auto it = mHeadersById.find(data::ActorID::Menu_font_grayscale);
  if (it == mHeadersById.end() || it->second.mFrames.empty())
  {
    throw runtime_error("Font data missing");
  }

  const auto& header = it->second;
  const auto sizeInTiles = header.mFrames.front().mSizeInTiles;

  FontData fontBitmaps;
  for (const auto& frameHeader : header.mFrames)
  {
    if (frameHeader.mSizeInTiles != sizeInTiles)
    {
      throw runtime_error("Font bitmaps must all be equally sized");
    }

    const auto dataSize =
      sizeInTiles.width * sizeInTiles.height * GameTraits::bytesPerFontTile();
    if (frameHeader.mFileOffset + dataSize > mImageData.size())
    {
      throw runtime_error("Not enough data");
    }

    const auto dataStart = mImageData.begin() + frameHeader.mFileOffset;
    auto characterBitmap =
      loadTiledFontBitmap(dataStart, dataStart + dataSize, sizeInTiles.width);
    fontBitmaps.emplace_back(std::move(characterBitmap));
  }

  return fontBitmaps;
}

} // namespace rigel::loader
