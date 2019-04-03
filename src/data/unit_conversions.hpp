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
#include "data/game_traits.hpp"


namespace rigel { namespace data {

template<typename T>
constexpr T pixelsToTiles(const T pixels) {
  return pixels / static_cast<T>(GameTraits::tileSize);
}

template<typename T>
constexpr T tilesToPixels(const T tiles) {
  return tiles * static_cast<T>(GameTraits::tileSize);
}


base::Vector tileVectorToPixelVector(const base::Vector& tileVector);
base::Vector pixelVectorToTileVector(const base::Vector& pixelVector);

base::Extents tileExtentsToPixelExtents(
  const base::Extents& tileExtents);
base::Extents pixelExtentsToTileExtents(
  const base::Extents& pixelExtents);

}}
