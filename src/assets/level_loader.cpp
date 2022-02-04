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

#include "assets/bitwise_iter.hpp"
#include "assets/file_utils.hpp"
#include "assets/resource_loader.hpp"
#include "assets/rle_compression.hpp"
#include "base/container_utils.hpp"
#include "base/grid.hpp"
#include "base/math_utils.hpp"
#include "base/string_utils.hpp"
#include "data/game_traits.hpp"
#include "data/unit_conversions.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <type_traits>


/* Duke Nukem II level loader
 *
 * This is mainly based on Dave Bollinger's game file format information,
 * plus some reverse-engineering efforts of my own to figure out the additional
 * masked tile bit section format (which was done many years ago, before that
 * information became available on wikis).
 *
 * See
 * http://archive.shikadi.net/sites/www.geocities.com/dooknookimklassik/dn2specs.txt
 */


namespace rigel::assets
{

using data::ActorID;
using data::Difficulty;
using data::GameTraits;
using data::map::BackdropScrollMode;
using data::map::BackdropSwitchCondition;
using data::map::LevelData;


namespace
{

using ActorList = std::vector<LevelData::Actor>;


constexpr auto VALID_LEVEL_WIDTHS = std::array{32, 64, 128, 256, 512, 1024};


bool isValidWidth(int width)
{
  using std::begin;
  using std::end;
  using std::find;

  return find(begin(VALID_LEVEL_WIDTHS), end(VALID_LEVEL_WIDTHS), width) !=
    end(VALID_LEVEL_WIDTHS);
}


data::map::TileIndex convertTileIndex(const uint16_t rawIndex)
{
  const auto index = rawIndex / 8u;
  if (index >= GameTraits::CZone::numSolidTiles)
  {
    return (index - GameTraits::CZone::numSolidTiles) / 5u +
      GameTraits::CZone::numSolidTiles;
  }
  else
  {
    return index;
  }
}


struct LevelHeader
{
  explicit LevelHeader(LeStreamReader& reader)
    : dataOffset(reader.readU16())
    , CZone(strings::trimRight(readFixedSizeString(reader, 13)))
    , backdrop(strings::trimRight(readFixedSizeString(reader, 13)))
    , music(strings::trimRight(readFixedSizeString(reader, 13)))
    , flags(reader.readU8())
    , alternativeBackdropNumber(reader.readU8())
    , unknown(reader.readU16())
    , numActorWords(reader.readU16())
  {
  }

  bool flagBitSet(const uint8_t bitMask) { return (flags & bitMask) != 0; }

  const uint16_t dataOffset;
  const std::string CZone;
  const std::string backdrop;
  const std::string music;
  const uint8_t flags;
  const uint8_t alternativeBackdropNumber;
  const uint16_t unknown;
  const uint16_t numActorWords;
};


ByteBuffer readExtraMaskedTileBits(const LeStreamReader& levelReader)
{
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
    base::integerDivCeil<size_t>(GameTraits::mapDataWords, 4u));

  decompressRle(rleReader, [&maskedTileOffsets](const auto decoded) {
    maskedTileOffsets.push_back(decoded);
  });

  return maskedTileOffsets;
}


std::string backdropNameFromNumber(const uint8_t backdropNumber)
{
  auto name = std::string("DROP");
  name += std::to_string(backdropNumber);
  name += ".MNI";
  return name;
}


/** Creates 2d grid of actor descriptions in a level
 *
 * Takes a linear list of actor descriptions, and puts them into a 2d grid.
 * This is useful since some meta actors have spatial relations to others.
 */
auto makeActorGrid(const data::map::Map& map, const ActorList& actors)
{
  base::Grid<const LevelData::Actor*> actorGrid(map.width(), map.height());

  for (const auto& actor : actors)
  {
    actorGrid.setValueAt(actor.mPosition.x, actor.mPosition.y, &actor);
  }
  return actorGrid;
}


class ActorGrid
{
public:
  ActorGrid(const data::map::Map& map, const ActorList& actors)
    : mActorGrid(makeActorGrid(map, actors))
  {
  }

  const LevelData::Actor& actorAt(const size_t col, const size_t row) const
  {
    return *mActorGrid.valueAt(col, row);
  }

  bool hasActorAt(const size_t col, const size_t row) const
  {
    return mActorGrid.valueAt(col, row) != nullptr;
  }

  void removeActorAt(const size_t col, const size_t row)
  {
    mActorGrid.setValueAt(col, row, nullptr);
  }

  std::optional<base::Rect<int>>
    findTileSectionRect(const int startCol, const int startRow)
  {
    for (auto x = startCol; x < int(mActorGrid.width()); ++x)
    {
      auto pTopRightMarkerCandidate = mActorGrid.valueAt(x, startRow);

      if (
        pTopRightMarkerCandidate &&
        pTopRightMarkerCandidate->mID ==
          ActorID::META_Dynamic_geometry_marker_1)
      {
        const auto rightCol = pTopRightMarkerCandidate->mPosition.x;

        for (auto y = startRow + 1; y < int(mActorGrid.height()); ++y)
        {
          auto pBottomRightMarkerCandidate = mActorGrid.valueAt(rightCol, y);

          if (
            pBottomRightMarkerCandidate &&
            pBottomRightMarkerCandidate->mID ==
              ActorID::META_Dynamic_geometry_marker_2)
          {
            const auto bottomRow = y;
            removeActorAt(rightCol, startRow);
            removeActorAt(rightCol, bottomRow);

            return base::Rect<int>{
              {startCol, startRow},
              {rightCol - startCol + 1, bottomRow - startRow + 1}};
          }
        }
      }
    }

    return std::nullopt;
  }

private:
  base::Grid<const LevelData::Actor*> mActorGrid;
};


bool isValidActorId(const uint16_t id)
{
  using ID = data::ActorID;

  switch (static_cast<data::ActorID>(id))
  {
    case ID::Keyhole_mounting_pole:
    case ID::Duke_LEFT:
    case ID::Duke_RIGHT:
    case ID::META_Dynamic_geometry_marker_1:
    case ID::META_Dynamic_geometry_marker_2:
    case ID::Laser_turret_mounting_post:
    case ID::META_Appear_only_in_med_hard_difficulty:
    case ID::META_Appear_only_in_hard_difficulty:
    case ID::Blue_bonus_globe_1:
    case ID::Blue_bonus_globe_2:
    case ID::Blue_bonus_globe_3:
    case ID::Blue_bonus_globe_4:
    case ID::Force_field:
    case ID::Circuit_card_keyhole:
    case ID::Blue_key_keyhole:
    case ID::Green_box_empty:
    case ID::Red_box_empty:
    case ID::Blue_box_empty:
    case ID::White_box_empty:
    case ID::White_box_circuit_card:
    case ID::White_box_blue_key:
    case ID::White_box_rapid_fire:
    case ID::White_box_cloaking_device:
    case ID::Red_box_bomb:
    case ID::Red_box_cola:
    case ID::Red_box_6_pack_cola:
    case ID::Red_box_turkey:
    case ID::Green_box_rocket_launcher:
    case ID::Green_box_flame_thrower:
    case ID::Green_box_normal_weapon:
    case ID::Green_box_laser:
    case ID::Blue_box_health_molecule:
    case ID::Blue_box_N:
    case ID::Blue_box_U:
    case ID::Blue_box_K:
    case ID::Blue_box_E:
    case ID::Blue_box_M:
    case ID::Blue_box_video_game_cartridge:
    case ID::Blue_box_sunglasses:
    case ID::Blue_box_phone:
    case ID::Blue_box_boom_box:
    case ID::Blue_box_disk:
    case ID::Blue_box_TV:
    case ID::Blue_box_camera:
    case ID::Blue_box_PC:
    case ID::Blue_box_CD:
    case ID::Blue_box_T_shirt:
    case ID::Blue_box_videocassette:
    case ID::Teleporter_1:
    case ID::Teleporter_2:
    case ID::Respawn_checkpoint:
    case ID::Special_hint_globe:
    case ID::Hoverbot:
    case ID::Big_green_cat_LEFT:
    case ID::Big_green_cat_RIGHT:
    case ID::Wall_mounted_flamethrower_RIGHT:
    case ID::Wall_mounted_flamethrower_LEFT:
    case ID::Watchbot:
    case ID::Rocket_launcher_turret:
    case ID::Enemy_rocket_left:
    case ID::Enemy_rocket_up:
    case ID::Enemy_rocket_right:
    case ID::Enemy_rocket_2_up:
    case ID::Enemy_rocket_2_down:
    case ID::Watchbot_container_carrier:
    case ID::Bomb_dropping_spaceship:
    case ID::Napalm_bomb:
    case ID::Bouncing_spike_ball:
    case ID::Green_slime_blob:
    case ID::Green_slime_container:
    case ID::Napalm_bomb_small:
    case ID::Snake:
    case ID::Camera_on_ceiling:
    case ID::Camera_on_floor:
    case ID::Green_hanging_suction_plant:
    case ID::Wall_walker:
    case ID::Eyeball_thrower_LEFT:
    case ID::Sentry_robot_generator:
    case ID::Skeleton:
    case ID::Metal_grabber_claw:
    case ID::Hovering_laser_turret:
    case ID::Spider:
    case ID::Ugly_green_bird:
    case ID::Spiked_green_creature_LEFT:
    case ID::Spiked_green_creature_RIGHT:
    case ID::Small_flying_ship_1:
    case ID::Small_flying_ship_2:
    case ID::Small_flying_ship_3:
    case ID::Blue_guard_RIGHT:
    case ID::Blue_guard_LEFT:
    case ID::Blue_guard_using_a_terminal:
    case ID::Laser_turret:
    case ID::BOSS_Episode_1:
    case ID::BOSS_Episode_2:
    case ID::BOSS_Episode_3:
    case ID::BOSS_Episode_4:
    case ID::BOSS_Episode_4_projectile:
    case ID::Red_bird:
    case ID::Smash_hammer:
    case ID::Unicycle_bot:
    case ID::Aggressive_prisoner:
    case ID::Passive_prisoner:
    case ID::Rigelatin_soldier:
    case ID::Dukes_ship_LEFT:
    case ID::Dukes_ship_RIGHT:
    case ID::Dukes_ship_after_exiting_LEFT:
    case ID::Dukes_ship_after_exiting_RIGHT:
    case ID::Nuclear_waste_can_empty:
    case ID::Nuclear_waste_can_green_slime_inside:
    case ID::Electric_reactor:
    case ID::Super_force_field_LEFT:
    case ID::Missile_broken:
    case ID::Sliding_door_vertical:
    case ID::Blowing_fan:
    case ID::Sliding_door_horizontal:
    case ID::Missile_intact:
    case ID::Rocket_elevator:
    case ID::Lava_pit:
    case ID::Green_acid_pit:
    case ID::Fire_on_floor_1:
    case ID::Fire_on_floor_2:
    case ID::Slime_pipe:
    case ID::Floating_exit_sign_RIGHT:
    case ID::Floating_exit_sign_LEFT:
    case ID::Floating_arrow:
    case ID::Radar_dish:
    case ID::Radar_computer_terminal:
    case ID::Special_hint_machine:
    case ID::Rotating_floor_spikes:
    case ID::Computer_Terminal_Duke_Escaped:
    case ID::Lava_fall_1:
    case ID::Lava_fall_2:
    case ID::Water_fall_1:
    case ID::Water_fall_2:
    case ID::Water_fall_splash_left:
    case ID::Water_fall_splash_center:
    case ID::Water_fall_splash_right:
    case ID::Water_on_floor_1:
    case ID::Water_on_floor_2:
    case ID::Messenger_drone_1:
    case ID::Messenger_drone_2:
    case ID::Messenger_drone_3:
    case ID::Messenger_drone_4:
    case ID::Messenger_drone_5:
    case ID::Lava_fountain:
    case ID::Flame_jet_1:
    case ID::Flame_jet_2:
    case ID::Flame_jet_3:
    case ID::Flame_jet_4:
    case ID::Exit_trigger:
    case ID::Dynamic_geometry_2:
    case ID::Dynamic_geometry_3:
    case ID::Dynamic_geometry_1:
    case ID::Dynamic_geometry_4:
    case ID::Dynamic_geometry_5:
    case ID::Dynamic_geometry_6:
    case ID::Dynamic_geometry_7:
    case ID::Dynamic_geometry_8:
    case ID::Water_body:
    case ID::Water_drop:
    case ID::Water_drop_spawner:
    case ID::Water_surface_1:
    case ID::Water_surface_2:
    case ID::Windblown_spider_generator:
    case ID::Airlock_death_trigger_LEFT:
    case ID::Airlock_death_trigger_RIGHT:
    case ID::Explosion_FX_trigger:
    case ID::Enemy_laser_shot_RIGHT:
      return true;

    default:
      break;
  }

  return false;
}


/** Transforms actor list to be more useful in subsequent stages
 *
 * This does three things:
 *  - Applies the selected difficulty, i.e. removes actors that only appear
 *    in higher difficulties than the selected one
 *  - Assigns an area/bounding box to actors that require it, e.g. shootable
 *    walls
 *  - Extracts the player spawn position and orientation
 *
 * Actors which are only relevant for these purposes will be removed from
 * the list (difficulty markers and section markers, player).
 */
std::tuple<ActorList, base::Vec2, bool> preProcessActorDescriptions(
  const data::map::Map& map,
  const ActorList& originalActors,
  const Difficulty chosenDifficulty)
{
  ActorList actors;
  base::Vec2 playerSpawnPosition;
  bool playerFacingLeft = false;

  ActorGrid grid(map, originalActors);
  for (int row = 0; row < map.height(); ++row)
  {
    for (int col = 0; col < map.width(); ++col)
    {
      if (!grid.hasActorAt(col, row))
      {
        continue;
      }

      auto applyDifficultyMarker = [&grid, col, row, chosenDifficulty](
                                     const Difficulty requiredDifficulty) {
        const auto targetCol = col + 1;
        if (chosenDifficulty < requiredDifficulty)
        {
          grid.removeActorAt(targetCol, row);
        }
      };

      const auto& actor = grid.actorAt(col, row);
      switch (actor.mID)
      {
        case ActorID::META_Appear_only_in_med_hard_difficulty:
          applyDifficultyMarker(Difficulty::Medium);
          break;

        case ActorID::META_Appear_only_in_hard_difficulty:
          applyDifficultyMarker(Difficulty::Hard);
          break;

        case ActorID::META_Dynamic_geometry_marker_1:
        case ActorID::META_Dynamic_geometry_marker_2:
          // stray tile section marker, ignore
          break;

        case ActorID::Dynamic_geometry_1:
        case ActorID::Dynamic_geometry_2:
        case ActorID::Dynamic_geometry_3:
        case ActorID::Dynamic_geometry_4:
        case ActorID::Dynamic_geometry_5:
        case ActorID::Dynamic_geometry_6:
        case ActorID::Dynamic_geometry_7:
        case ActorID::Dynamic_geometry_8:
          {
            auto tileSection = grid.findTileSectionRect(col, row);
            if (tileSection)
            {
              actors.emplace_back(
                LevelData::Actor{actor.mPosition, actor.mID, tileSection});
            }
          }
          break;

        case ActorID::Duke_LEFT:
        case ActorID::Duke_RIGHT:
          playerSpawnPosition = actor.mPosition;
          playerFacingLeft = actor.mID == ActorID::Duke_LEFT;
          break;

        default:
          actors.emplace_back(
            LevelData::Actor{actor.mPosition, actor.mID, std::nullopt});
          break;
      }

      grid.removeActorAt(col, row);
    }
  }

  return {std::move(actors), playerSpawnPosition, playerFacingLeft};
}


void sortByDrawIndex(ActorList& actors, const ResourceLoader& resources)
{
  std::stable_sort(
    std::begin(actors),
    std::end(actors),
    [&](const LevelData::Actor& lhs, const LevelData::Actor& rhs) {
      return resources.drawIndexFor(lhs.mID) < resources.drawIndexFor(rhs.mID);
    });
}

} // namespace


LevelData loadLevel(
  std::string_view mapName,
  const ResourceLoader& resources,
  const Difficulty chosenDifficulty)
{
  const auto levelData = resources.file(mapName);
  LeStreamReader levelReader(levelData);

  LevelHeader header(levelReader);
  ActorList actors;
  for (size_t i = 0; i < header.numActorWords / 3u; ++i)
  {
    const auto type = levelReader.readU16();
    const base::Vec2 position{levelReader.readU16(), levelReader.readU16()};
    if (isValidActorId(type))
    {
      actors.emplace_back(
        LevelData::Actor{position, static_cast<ActorID>(type), std::nullopt});
    }
  }

  auto tileSet = resources.loadCZone(header.CZone);

  const auto width = static_cast<int>(levelReader.readU16());

  if (!isValidWidth(width))
  {
    throw std::runtime_error(
      "Level file has invalid width: " + std::to_string(width));
  }

  const auto height = static_cast<int>(GameTraits::mapHeightForWidth(width));
  data::map::Map map(width, height, std::move(tileSet.mAttributes));

  const auto maskedTileOffsets = readExtraMaskedTileBits(levelReader);
  auto lookupExtraMaskedTileBits =
    [&maskedTileOffsets, width, height](const int x, const int y) {
      const auto index = x / 4 + y * (width / 4);
      const auto extraBitPack = maskedTileOffsets.at(index);

      const auto indexInPack = x % 4;
      const auto bitMask = 0x03 << indexInPack * 2;
      const auto extraBits = (extraBitPack & bitMask) >> indexInPack * 2;

      return static_cast<uint8_t>(extraBits << 5);
    };

  LeStreamReader tileDataReader(
    levelReader.currentIter(),
    levelReader.currentIter() + width * height * sizeof(uint16_t));
  for (int y = 0; y < height; ++y)
  {
    for (int x = 0; x < width; ++x)
    {
      const auto tileSpec = tileDataReader.readU16();

      if (tileSpec & 0x8000)
      {
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
      }
      else
      {
        const auto index = convertTileIndex(tileSpec);
        map.setTileAt(0, x, y, index);
      }
    }
  }

  auto scrollMode = BackdropScrollMode::None;
  if (header.flagBitSet(0x1))
  {
    scrollMode = BackdropScrollMode::ParallaxBoth;
  }
  else if (header.flagBitSet(0x2))
  {
    scrollMode = BackdropScrollMode::ParallaxHorizontal;
  }
  else if (header.flagBitSet(0x8))
  {
    scrollMode = BackdropScrollMode::AutoHorizontal;
  }
  else if (header.flagBitSet(0x10))
  {
    scrollMode = BackdropScrollMode::AutoVertical;
  }
  auto backdropSwitchCondition = BackdropSwitchCondition::None;
  if (
    scrollMode != BackdropScrollMode::AutoHorizontal &&
    scrollMode != BackdropScrollMode::AutoVertical)
  {
    if (header.flagBitSet(0x40))
    {
      backdropSwitchCondition = BackdropSwitchCondition::OnReactorDestruction;
    }
    else if (header.flagBitSet(0x80))
    {
      backdropSwitchCondition = BackdropSwitchCondition::OnTeleportation;
    }
  }

  auto backdropImage = resources.loadBackdrop(header.backdrop);
  std::optional<data::Image> alternativeBackdropImage;
  if (header.flagBitSet(0x40) || header.flagBitSet(0x80))
  {
    alternativeBackdropImage = resources.loadBackdrop(
      backdropNameFromNumber(header.alternativeBackdropNumber));
  }

  auto [actorDescriptions, playerSpawnPosition, playerFacingLeft] =
    preProcessActorDescriptions(map, actors, chosenDifficulty);
  sortByDrawIndex(actorDescriptions, resources);

  return LevelData{
    std::move(tileSet.mTiles),
    std::move(backdropImage),
    std::move(alternativeBackdropImage),
    std::move(map),
    std::move(actorDescriptions),
    playerSpawnPosition,
    playerFacingLeft,
    scrollMode,
    backdropSwitchCondition,
    header.flagBitSet(0x20),
    header.music};
}


} // namespace rigel::assets
