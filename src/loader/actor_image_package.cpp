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

#include <data/game_traits.hpp>
#include <data/unit_conversions.hpp>
#include <utils/container_tools.hpp>
#include <loader/cmp_file_package.hpp>
#include <loader/ega_image_decoder.hpp>
#include <loader/file_utils.hpp>

#include <cassert>


namespace rigel { namespace loader {

using namespace std;
using data::ActorID;
using data::GameTraits;


namespace {

const auto FONT_ACTOR_ID = 29;

}


ActorImagePackage::ActorImagePackage(const CMPFilePackage& filePackage)
  : mImageData(filePackage.file("ACTORS.MNI"))
{
  const auto actorInfoData = filePackage.file("ACTRINFO.MNI");

  LeStreamReader actorInfoReader(actorInfoData);
  const auto numEntries = actorInfoReader.peekU16();

  for (uint16_t index=0; index<numEntries; ++index) {
    const auto offset = actorInfoReader.readU16();

    LeStreamReader entryReader(actorInfoData);
    entryReader.skipBytes(offset * sizeof(uint16_t));

    const auto numFrames = entryReader.readU16();
    const auto drawIndex = entryReader.readS16();

    vector<ActorFrameHeader> frameHeaders;
    for (size_t frame=0; frame<numFrames; ++frame) {
      base::Vector drawOffset{entryReader.readS16(), entryReader.readS16()};

      const auto height = entryReader.readU16();
      const auto width = entryReader.readU16();
      base::Extents size{width, height};

      const auto imageDataOffset = entryReader.readU32();
      entryReader.skipBytes(4); // padding

      frameHeaders.emplace_back(
        ActorFrameHeader{drawOffset, size, imageDataOffset});
    }

    if (!frameHeaders.empty()) {
      mHeadersById.emplace(
        ActorID(index),
        ActorHeader{drawIndex, move(frameHeaders)});
    }
  }
}

ActorData ActorImagePackage::loadActor(
  const ActorID id,
  const Palette16& palette
) const {
  // Font has to be loaded using loadFont()
  assert(id != FONT_ACTOR_ID);

  const auto it = mHeadersById.find(id);
  if (it == mHeadersById.end()) {
    throw invalid_argument("No actor at this ID");
  }

  const auto& header = it->second;
  return ActorData{
    header.mDrawIndex,
    loadFrameImages(header, palette)
  };
}


std::vector<ActorData::Frame> ActorImagePackage::loadFrameImages(
  const ActorHeader& header,
  const Palette16& palette
) const {
  return utils::transformed(
    header.mFrames,
    [this, &palette](const auto& frameHeader) {
      const auto width = frameHeader.mSizeInTiles.width;
      const auto height = frameHeader.mSizeInTiles.height;

      const auto dataSize = height * width * GameTraits::bytesPerTile(true);
      if (frameHeader.mFileOffset + dataSize > mImageData.size()) {
        throw invalid_argument("Not enough data");
      }

      const auto dataStart = mImageData.cbegin() + frameHeader.mFileOffset;
      auto image = loadTiledImage(
        dataStart,
        dataStart + dataSize,
        width,
        palette,
        true);

      return ActorData::Frame{
        frameHeader.mDrawOffset,
        image};
    });
}


FontData ActorImagePackage::loadFont() const {
  const auto it = mHeadersById.find(FONT_ACTOR_ID);
  if (it == mHeadersById.end() || it->second.mFrames.empty()) {
    throw runtime_error("Font data missing");
  }

  const auto& header = it->second;
  const auto sizeInTiles = header.mFrames.front().mSizeInTiles;

  FontData fontBitmaps;
  for (const auto& frameHeader : header.mFrames) {
    if (frameHeader.mSizeInTiles != sizeInTiles) {
      throw runtime_error("Font bitmaps must all be equally sized");
    }

    const auto dataSize =
      sizeInTiles.width * sizeInTiles.height * GameTraits::bytesPerFontTile();
    if (frameHeader.mFileOffset + dataSize > mImageData.size()) {
      throw runtime_error("Not enough data");
    }

    const auto dataStart = mImageData.cbegin() + frameHeader.mFileOffset;
    auto characterBitmap = loadTiledFontBitmap(
      dataStart,
      dataStart + dataSize,
      sizeInTiles.width);
    fontBitmaps.emplace_back(std::move(characterBitmap));
  }

  return fontBitmaps;
}

}}
