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

#include "assets/ega_image_decoder.hpp"
#include "assets/file_utils.hpp"
#include "base/container_utils.hpp"
#include "data/game_traits.hpp"
#include "data/unit_conversions.hpp"

#include <cassert>
#include <utility>


namespace rigel::assets
{

using namespace std;
using data::ActorID;
using data::GameTraits;


ActorImagePackage::ActorImagePackage(
  ByteBuffer imageData,
  const ByteBuffer& actorInfoData)
  : mImageData(std::move(imageData))
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
      base::Vec2 drawOffset{entryReader.readS16(), entryReader.readS16()};

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
}


const ActorHeader& ActorImagePackage::loadActorInfo(const ActorID id) const
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

  return it->second;
}


data::Image ActorImagePackage::loadImage(
  const ActorFrameHeader& frameHeader,
  const data::Palette16& palette) const
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

} // namespace rigel::assets
