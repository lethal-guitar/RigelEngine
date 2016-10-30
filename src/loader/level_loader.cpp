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

#include "level_loader.hpp"

#include <fstream>
#include <string>
#include <type_traits>

#include <data/game_traits.hpp>
#include <data/unit_conversions.hpp>
#include <loader/bitwise_iter.hpp>
#include <loader/file_utils.hpp>
#include <loader/resource_loader.hpp>
#include <loader/rle_compression.hpp>
#include <utils/container_tools.hpp>
#include <utils/math_tools.hpp>


/* Duke Nukem II level loader
 *
 * This is mainly based on Dave Bollinger's game file format information,
 * plus some reverse-engineering efforts of my own to figure out the additional
 * masked tile bit section format (which was done many years ago, before that
 * information became available on wikis).
 *
 * See http://archive.shikadi.net/sites/www.geocities.com/dooknookimklassik/dn2specs.txt
 */


namespace rigel { namespace loader {

using namespace std;
using data::ActorID;
using data::map::BackdropScrollMode;
using data::map::BackdropSwitchCondition;
using data::map::LevelData;
using data::GameTraits;


namespace {

string stripSpaces(string str) {
  const auto it = str.find(' ');
  if (it != str.npos) {
    str.erase(it);
  }
  return str;
}


data::map::TileIndex convertTileIndex(const uint16_t rawIndex) {
  const auto index = rawIndex / 8u;
  if (index >= GameTraits::CZone::numSolidTiles) {
    return
      (index - GameTraits::CZone::numSolidTiles) / 5u +
      GameTraits::CZone::numSolidTiles;
  } else {
    return index;
  }
}


struct LevelHeader {
  explicit LevelHeader(LeStreamReader& reader)
    : dataOffset(reader.readU16())
    , CZone(stripSpaces(readFixedSizeString(reader, 13)))
    , backdrop(stripSpaces(readFixedSizeString(reader, 13)))
    , music(stripSpaces(readFixedSizeString(reader, 13)))
    , flags(reader.readU8())
    , alternativeBackdropNumber(reader.readU8())
    , unknown(reader.readU16())
    , numActorWords(reader.readU16())
  {
  }

  bool flagBitSet(const uint8_t bitMask) {
    return (flags & bitMask) != 0;
  }

  const uint16_t dataOffset;
  const string CZone;
  const string backdrop;
  const string music;
  const uint8_t flags;
  const uint8_t alternativeBackdropNumber;
  const uint16_t unknown;
  const uint16_t numActorWords;
};


ByteBuffer readExtraMaskedTileBits(const LeStreamReader& levelReader) {
  LeStreamReader extraInfoReader(levelReader);
  extraInfoReader.skipBytes(GameTraits::mapDataWords * sizeof(uint16_t));
  const auto extraInfoSize = extraInfoReader.readU16();

  LeStreamReader rleReader(
    extraInfoReader.currentIter(),
    extraInfoReader.currentIter() + extraInfoSize);

  ByteBuffer maskedTileOffsets;
  // The uncompressed masked tile extra bits contain 2 bits for each tile, so
  // we need one byte to represent 4 tiles.
  maskedTileOffsets.reserve(
    utils::integerDivCeil<size_t>(GameTraits::mapDataWords, 4u));

  decompressRle(rleReader, [&maskedTileOffsets](const auto decoded) {
    maskedTileOffsets.push_back(decoded);
  });

  return maskedTileOffsets;
}


string backdropNameFromNumber(const uint8_t backdropNumber) {
  auto name = string("DROP");
  name += to_string(backdropNumber);
  name += ".MNI";
  return name;
}

}


LevelData loadLevel(
  const string& mapName,
  const ResourceLoader& resources
) {
  const auto levelData = resources.mFilePackage.file(mapName);
  LeStreamReader levelReader(levelData);

  LevelHeader header(levelReader);
  vector<LevelData::Actor> actors;
  for (size_t i=0; i<header.numActorWords/3; ++i) {
    const auto type = levelReader.readU16();
    const base::Vector position{levelReader.readU16(), levelReader.readU16()};
    actors.emplace_back(LevelData::Actor{position, type});
  }

  const auto backdropImage = resources.loadTiledFullscreenImage(
    header.backdrop);
  boost::optional<data::Image> alternativeBackdropImage;
  if (header.flagBitSet(0x40) || header.flagBitSet(0x80)) {
    alternativeBackdropImage = resources.loadTiledFullscreenImage(
      backdropNameFromNumber(header.alternativeBackdropNumber));
  }

  const auto width = static_cast<int>(levelReader.readU16());
  const auto height = static_cast<int>(GameTraits::mapHeightForWidth(width));
  data::map::Map map(
    resources.loadCZone(header.CZone),
    std::move(backdropImage),
    std::move(alternativeBackdropImage),
    width,
    height);

  const auto maskedTileOffsets = readExtraMaskedTileBits(levelReader);

  auto lookupExtraMaskedTileBits = [&maskedTileOffsets, width, height](
    const int x,
    const int y
  ) {
    const auto index = x/4 + y*(width/4);
    const auto extraBitPack = maskedTileOffsets.at(index);

    const auto indexInPack = x % 4;
    const auto bitMask = 0x03 << indexInPack*2;
    const auto extraBits = (extraBitPack & bitMask) >> indexInPack*2;

    return static_cast<uint8_t>(extraBits << 5);
  };

  LeStreamReader tileDataReader(
    levelReader.currentIter(),
    levelReader.currentIter() + width*height*sizeof(uint16_t));
  for (int y=0; y<height; ++y) {
    for (int x=0; x<width; ++x) {
      const auto tileSpec = tileDataReader.readU16();

      if (tileSpec & 0x8000) {
        // extended tile spec: separate indices for layers 0 and 1.
        // 10 bits for solid, 5 for masked (the most significant bit serves
        // as a marker to distinguish the complex and simple masked tile
        // combination cases).
        const auto solidIndex = tileSpec & 0x3FF;
        auto maskedIndex = ((tileSpec & 0x7C00) >> 10);
        maskedIndex |= lookupExtraMaskedTileBits(x, y);
        maskedIndex += GameTraits::CZone::numSolidTiles;

        map.setTileAt(0, x, y, solidIndex);
        map.setTileAt(1, x, y, maskedIndex);
      } else {
        const auto index = convertTileIndex(tileSpec);
        const auto layerToUse =
          index >= GameTraits::CZone::numSolidTiles ? 1 : 0;
        map.setTileAt(layerToUse, x, y, index);
      }
    }
  }

  auto scrollMode = BackdropScrollMode::None;
  if (header.flagBitSet(0x1)) {
    scrollMode = BackdropScrollMode::ParallaxBoth;
  } else if (header.flagBitSet(0x2)) {
    scrollMode = BackdropScrollMode::ParallaxHorizontal;
  } else if (header.flagBitSet(0x8)) {
    scrollMode = BackdropScrollMode::AutoHorizontal;
  } else if (header.flagBitSet(0x10)) {
    scrollMode = BackdropScrollMode::AutoVertical;
  }
  auto backdropSwitchCondition = BackdropSwitchCondition::None;
  if (
    scrollMode != BackdropScrollMode::AutoHorizontal &&
    scrollMode != BackdropScrollMode::AutoVertical
    ) {
    if (header.flagBitSet(0x40)) {
      backdropSwitchCondition = BackdropSwitchCondition::OnReactorDestruction;
    }
    else if (header.flagBitSet(0x80)) {
      backdropSwitchCondition = BackdropSwitchCondition::OnTeleportation;
    }
  }

  return LevelData{
    move(map),
    move(actors),
    scrollMode,
    backdropSwitchCondition,
    header.flagBitSet(0x20),
    header.music
  };
}


}}
