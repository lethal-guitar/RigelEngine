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
#include "engine/base_components.hpp"
#include "engine/map_renderer.hpp"
#include "engine/renderer.hpp"
#include "engine/texture.hpp"
#include "engine/timing.hpp"
#include "engine/visual_components.hpp"

#include <entityx/entityx.h>
#include <cstdint>
#include <utility>
#include <vector>


namespace rigel { namespace engine {

/** Animates sprites with an Animated component
 *
 * Should be called at game-logic rate. Works on all entities that have a
 * Sprite and an Animated component. Adjusts the sprite's animation frame
 * based on the animation.
 */
void updateAnimatedSprites(entityx::EntityManager& es);


/** Renders the map and in-game sprites
 *
 * Works on all entities that have a Sprite and WorldPosition component.
 * Also renders the map using a engine::MapRenderer. Map and sprite rendering
 * are handled by the same system so that draw-order can be done properly
 * (e.g. some sprites are rendered behind certain tiles, others before etc.)
 */
class RenderingSystem : public entityx::System<RenderingSystem> {
public:
  template<typename... MapRendererArgTs>
  RenderingSystem(
    const base::Vector* pScrollOffset,
    engine::Renderer* pRenderer,
    MapRendererArgTs&&... mapRendererArgs
  )
    : mpRenderer(pRenderer)
    , mMapRenderer(
        pRenderer,
        std::forward<MapRendererArgTs>(mapRendererArgs)...)
    , mpScrollOffset(pScrollOffset)
  {
  }

  /** Update map tile animation state. Should be called at game-logic rate. */
  void updateAnimatedMapTiles() {
    mMapRenderer.updateAnimatedMapTiles();
  }

  /** Render everything. Can be called at full frame rate. */
  void update(
    entityx::EntityManager& es,
    entityx::EventManager& events,
    entityx::TimeDelta dt
  ) override;

  void switchBackdrops() {
    mMapRenderer.switchBackdrops();
  }

  std::size_t spritesRendered() const {
    return mSpritesRendered;
  }

private:
  struct SpriteData;
  void renderSprite(const SpriteData& data) const;

private:
  engine::Renderer* mpRenderer;
  MapRenderer mMapRenderer;
  const base::Vector* mpScrollOffset;
  std::size_t mSpritesRendered = 0;
};

}}
