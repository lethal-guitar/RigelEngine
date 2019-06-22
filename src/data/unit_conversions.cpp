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

#include "unit_conversions.hpp"

#include "game_traits.hpp"


namespace rigel::data {

using namespace base;


Vector tileVectorToPixelVector(const Vector& tileVec) {
  return {
    tilesToPixels(tileVec.x),
    tilesToPixels(tileVec.y),
  };
}


Vector pixelVectorToTileVector(const Vector& pixelVector) {
  return {
    pixelsToTiles(pixelVector.x),
    pixelsToTiles(pixelVector.y),
  };
}


Extents tileExtentsToPixelExtents(
  const Extents& tileExtents
) {
  return {
    tilesToPixels(tileExtents.width),
    tilesToPixels(tileExtents.height),
  };
}


Extents pixelExtentsToTileExtents(
  const Extents& pixelExtents
) {
  return {
    pixelsToTiles(pixelExtents.width),
    pixelsToTiles(pixelExtents.height),
  };
}


}
