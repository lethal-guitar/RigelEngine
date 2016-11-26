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
#include "engine/timing.hpp"
#include "engine/visual_components.hpp"
#include "sdl_utils/texture.hpp"

#include <entityx/entityx.h>
#include <cstdint>
#include <utility>
#include <vector>


namespace rigel { namespace engine {

/** Renders the map and in-game sprites, optionally animating them
 *
 * Works on all entities that have a SpriteBundle and WorldPosition component.
 * Performs animation if an Animated component is also present.
 * Also renders the map using a engine::MapRenderer. Map and sprite rendering
 * are handled by the same system so that draw-order can be done properly
 * (e.g. some sprites are rendered behind certain tiles, others before etc.)
 */
class RenderingSystem : public entityx::System<RenderingSystem> {
public:
  template<typename... MapRendererArgTs>
  RenderingSystem(
    const base::Vector* pScrollOffset,
    SDL_Renderer* pRenderer,
    MapRendererArgTs&&... mapRendererArgs
  )
    : mpRenderer(pRenderer)
    , mMapRenderer(
        pRenderer,
        std::forward<MapRendererArgTs>(mapRendererArgs)...)
    , mpScrollOffset(pScrollOffset)
  {
  }

  void update(
    entityx::EntityManager& es,
    entityx::EventManager& events,
    entityx::TimeDelta dt
  ) override;

  void switchBackdrops() {
    mMapRenderer.switchBackdrops();
  }

private:
  void animateSprites(
    entityx::EntityManager& es,
    entityx::EventManager& events,
    entityx::TimeDelta dt);

  void renderSprite(
    const components::Sprite& sprite,
    const components::WorldPosition& pos
  ) const;

private:
  SDL_Renderer* mpRenderer;
  MapRenderer mMapRenderer;
  const base::Vector* mpScrollOffset;
};

}}
