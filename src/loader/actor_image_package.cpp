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
#include "data/game_traits.hpp"
#include "data/unit_conversions.hpp"
#include "loader/ega_image_decoder.hpp"
#include "loader/file_utils.hpp"
#include "loader/png_image.hpp"

#include <cassert>
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
  const ByteBuffer& actorInfoData,
  std::string maybeImageReplacementsPath)
  : mMaybeReplacementsPath(std::move(maybeImageReplacementsPath))
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


ActorData ActorImagePackage::loadActor(
  const ActorID id,
  const data::Palette16& palette) const
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
  const data::Palette16& palette) const
{
  return utils::transformed(
    header.mFrames, [&, this, frame = 0](const auto& frameHeader) mutable {
      const auto maybeImage = loadPng(replacementImagePath(
        mMaybeReplacementsPath, static_cast<int>(id), frame));
      ++frame;

      if (!maybeImage)
      {
        throw std::runtime_error(
          "No image for actor " + std::to_string(static_cast<int>(id)) +
          ", frame " + std::to_string(frame - 1));
      }

      return ActorData::Frame{
        frameHeader.mDrawOffset,
        frameHeader.mSizeInTiles,
        std::move(*maybeImage)};
    });
}


FontData ActorImagePackage::loadFont() const
{
  return {};
}

} // namespace rigel::loader
