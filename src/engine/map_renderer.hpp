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
#include "engine/tiled_texture.hpp"
#include "engine/timing.hpp"
#include "loader/level_loader.hpp"
#include "renderer/renderer.hpp"
#include "renderer/texture.hpp"


namespace rigel::engine {

class MapRenderer {
public:
  struct MapRenderData {
    data::Image mTileSetImage;
    data::Image mBackdropImage;
    std::optional<data::Image> mSecondaryBackdropImage;
    data::map::BackdropScrollMode mBackdropScrollMode;
  };

  MapRenderer(
    renderer::Renderer* renderer,
    const data::map::Map* pMap,
    MapRenderData&& renderData);

  void switchBackdrops();

  void renderBackdrop(
    const base::Vector& cameraPosition,
    const base::Extents& viewPortSize) const;
  void renderBackground(
    const base::Vector& sectionStart,
    const base::Extents& sectionSize) const;
  void renderForeground(
    const base::Vector& sectionStart,
    const base::Extents& sectionSize) const;

  void updateAnimatedMapTiles();
  void updateBackdropAutoScrolling(engine::TimeDelta dt);

  void renderSingleTile(
    data::map::TileIndex index,
    const base::Vector& position,
    const base::Vector& cameraPosition) const;

private:
  enum class DrawMode {
    Background,
    Foreground
  };

  void renderMapTiles(
    const base::Vector& sectionStart,
    const base::Extents& sectionSize,
    DrawMode drawMode) const;
  void renderTile(data::map::TileIndex index, int x, int y) const;
  data::map::TileIndex animatedTileIndex(data::map::TileIndex) const;

private:
  mutable renderer::Renderer* mpRenderer;
  const data::map::Map* mpMap;

  TiledTexture mTileSetTexture;
  renderer::Texture mBackdropTexture;
  renderer::Texture mAlternativeBackdropTexture;

  data::map::BackdropScrollMode mScrollMode;

  double mBackdropAutoScrollOffset = 0.0;
  std::uint32_t mElapsedFrames = 0;
};

}
