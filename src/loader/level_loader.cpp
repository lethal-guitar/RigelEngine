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

#include <base/grid.hpp>
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
using data::Difficulty;
using data::map::BackdropScrollMode;
using data::map::BackdropSwitchCondition;
using data::map::LevelData;
using data::GameTraits;


namespace {

using ActorList = std::vector<LevelData::Actor>;


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


/** Creates 2d grid of actor descriptions in a level
 *
 * Takes a linear list of actor descriptions, and puts them into a 2d grid.
 * This is useful since some meta actors have spatial relations to others.
 */
auto makeActorGrid(const data::map::Map& map, const ActorList& actors) {
  base::Grid<const LevelData::Actor*> actorGrid(map.width(), map.height());

  for (const auto& actor : actors) {
    actorGrid.setValueAt(actor.mPosition.x, actor.mPosition.y, &actor);
  }
  return actorGrid;
}


class ActorGrid {
public:
  ActorGrid(const data::map::Map& map, const ActorList& actors)
    : mActorGrid(makeActorGrid(map, actors))
  {
  }

  const LevelData::Actor& actorAt(const size_t col, const size_t row) const {
    return *mActorGrid.valueAt(col, row);
  }

  bool hasActorAt(const size_t col, const size_t row) const {
    return mActorGrid.valueAt(col, row) != nullptr;
  }

  void removeActorAt(const size_t col, const size_t row) {
    mActorGrid.setValueAt(col, row, nullptr);
  }

  boost::optional<base::Rect<int>> findTileSectionRect(
    const int startCol,
    const int startRow
  ) {
    for (auto x=startCol; x<int(mActorGrid.width()); ++x) {
      auto pTopRightMarkerCandidate = mActorGrid.valueAt(x, startRow);

      if (pTopRightMarkerCandidate && pTopRightMarkerCandidate->mID == 103) {
        const auto rightCol = pTopRightMarkerCandidate->mPosition.x;

        for (auto y=startRow+1; y<int(mActorGrid.height()); ++y) {
          auto pBottomRightMarkerCandidate = mActorGrid.valueAt(rightCol, y);

          if (
            pBottomRightMarkerCandidate &&
            pBottomRightMarkerCandidate->mID == 104
          ) {
            const auto bottomRow = y;
            removeActorAt(rightCol, startRow);
            removeActorAt(rightCol, bottomRow);

            return base::Rect<int>{
              {startCol, startRow},
              {rightCol - startCol + 1, bottomRow - startRow + 1}
            };
          }
        }
      }
    }

    return boost::none;
  }

private:
  base::Grid<const LevelData::Actor*> mActorGrid;
};


/** Transforms actor list to be more useful in subsequent stages
 *
 * This does two things:
 *  - Applies the selected difficulty, i.e. removes actors that only appear
 *    in higher difficulties than the selected one
 *  - Assigns an area/bounding box to actors that require it, e.g. shootable
 *    walls
 *
 * Actors which are only relevant for these two purposes will be removed from
 * the list (difficulty markers and section markers).
 */
ActorList preProcessActorDescriptions(
  const data::map::Map& map,
  const ActorList& originalActors,
  const Difficulty chosenDifficulty
) {
  ActorList actors;

  ActorGrid grid(map, originalActors);
  for (int row=0; row<map.height(); ++row) {
    for (int col=0; col<map.width(); ++col) {
      if (!grid.hasActorAt(col, row)) {
        continue;
      }

      auto applyDifficultyMarker = [&grid, col, row, chosenDifficulty](
        const Difficulty requiredDifficulty
      ) {
        const auto targetCol = col + 1;
        if (chosenDifficulty < requiredDifficulty) {
          grid.removeActorAt(targetCol, row);
        }
      };

      const auto& actor = grid.actorAt(col, row);
      switch (actor.mID) {
        case 82:
          applyDifficultyMarker(Difficulty::Medium);
          break;

        case 83:
          applyDifficultyMarker(Difficulty::Hard);
          break;

        case 103: // stray tile section marker, ignore
        case 104: // stray tile section marker, ignore
          break;

        case 102:
        case 106:
        case 116:
        case 137:
        case 138:
        case 142:
        case 143:
          {
            auto tileSection = grid.findTileSectionRect(col, row);
            if (tileSection) {
              actors.emplace_back(LevelData::Actor{
                actor.mPosition,
                actor.mID,
                tileSection
              });
            }
          }
          break;

        default:
          actors.emplace_back(
            LevelData::Actor{actor.mPosition, actor.mID, boost::none});
          break;
      }

      grid.removeActorAt(col, row);
    }
  }

  return actors;
}

}


LevelData loadLevel(
  const string& mapName,
  const ResourceLoader& resources,
  const Difficulty chosenDifficulty
) {
  const auto levelData = resources.mFilePackage.file(mapName);
  LeStreamReader levelReader(levelData);

  LevelHeader header(levelReader);
  ActorList actors;
  for (size_t i=0; i<header.numActorWords/3; ++i) {
    const auto type = levelReader.readU16();
    const base::Vector position{levelReader.readU16(), levelReader.readU16()};
    actors.emplace_back(LevelData::Actor{position, type, boost::none});
  }

  const auto width = static_cast<int>(levelReader.readU16());
  const auto height = static_cast<int>(GameTraits::mapHeightForWidth(width));
  data::map::Map map(width, height);

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

  const auto backdropImage = resources.loadTiledFullscreenImage(
    header.backdrop);
  boost::optional<data::Image> alternativeBackdropImage;
  if (header.flagBitSet(0x40) || header.flagBitSet(0x80)) {
    alternativeBackdropImage = resources.loadTiledFullscreenImage(
      backdropNameFromNumber(header.alternativeBackdropNumber));
  }
  auto actorDescriptions =
      preProcessActorDescriptions(map, actors, chosenDifficulty);
  return LevelData{
    resources.loadCZone(header.CZone),
    std::move(backdropImage),
    std::move(alternativeBackdropImage),
    std::move(map),
    std::move(actorDescriptions),
    scrollMode,
    backdropSwitchCondition,
    header.flagBitSet(0x20),
    header.music
  };
}


}}
