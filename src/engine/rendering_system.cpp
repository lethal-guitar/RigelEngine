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

#include "rendering_system.hpp"

#include "base/warnings.hpp"
#include "data/game_traits.hpp"
#include "data/unit_conversions.hpp"
#include "engine/physics_system.hpp"

RIGEL_DISABLE_WARNINGS
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/sort.hpp>
RIGEL_RESTORE_WARNINGS


namespace ex = entityx;


namespace rigel { namespace engine {

using components::Animated;
using components::DrawTopMost;
using components::Sprite;
using components::WorldPosition;


void RenderingSystem::update(
  ex::EntityManager& es,
  ex::EventManager& events,
  const ex::TimeDelta dt
) {
  using namespace boost::range;

  // Animate sprites and map
  mMapRenderer.update(dt);
  animateSprites(es, events, dt);

  // Collect sprites, then order by draw index
  std::vector<std::pair<const Sprite*, WorldPosition>> spritesByDrawOrder;
  std::vector<std::pair<const Sprite*, WorldPosition>> topMostSprites;
  es.each<Sprite, WorldPosition>(
    [this, &spritesByDrawOrder, &topMostSprites](
      ex::Entity entity,
      Sprite& sprite,
      const WorldPosition& pos
    ) {
      if (!entity.has_component<DrawTopMost>()) {
        spritesByDrawOrder.emplace_back(&sprite, pos);
      } else {
        topMostSprites.emplace_back(&sprite, pos);
      }
    });
  sort(spritesByDrawOrder, [](const auto& lhs, const auto& rhs) {
    return lhs.first->mDrawOrder < rhs.first->mDrawOrder;
  });

  // Render
  mMapRenderer.renderBackground(*mpScrollOffset);
  for (const auto& spriteAndPos : spritesByDrawOrder) {
    renderSprite(*spriteAndPos.first, spriteAndPos.second);
  }
  mMapRenderer.renderForeground(*mpScrollOffset);

  for (const auto& spriteAndPos : topMostSprites) {
    renderSprite(*spriteAndPos.first, spriteAndPos.second);
  }
}


void RenderingSystem::animateSprites(
  ex::EntityManager& es,
  ex::EventManager& events,
  const ex::TimeDelta dt
) {
  es.each<Sprite, Animated>(
    [this, dt](ex::Entity entity, Sprite& sprite, Animated& animated) {
      for (auto& sequence : animated.mSequences) {
        if (!updateAndCheckIfDesiredTicksElapsed(
          sequence.mTimeStepper, sequence.mDelayInTicks, dt)
        ) {
          continue;
        }

        const auto endFrame = sequence.mEndFrame ?
          *sequence.mEndFrame :
          static_cast<int>(sprite.mFrames.size()) - 1;
        assert(endFrame >= 0 && endFrame < int(sprite.mFrames.size()));
        assert(endFrame > sequence.mStartFrame);
          //Animations must have at least two frames
        assert(
          sequence.mRenderSlot >= 0 &&
          sequence.mRenderSlot < int(sprite.mFramesToRender.size()));

        auto newFrameNr = sprite.mFramesToRender[sequence.mRenderSlot] + 1;
        if (newFrameNr > endFrame) {
          newFrameNr = sequence.mStartFrame;
        }

        assert(newFrameNr >= 0 && newFrameNr < int(sprite.mFrames.size()));
        sprite.mFramesToRender[sequence.mRenderSlot] = newFrameNr;
      }
    });
}


void RenderingSystem::renderSprite(
  const Sprite& sprite,
  const WorldPosition& pos
) const {
  if (!sprite.mShow) {
    return;
  }

  for (const auto frameIndex : sprite.mFramesToRender) {
    assert(frameIndex < int(sprite.mFrames.size()));
    auto& frame = sprite.mFrames[frameIndex];

    // World-space tile positions refer to a sprite's bottom left tile,
    // but we need its top left corner for drawing.
    const auto spriteHeightTiles = data::pixelsToTiles(frame.mImage.height());
    const auto topLeft = pos - base::Vector(0, spriteHeightTiles - 1);
    const auto topLeftPx = data::tileVectorToPixelVector(topLeft);
    const auto drawOffsetPx = data::tileVectorToPixelVector(
      frame.mDrawOffset);

    const auto worldToScreenPx = data::tileVectorToPixelVector(*mpScrollOffset);
    const auto screenPositionPx = topLeftPx + drawOffsetPx - worldToScreenPx;
    frame.mImage.render(mpRenderer, screenPositionPx);
  }
}

// DEBUGGING collision boxes
/*
auto physical = entity.component<components::Physical>();
if (physical) {
  const auto tileSize = data::GameTraits::tileSize;
  SDL_SetRenderDrawColor(mpRenderer, 255, 0, 0, 200);
  SDL_Rect bboxRect{
    pos.x*tileSize + tileSize*physical->mCollisionRect.topLeft.x - worldToScreenPx.x,
    pos.y*tileSize + (tileSize*physical->mCollisionRect.topLeft.y - tileSize*(physical->mCollisionRect.size.height-1)) - worldToScreenPx.y,
    physical->mCollisionRect.size.width*tileSize,
    physical->mCollisionRect.size.height*tileSize
  };
  SDL_RenderDrawRect(mpRenderer, &bboxRect);
}
*/

}}
