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
#include "engine/renderer.hpp"
#include "engine/texture.hpp"
#include "engine/tile_renderer.hpp"
#include "loader/level_loader.hpp"


namespace rigel { namespace engine {

class MapRenderer {
public:
  struct MapRenderData {
    explicit MapRenderData(data::map::LevelData&& loadedLevel)
      : mTileSetImage(std::move(loadedLevel.mTileSetImage))
      , mBackdropImage(std::move(loadedLevel.mBackdropImage))
      , mSecondaryBackdropImage(std::move(loadedLevel.mSecondaryBackdropImage))
      , mBackdropScrollMode(loadedLevel.mBackdropScrollMode)
    {
    }

    data::Image mTileSetImage;
    data::Image mBackdropImage;
    std::optional<data::Image> mSecondaryBackdropImage;
    data::map::BackdropScrollMode mBackdropScrollMode;
  };

  MapRenderer(
    engine::Renderer* renderer,
    const data::map::Map* pMap,
    MapRenderData&& renderData);

  void switchBackdrops();

  void renderBackdrop(const base::Vector& cameraPosition);
  void renderBackground(const base::Vector& cameraPosition);
  void renderForeground(const base::Vector& cameraPosition);

  void updateAnimatedMapTiles();

  void renderSingleTile(
    data::map::TileIndex index,
    const base::Vector& position,
    const base::Vector& cameraPosition);

private:
  void renderMapTiles(
    const base::Vector& cameraPosition,
    bool renderForeground);
  void renderTile(data::map::TileIndex index, int x, int y);
  data::map::TileIndex animatedTileIndex(data::map::TileIndex) const;

private:
  engine::Renderer* mpRenderer;
  const data::map::Map* mpMap;

  TileRenderer mTileRenderer;
  engine::OwningTexture mBackdropTexture;
  engine::OwningTexture mAlternativeBackdropTexture;

  data::map::BackdropScrollMode mScrollMode;

  std::uint32_t mElapsedFrames = 0;
  std::uint32_t mElapsedFrames60Fps = 0;
};

}}
