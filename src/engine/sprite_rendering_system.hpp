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
#include "engine/base_components.hpp"
#include "renderer/renderer.hpp"
#include "renderer/texture.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <utility>
#include <vector>


namespace rigel::renderer
{
class TextureAtlas;
}


namespace rigel::engine
{

/** Animates sprites with an AnimationLoop component
 *
 * Should be called at game-logic rate. Works on all entities that have a
 * Sprite and an AnimationLoop component. Adjusts the sprite's animation frame
 * based on the animation.
 */
void updateAnimatedSprites(entityx::EntityManager& es);


struct SpriteDrawSpec
{
  base::Rect<int> mDestRect;
  int mImageId;
  bool mIsFlashingWhite;
  bool mIsTranslucent;
};


struct SortableDrawSpec
{
  SpriteDrawSpec mSpec;
  int mDrawOrder;
  bool mDrawTopMost;

  friend bool
    operator<(const SortableDrawSpec& lhs, const SortableDrawSpec& rhs)
  {
    return std::tie(lhs.mDrawTopMost, lhs.mDrawOrder) <
      std::tie(rhs.mDrawTopMost, rhs.mDrawOrder);
  }
};


class SpriteRenderingSystem
{
public:
  SpriteRenderingSystem(
    renderer::Renderer* pRenderer,
    const renderer::TextureAtlas* pTextureAtlas);

  void update(
    entityx::EntityManager& es,
    const base::Extents& viewportSize,
    const base::Vec2& cameraPosition,
    float interpolationFactor);

  void renderRegularSprites() const;
  void renderForegroundSprites() const;

private:
  void renderSprite(const SpriteDrawSpec& spec) const;

  // Temporary storage used for sorting sprites by draw order during sprite
  // collection. Scope-wise, this is only needed during update(), but in order
  // to reduce the number of allocations happening each frame, we reuse the
  // vector.
  std::vector<SortableDrawSpec> mSortBuffer;

  // Data needed to draw sprites that are currently visible. This is updated
  // by each call to update().
  std::vector<SpriteDrawSpec> mSprites;
  std::vector<SpriteDrawSpec>::iterator miForegroundSprites;

  // Dependencies needed for drawing
  renderer::Renderer* mpRenderer;
  const renderer::TextureAtlas* mpTextureAtlas;
};

} // namespace rigel::engine
