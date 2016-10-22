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

#include "movie_loader.hpp"

#include <loader/file_utils.hpp>
#include <loader/palette.hpp>
#include <loader/rle_compression.hpp>
#include <stdexcept>

/* Duke Nukem II Movie/Animation loader
 *
 * This implementation is based on information found at:
 *
 *   http://www.shikadi.net/moddingwiki/Duke_Nukem_II_Animation_Format
 */


namespace rigel { namespace loader {

using namespace std;

namespace {

const char* INVALID_MOVIE_FILE = "Invalid/corrupted movie file";

enum class SubChunkType {
  Palette,
  MainImage,
  AnimationFrame
};


struct SubChunkHeader {
  explicit SubChunkHeader(LeStreamReader& reader)
    : mSize(reader.readU32())
  {
    const auto typeId = reader.readU16();
    switch (typeId) {
      case 0xB:
        if (mSize != 778) {
          throw invalid_argument(INVALID_MOVIE_FILE);
        }
        mType = SubChunkType::Palette;
        break;

      case 0xF:
        mType = SubChunkType::MainImage;
        break;

      case 0xC:
        mType = SubChunkType::AnimationFrame;
        break;

      default:
        throw invalid_argument(INVALID_MOVIE_FILE);
    }
  }

  uint32_t mSize = 0;
  SubChunkType mType;
};


struct ChunkHeader {
  explicit ChunkHeader(LeStreamReader& reader)
    : mSize(reader.readU32())
    , mType(reader.readU16())
    , mNumSubChunks(reader.readU16())
  {
    if (mType != 0xF1FA) {
      throw invalid_argument(INVALID_MOVIE_FILE);
    }

    reader.skipBytes(8);
  }

  uint32_t mSize = 0;
  uint16_t mType = 0;
  uint16_t mNumSubChunks = 0;
};


Palette256 readPalette(LeStreamReader& reader) {
  SubChunkHeader mPaletteChunkHeader(reader);
  if (mPaletteChunkHeader.mType != SubChunkType::Palette) {
    throw invalid_argument(INVALID_MOVIE_FILE);
  }
  reader.skipBytes(4); // always 1
  const auto palette = load6bitPalette256(
    reader.currentIter(),
    reader.currentIter() + 768);
  reader.skipBytes(768);
  return palette;
}


data::PixelBuffer readMainImagePixels(
  LeStreamReader& reader,
  const uint16_t width,
  const uint16_t height,
  const Palette256& palette
) {
  SubChunkHeader mainImageSubChunkHeader(reader);
  if (mainImageSubChunkHeader.mType != SubChunkType::MainImage) {
    throw invalid_argument(INVALID_MOVIE_FILE);
  }

  data::PixelBuffer mainImagePixels;
  mainImagePixels.reserve(width * height);

  for (auto row=0u; row<height; ++row) {
    const auto numRLEFlagsInRow = reader.readU8();
    decompressRle(reader, numRLEFlagsInRow,
      [&mainImagePixels, &palette](const auto colorIndex) {
        mainImagePixels.push_back(palette[colorIndex]);
      });
  }

  return mainImagePixels;
}


data::PixelBuffer readAnimationFramePixels(
  LeStreamReader& reader,
  const uint16_t width,
  const uint16_t height,
  const Palette256& palette
) {
  data::PixelBuffer framePixels(width * height, data::Pixel{});

  for (auto row=0u; row<height; ++row) {
    const auto startOffset = row * width;
    auto targetCol = 0;

    const auto numRleWords = reader.readU8();
    for (auto rleEntry=0u; rleEntry<numRleWords; ++rleEntry) {
      const auto pixelsToSkip = reader.readU8();
      targetCol += pixelsToSkip;

      // For some reason, the RLE markers are inverted in the animation frame
      // chunks...
      const auto invertedMarkerByte = reader.readS8();
      expandSingleRleWord(-invertedMarkerByte, reader,
        [&framePixels, &palette, &targetCol, startOffset](
          const auto colorIndex
        ) {
          framePixels[targetCol++ + startOffset] = palette[colorIndex];
        });
    }
  }

  return framePixels;
}


vector<data::MovieFrame> readAnimationFrames(
  LeStreamReader& reader,
  const uint16_t width,
  const uint16_t numAnimFrames,
  const Palette256& palette
) {
  vector<data::MovieFrame> frames;
  for (auto frame=0u; frame<numAnimFrames; ++frame) {
    ChunkHeader frameChunkHeader(reader);
    SubChunkHeader frameChunkSubHeader(reader);
    if (
      frameChunkHeader.mNumSubChunks != 1 ||
      frameChunkSubHeader.mType != SubChunkType::AnimationFrame
    ) {
      throw invalid_argument(INVALID_MOVIE_FILE);
    }

    const auto yOffset = reader.readU16();
    const auto numRows = reader.readU16();
    frames.emplace_back(
      data::Image(
        readAnimationFramePixels(reader, width, numRows, palette),
        width,
        numRows),
      yOffset);
  }

  return frames;
}

}


data::Movie loadMovie(const ByteBuffer& file) {
  LeStreamReader reader(file);

  const auto fileSize = reader.readU32();
  const auto type = reader.readU16();
  const auto numAnimFrames = reader.readU16();
  const auto width = reader.readU16();
  const auto height = reader.readU16();
  reader.skipBytes(4 + 4); // unknown1, unknown2
  reader.skipBytes(108); // padding

  if (fileSize != file.size() || type != 0xAF11) {
    throw invalid_argument(INVALID_MOVIE_FILE);
  }
  ChunkHeader mainImageChunkHeader(reader);
  if (mainImageChunkHeader.mNumSubChunks != 2) {
    throw invalid_argument(INVALID_MOVIE_FILE);
  }
  const auto palette = readPalette(reader);
  auto mainImagePixels =
    readMainImagePixels(reader, width, height, palette);

  auto frames =
    readAnimationFrames(reader, width, numAnimFrames, palette);
  return {
    data::Image(std::move(mainImagePixels), width, height),
    std::move(frames)
  };
}


}}
