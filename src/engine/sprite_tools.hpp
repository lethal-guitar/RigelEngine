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

#include "base/warnings.hpp"
#include "data/unit_conversions.hpp"
#include "engine/base_components.hpp"
#include "engine/visual_components.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS


namespace rigel { namespace engine {

inline components::BoundingBox inferBoundingBox(
  const components::Sprite& sprite
) {
  const auto& frame = sprite.mpDrawData->mFrames[0];
  const auto dimensionsInTiles = data::pixelExtentsToTileExtents(
    frame.mImage.extents());

  return {frame.mDrawOffset, dimensionsInTiles};
}


inline void synchronizeBoundingBoxToSprite(entityx::Entity& entity) {
  auto& sprite = *entity.component<components::Sprite>();
  auto& bbox = *entity.component<components::BoundingBox>();

  bbox = inferBoundingBox(sprite);
}


inline void startAnimation(
  entityx::Entity& entity,
  const int delayInFrames,
  const int startFrame,
  boost::optional<int> endFrame,
  const int renderSlot = 0
) {
  if (entity.has_component<components::Animated>()) {
    entity.remove<components::Animated>();
  }

  auto& sprite = *entity.component<components::Sprite>();
  sprite.mFramesToRender[renderSlot] = startFrame;
  entity.assign<components::Animated>(
    delayInFrames, startFrame, endFrame, renderSlot);
}

}}
