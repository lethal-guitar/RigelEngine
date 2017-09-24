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
  const SpriteFrame& frame
) {
  const auto dimensionsInTiles = data::pixelExtentsToTileExtents(
    frame.mImage.extents());

  return {frame.mDrawOffset, dimensionsInTiles};
}


inline components::BoundingBox inferBoundingBox(
  const components::Sprite& sprite
) {
  return inferBoundingBox(sprite.mpDrawData->mFrames[0]);
}


inline void synchronizeBoundingBoxToSprite(
  entityx::Entity& entity,
  const int renderSlot = 0
) {
  auto& sprite = *entity.component<components::Sprite>();
  auto& bbox = *entity.component<components::BoundingBox>();

  bbox = inferBoundingBox(
    sprite.mpDrawData->mFrames[sprite.mFramesToRender[renderSlot]]);
}


inline void startAnimationLoop(
  entityx::Entity& entity,
  const int delayInFrames,
  const int startFrame,
  boost::optional<int> endFrame,
  const int renderSlot = 0
) {
  if (entity.has_component<components::AnimationLoop>()) {
    entity.remove<components::AnimationLoop>();
  }

  auto& sprite = *entity.component<components::Sprite>();
  sprite.mFramesToRender[renderSlot] = startFrame;
  entity.assign<components::AnimationLoop>(
    delayInFrames, startFrame, endFrame, renderSlot);
}


inline void startAnimationSequence(
  entityx::Entity& entity,
  const base::ArrayView<int>& frames,
  const int renderSlot = 0
) {
  if (entity.has_component<components::AnimationSequence>()) {
    entity.remove<components::AnimationSequence>();
  }

  auto& sprite = *entity.component<components::Sprite>();
  sprite.mFramesToRender[renderSlot] = frames.front();
  entity.assign<components::AnimationSequence>(frames, renderSlot);
}


}}
