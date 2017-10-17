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
#include "game_logic/dynamic_geometry_components.hpp"

#include "game_mode.hpp"


namespace rigel { namespace game_logic {

using game_logic::components::MapGeometryLink;

DynamicGeometrySystem::DynamicGeometrySystem(
  IGameServiceProvider* pServiceProvider,
  data::map::Map* pMap
)
  : mpServiceProvider(pServiceProvider)
  , mpMap(pMap)
{
}


void DynamicGeometrySystem::onShootableKilled(entityx::Entity entity) {
  // Take care of shootable walls
  if (!entity.has_component<MapGeometryLink>()) {
    return;
  }

  const auto& mapSection =
    entity.component<MapGeometryLink>()->mLinkedGeometrySection;

  mpMap->clearSection(
    mapSection.topLeft.x, mapSection.topLeft.y,
    mapSection.size.width, mapSection.size.height);

  mpServiceProvider->playSound(data::SoundId::BigExplosion);
}

}}
