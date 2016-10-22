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

#include <base/spatial_types.hpp>
#include <data/tile_set.hpp>

#include <boost/optional.hpp>
#include <array>
#include <cstddef>
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
  Map(
    TileSet tileSet,
    Image backdrop,
    int widthInTiles,
    int heightInTiles
  );

  Map(
    TileSet tileSet,
    Image backdrop,
    boost::optional<Image>&& secondaryBackdrop,
    int widthInTiles,
    int heightInTiles
  );

  TileIndex tileAt(int layer, int x, int y) const;

  void setTileAt(int layer, int x, int y, TileIndex index);

  const TileSet& tileSet() const {
    return mTileSet;
  }

  int width() const {
    return static_cast<int>(mWidthInTiles);
  }

  int height() const {
    return static_cast<int>(mHeightInTiles);
  }

  const Image& backdropImage() const {
    return mBackdropImage;
  }

  const boost::optional<Image>& secondaryBackdropImage() const {
    return mSecondaryBackdropImage;
  }

private:
  const TileIndex& tileRefAt(int layer, int x, int y) const;
  TileIndex& tileRefAt(int layer, int x, int y);

private:
  TileSet mTileSet;

  using TileArray = std::vector<TileIndex>;
  std::array<TileArray, 2> mLayers;

  Image mBackdropImage;
  boost::optional<Image> mSecondaryBackdropImage;

  std::size_t mWidthInTiles;
  std::size_t mHeightInTiles;
};


struct LevelData {
  struct Actor {
    base::Vector mPosition;
    ActorID mID;
  };

  data::map::Map mMap;
  std::vector<Actor> mActors;

  BackdropScrollMode mBackdropScrollMode;
  BackdropSwitchCondition mBackdropSwitchCondition;
  bool mEarthquake;
  std::string mMusicFile;
};

}}}
