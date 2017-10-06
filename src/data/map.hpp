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
#include "base/warnings.hpp"
#include "data/tile_attributes.hpp"

RIGEL_DISABLE_WARNINGS
#include <boost/optional.hpp>
RIGEL_RESTORE_WARNINGS

#include <array>
#include <cstddef>
#include <string>
#include <vector>


namespace rigel { namespace data {

using ActorID = std::uint16_t;


namespace map {

enum class BackdropScrollMode {
  None,
  ParallaxBoth,
  ParallaxHorizontal,
  AutoHorizontal,
  AutoVertical
};


enum class BackdropSwitchCondition {
  None,
  OnTeleportation,
  OnReactorDestruction
};


class Map {
public:
  Map() = default;
  Map(int widthInTiles, int heightInTiles, TileAttributes attributes);

  TileIndex tileAt(int layer, int x, int y) const;

  void setTileAt(int layer, int x, int y, TileIndex index);

  int width() const {
    return static_cast<int>(mWidthInTiles);
  }

  int height() const {
    return static_cast<int>(mHeightInTiles);
  }

  void clearSection(int x, int y, int width, int height);

  TileAttributes& attributes() {
    return mAttributes;
  }

  const TileAttributes& attributes() const {
    return mAttributes;
  }

  CollisionData collisionData(int x, int y) const;

private:
  const TileIndex& tileRefAt(int layer, int x, int y) const;
  TileIndex& tileRefAt(int layer, int x, int y);

private:
  using TileArray = std::vector<TileIndex>;
  std::array<TileArray, 2> mLayers;

  std::size_t mWidthInTiles;
  std::size_t mHeightInTiles;

  TileAttributes mAttributes;
};


struct LevelData {
  struct Actor {
    base::Vector mPosition;
    ActorID mID;
    boost::optional<base::Rect<int>> mAssignedArea;
  };

  Image mTileSetImage;
  Image mBackdropImage;
  boost::optional<Image> mSecondaryBackdropImage;

  data::map::Map mMap;
  std::vector<Actor> mActors;

  BackdropScrollMode mBackdropScrollMode;
  BackdropSwitchCondition mBackdropSwitchCondition;
  bool mEarthquake;
  std::string mMusicFile;
};


using ActorDescriptionList = std::vector<LevelData::Actor>;

}}}
