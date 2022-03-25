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

#include "assets/level_loader.hpp"
#include "base/spatial_types.hpp"
#include "engine/tiled_texture.hpp"
#include "engine/timing.hpp"
#include "renderer/renderer.hpp"
#include "renderer/texture.hpp"

#include <array>
#include <vector>


namespace rigel::engine
{

using PackedTileData = std::uint32_t;


constexpr auto BLOCK_SIZE = 32;


struct AnimatedTile
{
  base::Vec2 mPosition;
  data::map::TileIndex mIndex;
};


struct TileBlock
{
  renderer::VertexBufferId mTilesBuffer;
  std::vector<AnimatedTile> mAnimatedTiles;
};


struct TileRenderData
{
  TileRenderData(base::Extents size, renderer::Renderer* pRenderer);
  ~TileRenderData();

  TileRenderData(TileRenderData&&) noexcept = default;
  TileRenderData& operator=(TileRenderData&&) noexcept = default;
  TileRenderData(const TileRenderData&) = delete;
  TileRenderData& operator=(const TileRenderData&) = delete;

  std::array<std::vector<TileBlock>, 2> mLayers;
  base::Extents mSize;
  renderer::Renderer* mpRenderer;
};


/** Grab a copy of map data for use with renderCachedSection
 *
 * It's the client's responsibility to separately keep track of the
 * width of the section, since that's needed by renderCachedSection.
 */
std::vector<PackedTileData>
  copyMapData(const base::Rect<int>& section, const data::map::Map& map);


class MapRenderer
{
public:
  // The enum values also serve as array indices into the layers in
  // TileRenderData.
  enum class DrawMode : std::uint8_t
  {
    Background = 0,
    Foreground = 1
  };

  struct MapRenderData
  {
    data::Image mTileSetImage;
    data::Image mBackdropImage;
    std::optional<data::Image> mSecondaryBackdropImage;
    data::map::BackdropScrollMode mBackdropScrollMode;
  };

  MapRenderer(
    renderer::Renderer* renderer,
    const data::map::Map& map,
    const data::map::TileAttributeDict* pTileAttributes,
    MapRenderData&& renderData);

  void synchronizeTo(const MapRenderer& other);

  bool hasHighResReplacements() const;

  void switchBackdrops();

  void renderBackdrop(
    const base::Vec2f& cameraPosition,
    const base::Extents& viewportSize) const;
  void renderBackground(
    const base::Vec2& sectionStart,
    const base::Extents& sectionSize) const;
  void renderForeground(
    const base::Vec2& sectionStart,
    const base::Extents& sectionSize) const;

  void updateAnimatedMapTiles();
  void updateBackdropAutoScrolling(engine::TimeDelta dt);

  void renderSingleTile(
    data::map::TileIndex index,
    const base::Vec2& pixelPosition) const;
  void renderDynamicSection(
    const data::map::Map& map,
    const base::Rect<int>& coordinates,
    const base::Vec2& pixelPosition,
    const DrawMode drawMode) const;
  void renderCachedSection(
    const base::Vec2& pixelPosition,
    base::ArrayView<PackedTileData> data,
    int width,
    const DrawMode drawMode) const;

private:
  renderer::TexCoords calculateBackdropTexCoords(
    const base::Vec2f& cameraPosition,
    const base::Extents& viewportSize) const;

  void renderMapTiles(
    const base::Vec2& sectionStart,
    const base::Extents& sectionSize,
    DrawMode drawMode) const;
  data::map::TileIndex animatedTileIndex(data::map::TileIndex) const;

private:
  renderer::Renderer* mpRenderer;
  const data::map::TileAttributeDict* mpTileAttributes;

  TiledTexture mTileSetTexture;
  renderer::Texture mBackdropTexture;
  renderer::Texture mAlternativeBackdropTexture;

  TileRenderData mRenderData;

  data::map::BackdropScrollMode mScrollMode;

  float mBackdropAutoScrollOffset = 0.0f;
  std::uint32_t mElapsedFrames = 0;
};

} // namespace rigel::engine
