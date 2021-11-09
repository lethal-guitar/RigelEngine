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

#include "base/spatial_types.hpp"
#include "data/actor_ids.hpp"
#include "data/game_traits.hpp"
#include "data/image.hpp"
#include "loader/byte_buffer.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>


namespace rigel::loader
{

struct ActorData
{
  struct Frame
  {
    base::Vector mDrawOffset;
    base::Extents mLogicalSize;
    data::Image mFrameImage;
  };

  int mDrawIndex;
  std::vector<Frame> mFrames;
};


using FontData = std::vector<data::Image>;


class ActorImagePackage
{
public:
  static constexpr auto IMAGE_DATA_FILE = "ACTORS.MNI";
  static constexpr auto ACTOR_INFO_FILE = "ACTRINFO.MNI";

  ActorImagePackage(
    ByteBuffer imageData,
    const ByteBuffer& actorInfoData,
    std::optional<std::string> maybeImageReplacementsPath = std::nullopt);

  ActorData loadActor(
    data::ActorID id,
    const Palette16& palette = data::GameTraits::INGAME_PALETTE) const;

  FontData loadFont() const;

  int drawIndexFor(data::ActorID id) const
  {
    return mDrawIndexById.at(static_cast<size_t>(id));
  }

private:
  struct ActorFrameHeader
  {
    base::Vector mDrawOffset;
    base::Extents mSizeInTiles;
    std::uint32_t mFileOffset;
  };

  struct ActorHeader
  {
    int mDrawIndex;
    std::vector<ActorFrameHeader> mFrames;
  };

  std::vector<ActorData::Frame> loadFrameImages(
    data::ActorID id,
    const ActorHeader& header,
    const Palette16& palette) const;

  data::Image loadImage(
    const ActorFrameHeader& frameHeader,
    const Palette16& palette) const;

private:
  const ByteBuffer mImageData;
  std::map<data::ActorID, ActorHeader> mHeadersById;
  std::vector<int> mDrawIndexById;
  std::optional<std::string> mMaybeReplacementsPath;
};


} // namespace rigel::loader
