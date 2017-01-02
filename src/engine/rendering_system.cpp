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


struct RenderingSystem::SpriteData {
  SpriteData(
    const Sprite* pSprite,
    const bool drawTopMost,
    const WorldPosition& position
  )
    : mpSprite(pSprite)
    , mDrawTopMost(drawTopMost)
    , mPosition(position)
  {
  }

  bool operator<(const SpriteData& rhs) const {
    return
      std::tie(mDrawTopMost, mpSprite->mDrawOrder) <
      std::tie(rhs.mDrawTopMost, rhs.mpSprite->mDrawOrder);
  }

  const Sprite* mpSprite;
  bool mDrawTopMost;
  WorldPosition mPosition;
};


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
  std::vector<SpriteData> spritesByDrawOrder;
  es.each<Sprite, WorldPosition>([this, &spritesByDrawOrder](
    ex::Entity entity,
    Sprite& sprite,
    const WorldPosition& pos
  ) {
    const auto drawTopMost = entity.has_component<DrawTopMost>();
    spritesByDrawOrder.emplace_back(&sprite, drawTopMost, pos);
  });
  sort(spritesByDrawOrder);

  // Render
  mMapRenderer.renderBackground(*mpScrollOffset);

  const auto firstTopMostIt = find_if(spritesByDrawOrder,
    [](const auto& data) { return data.mDrawTopMost; });

  // behind foreground
  for (auto it = spritesByDrawOrder.cbegin(); it != firstTopMostIt; ++it) {
    renderSprite(*it);
  }

  mMapRenderer.renderForeground(*mpScrollOffset);

  // top most
  for (auto it = firstTopMostIt; it != spritesByDrawOrder.cend(); ++it) {
    renderSprite(*it);
  }

  mSpritesRendered = spritesByDrawOrder.size();
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


void RenderingSystem::renderSprite(const SpriteData& data) const {
  const auto& pos = data.mPosition;
  const auto& sprite = *data.mpSprite;

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

}}
