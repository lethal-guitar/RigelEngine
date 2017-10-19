/* Copyright (C) 2017, Nikolai Wuttke. All rights reserved.
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

#include "dynamic_geometry_system.hpp"

#include "data/map.hpp"
#include "data/sound_ids.hpp"
#include "engine/base_components.hpp"
#include "engine/life_time_components.hpp"
#include "engine/physical_components.hpp"
#include "engine/random_number_generator.hpp"
#include "game_logic/dynamic_geometry_components.hpp"

#include "game_mode.hpp"


namespace rigel { namespace game_logic {

using game_logic::components::MapGeometryLink;

namespace {

const base::Point<float> TILE_DEBRIS_MOVEMENT_SEQUENCE[] = {
  {0.0f, -3.0f},
  {0.0f, -3.0f},
  {0.0f, -2.0f},
  {0.0f, -2.0f},
  {0.0f, -1.0f},
  {0.0f, 0.0f},
  {0.0f, 0.0f},
  {0.0f, 1.0f},
  {0.0f, 2.0f},
  {0.0f, 2.0f},
  {0.0f, 3.0f}
};


void spawnTileDebris(
  entityx::EntityManager& entities,
  const int x,
  const int y,
  const data::map::TileIndex tileIndex,
  const int velocityX,
  const int ySequenceOffset
) {
  using namespace engine::components;
  using namespace engine::components::parameter_aliases;
  using components::TileDebris;

  auto movement = MovementSequence{
    TILE_DEBRIS_MOVEMENT_SEQUENCE,
    ResetAfterSequence{false},
    EnableX{false}};
  movement.mCurrentStep = ySequenceOffset;

  auto debris = entities.create();
  debris.assign<WorldPosition>(x, y);
  debris.assign<BoundingBox>(BoundingBox{{}, {1, 1}});
  debris.assign<Active>();
  debris.assign<ActivationSettings>(ActivationSettings::Policy::Always);
  debris.assign<AutoDestroy>(AutoDestroy::afterTimeout(80));
  debris.assign<TileDebris>(TileDebris{tileIndex});
  debris.assign<MovingBody>(
    Velocity{static_cast<float>(velocityX), 0.0f},
    GravityAffected{false},
    IsPlayer{false},
    IgnoreCollisions{true});
  debris.assign<MovementSequence>(movement);
}


void spawnTileDebrisForSection(
  const engine::components::BoundingBox& mapSection,
  data::map::Map& map,
  entityx::EntityManager& entities,
  engine::RandomNumberGenerator& randomGen
) {
  const auto& start = mapSection.topLeft;
  const auto& size = mapSection.size;
  for (auto y = start.y; y < start.y + size.height; ++y) {
    for (auto x = start.x; x < start.x + size.width; ++x) {
      const auto tileIndex = map.tileAt(0, x, y);
      if (tileIndex == 0) {
        continue;
      }

      const auto velocityX = 3 - randomGen.gen() % 6;
      const auto ySequenceOffset = randomGen.gen() % 5;
      spawnTileDebris(entities, x, y, tileIndex, velocityX, ySequenceOffset);
    }
  }
}

}


DynamicGeometrySystem::DynamicGeometrySystem(
  IGameServiceProvider* pServiceProvider,
  entityx::EntityManager* pEntityManager,
  data::map::Map* pMap,
  engine::RandomNumberGenerator* pRandomGenerator
)
  : mpServiceProvider(pServiceProvider)
  , mpEntityManager(pEntityManager)
  , mpMap(pMap)
  , mpRandomGenerator(pRandomGenerator)
{
}


void DynamicGeometrySystem::onShootableKilled(entityx::Entity entity) {
  // Take care of shootable walls
  if (!entity.has_component<MapGeometryLink>()) {
    return;
  }

  const auto& mapSection =
    entity.component<MapGeometryLink>()->mLinkedGeometrySection;

  spawnTileDebrisForSection(
    mapSection, *mpMap, *mpEntityManager, *mpRandomGenerator);

  mpMap->clearSection(
    mapSection.topLeft.x, mapSection.topLeft.y,
    mapSection.size.width, mapSection.size.height);

  mpServiceProvider->playSound(data::SoundId::BigExplosion);
}

}}
