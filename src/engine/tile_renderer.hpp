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

#include "engine/texture.hpp"


namespace rigel { namespace engine {

class TileRenderer {
public:
  TileRenderer(OwningTexture&& tileSet, Renderer* pRenderer);

  void renderTile(int index, int posX, int posY) const;
  void renderTile(const int index, const base::Vector& tlPosition) const {
    renderTile(index, tlPosition.x, tlPosition.y);
  }

  /** Renders the given tile plus the one below it (vertical slice) */
  void renderTileSlice(int baseIndex, const base::Vector& tlPosition) const;

  /** Renders two adjacent slices (4x4 group of tiles) */
  void renderTileQuad(int baseIndex, const base::Vector& tlPosition) const;

  /** Renders two adjacent quads (8x4 group of tiles) */
  void renderTileDoubleQuad(
    int baseIndex,
    const base::Vector& tlPosition) const;

  void enableBlending(bool enabled);
  void setColorMod(int r, int g, int b);

  int tilesPerRow() const;

private:
  void renderTileGroup(
    const int index,
    const int posX,
    const int posY,
    const int tileSpanX,
    const int tileSpanY
  ) const;

private:
  OwningTexture mTileSetTexture;
  Renderer* mpRenderer;
};

}}
