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
#include "engine/timing.hpp"
#include "loader/level_loader.hpp"


namespace rigel { namespace engine {

class MapRenderer {
public:
  MapRenderer(
    engine::Renderer* renderer,
    const data::map::Map* pMap,
    const data::Image& tileSetImage,
    const data::Image& backdropImage,
    const boost::optional<data::Image>& secondaryBackdropImage,
    data::map::BackdropScrollMode backdropScrollMode);

  void switchBackdrops();

  void renderBackground(const base::Vector& scrollPosition);
  void renderForeground(const base::Vector& scrollPosition);

  void update(engine::TimeDelta dt);

private:
  void renderBackdrop(const base::Vector& scrollPosition);
  void renderMapTiles(
    const base::Vector& scrollPosition,
    bool renderForeground);
  data::map::TileIndex animatedTileIndex(data::map::TileIndex) const;

private:
  engine::Renderer* mpRenderer;
  const data::map::Map* mpMap;

  TileRenderer mTileRenderer;
  engine::OwningTexture mBackdropTexture;
  engine::OwningTexture mAlternativeBackdropTexture;

  data::map::BackdropScrollMode mScrollMode;

  TimeStepper mTimeStepper;
  std::uint32_t mElapsedFrames = 0;
};

}}
