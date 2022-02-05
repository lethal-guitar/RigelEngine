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

#include "renderer/texture.hpp"


namespace rigel::engine
{

class TiledTexture
{
public:
  TiledTexture(renderer::Texture&& tileSet, renderer::Renderer* pRenderer);
  TiledTexture(
    renderer::Texture&& tileSet,
    const base::Extents logicalSize,
    renderer::Renderer* pRenderer);

  void renderTileStretched(int index, const base::Rect<int>& destRect) const;

  void renderTile(int index, int posX, int posY) const;
  void renderTile(const int index, const base::Vec2& tlPosition) const
  {
    renderTile(index, tlPosition.x, tlPosition.y);
  }

  renderer::TextureId textureId() const { return mTileSetTexture.data(); }

  renderer::QuadVertices generateVertices(int index, int posX, int posY) const;

  /** Renders the given tile plus the one below it (vertical slice) */
  void renderTileSlice(int baseIndex, const base::Vec2& tlPosition) const;

  /** Renders two adjacent slices (4x4 group of tiles) */
  void renderTileQuad(int baseIndex, const base::Vec2& tlPosition) const;

  /** Renders two adjacent quads (8x4 group of tiles) */
  void renderTileDoubleQuad(int baseIndex, const base::Vec2& tlPosition) const;

  void renderTileAtPixelPos(int index, const base::Vec2& pxPosition) const;

  int tilesPerRow() const;

  bool isHighRes() const;

private:
  void renderTileGroup(
    const int index,
    const int posX,
    const int posY,
    const int tileSpanX,
    const int tileSpanY) const;

  base::Rect<int>
    sourceRect(int index, const int tileSpanX, const int tileSpanY) const;

private:
  renderer::Texture mTileSetTexture;
  renderer::Renderer* mpRenderer;
  int mScaleX = 1;
  int mScaleY = 1;
};

} // namespace rigel::engine
