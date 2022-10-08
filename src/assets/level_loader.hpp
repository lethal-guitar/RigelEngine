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

#include <string>
#include <vector>

#include "assets/actor_image_package.hpp"
#include "base/spatial_types.hpp"
#include "data/game_session_data.hpp"
#include "data/map.hpp"


namespace rigel::assets
{

class ResourceLoader;

std::string levelFileName(const int episode, const int level);

data::map::TileIndex convertTileIndex(const uint16_t rawIndex);

data::map::LevelData loadLevel(
  std::string_view mapName,
  const ResourceLoader& resources,
  data::Difficulty chosenDifficulty);

} // namespace rigel::assets
