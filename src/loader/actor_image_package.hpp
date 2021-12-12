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
#include <unordered_map>
#include <optional>
#include <string>
#include <vector>
#include <stdexcept>


namespace rigel::loader
{

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


using FontData = std::vector<data::Image>;


class ActorImagePackage
{
public:
  static constexpr auto IMAGE_DATA_FILE = "ACTORS.MNI";
  static constexpr auto ACTOR_INFO_FILE = "misc/actor_info.json";

  ActorImagePackage(
    const ByteBuffer& actorInfoData,
    std::string maybeImageReplacementsPath);

  ActorData loadActor(
    data::ActorID id,
    const data::Palette16& palette = data::GameTraits::INGAME_PALETTE) const;

  FontData loadFont() const;

  int drawIndexFor(data::ActorID id) const
  {
    const auto iResult = mDrawIndexById.find(static_cast<int>(id));
    if (iResult == mDrawIndexById.end())
    {
      throw std::runtime_error("No draw index for this ID");
    }

    return iResult->second;
  }

private:
  struct ActorFrameHeader
  {
    base::Vec2 mDrawOffset;
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
    const data::Palette16& palette) const;

  data::Image loadImage(
    const ActorFrameHeader& frameHeader,
    const data::Palette16& palette) const;

private:
  std::map<data::ActorID, ActorHeader> mHeadersById;
  std::unordered_map<int, int> mDrawIndexById;
  std::string mMaybeReplacementsPath;
};


} // namespace rigel::loader
