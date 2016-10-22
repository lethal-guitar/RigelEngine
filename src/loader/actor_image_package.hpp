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

#include <map>
#include <vector>

#include <base/spatial_types.hpp>
#include <data/image.hpp>
#include <data/map.hpp>
#include <loader/byte_buffer.hpp>
#include <loader/palette.hpp>


namespace rigel { namespace loader {

class CMPFilePackage;


struct ActorData {
  struct Frame {
    base::Vector mDrawOffset;
    data::Image mFrameImage;
  };

  int mDrawIndex;
  std::vector<Frame> mFrames;
};


struct FontData {
  base::Extents mSingleCharacterBitmapSize;
  int mCharacterCount;
  data::Image mCharacterBitmaps;
};


class ActorImagePackage {
public:
  explicit ActorImagePackage(const CMPFilePackage& filePackage);

  ActorData loadActor(
    data::ActorID id,
    const Palette16& palette = INGAME_PALETTE) const;

  FontData loadFont() const;

private:
  struct ActorFrameHeader {
    base::Vector mDrawOffset;
    base::Extents mSizeInTiles;
    std::uint32_t mFileOffset;
  };

  struct ActorHeader {
    int mDrawIndex;
    std::vector<ActorFrameHeader> mFrames;
  };

  std::vector<ActorData::Frame> loadFrameImages(
    const ActorHeader& header,
    const Palette16& palette) const;

private:
  const ByteBuffer mImageData;
  std::map<data::ActorID, ActorHeader> mHeadersById;
};


}}
