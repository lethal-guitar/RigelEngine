/* Copyright (C) 2018, Nikolai Wuttke. All rights reserved.
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

#include "base/warnings.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <limits>


namespace rigel::game_logic::components
{

// TODO: Consider finding a better place for this?
struct AppearsOnRadar
{
};

struct ActorTag
{
  static constexpr auto INVALID_SPAWN_INDEX = std::numeric_limits<int>::max();

  enum class Type
  {
    ForceField,
    Door,
    Reactor,
    ActiveElevator,
    WaterArea,
    AnimatedWaterArea,

    CollectableWeapon,
    Merchandise,
    ShootableBonusGlobe,
    ShootableCamera,
    MountedLaserTurret,
    FireBomb,
  };

  explicit ActorTag(const Type type, const int spawnIndex = INVALID_SPAWN_INDEX)
    : mType(type)
    , mSpawnIndex(spawnIndex)
  {
  }

  Type mType;
  int mSpawnIndex = INVALID_SPAWN_INDEX;
};


inline entityx::Entity findFirstMatchInSpawnOrder(
  entityx::EntityManager& es,
  const ActorTag::Type desiredType)
{
  entityx::Entity candidate;
  int candidateIndex = std::numeric_limits<int>::max();

  entityx::ComponentHandle<ActorTag> tag;
  for (auto entity : es.entities_with_components(tag))
  {
    if (tag->mType != desiredType)
    {
      continue;
    }

    if (tag->mSpawnIndex < candidateIndex)
    {
      candidate = entity;
      candidateIndex = tag->mSpawnIndex;
    }
  }

  return candidate;
}

} // namespace rigel::game_logic::components
