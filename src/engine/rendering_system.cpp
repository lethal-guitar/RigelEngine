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

#include "data/unit_conversions.hpp"
#include "engine/sprite_tools.hpp"
#include "renderer/texture_atlas.hpp"
#include "renderer/upscaling_utils.hpp"

#include <algorithm>
#include <functional>


namespace ex = entityx;


namespace rigel::engine {

using components::AnimationLoop;
using components::AnimationSequence;
using components::CustomRenderFunc;
using components::DrawTopMost;
using components::Orientation;
using components::Sprite;
using components::WorldPosition;


namespace {

void advanceAnimation(Sprite& sprite, AnimationLoop& animated) {
  const auto numFrames = static_cast<int>(sprite.mpDrawData->mFrames.size());
  const auto endFrame = animated.mEndFrame ? *animated.mEndFrame : numFrames-1;
  assert(endFrame >= 0 && endFrame < numFrames);
  assert(endFrame > animated.mStartFrame);
  //Animations must have at least two frames
  assert(
    animated.mRenderSlot >= 0 &&
    animated.mRenderSlot < int(sprite.mFramesToRender.size()));

  auto newFrameNr = sprite.mFramesToRender[animated.mRenderSlot] + 1;
  if (newFrameNr > endFrame) {
    newFrameNr = animated.mStartFrame;
  }

  assert(newFrameNr >= 0 && newFrameNr < numFrames);
  sprite.mFramesToRender[animated.mRenderSlot] = newFrameNr;
}

}


int virtualToRealFrame(
  const int virtualFrame,
  const SpriteDrawData& drawData,
  const entityx::Entity entity
) {
  const auto orientation = entity.has_component<const Orientation>()
    ? std::make_optional(*entity.component<const Orientation>())
    : std::optional<Orientation>{};

  return virtualToRealFrame(virtualFrame, drawData, orientation);
}


int virtualToRealFrame(
  const int virtualFrame,
  const SpriteDrawData& drawData,
  const std::optional<Orientation>& orientation
) {
  auto realFrame = virtualFrame;
  if (
    drawData.mOrientationOffset &&
    orientation &&
    *orientation == Orientation::Right
  ) {
    realFrame += *drawData.mOrientationOffset;
  }

  if (!drawData.mVirtualToRealFrameMap.empty()) {
    realFrame = drawData.mVirtualToRealFrameMap[realFrame];
  }

  return realFrame;
}


void updateAnimatedSprites(ex::EntityManager& es) {
  es.each<Sprite, AnimationLoop>([](
    ex::Entity entity,
    Sprite& sprite,
    AnimationLoop& animated
  ) {
    ++animated.mFramesElapsed;
    if (animated.mFramesElapsed >= animated.mDelayInFrames) {
      animated.mFramesElapsed = 0;
      advanceAnimation(sprite, animated);

      if (
        entity.has_component<components::BoundingBox>() &&
        animated.mRenderSlot == 0
      ) {
        engine::synchronizeBoundingBoxToSprite(entity);
      }
    }
  });

  es.each<Sprite, AnimationSequence>([](
    ex::Entity entity,
    Sprite& sprite,
    AnimationSequence& sequence
  ) {
    ++sequence.mCurrentFrame;
    if (sequence.mCurrentFrame >= sequence.mFrames.size()) {
      if (sequence.mRepeat) {
        sequence.mCurrentFrame = 0;
      } else {
        entity.remove<AnimationSequence>();
        return;
      }
    }

    sprite.mFramesToRender[sequence.mRenderSlot] =
      sequence.mFrames[sequence.mCurrentFrame];

    if (
      entity.has_component<components::BoundingBox>() &&
      sequence.mRenderSlot == 0
    ) {
      engine::synchronizeBoundingBoxToSprite(entity);
    }
  });

  es.each<Sprite>([](ex::Entity entity, Sprite& sprite) {
     sprite.mFlashingWhiteStates.reset();
  });
}


void drawSpriteFrame(
  const SpriteFrame& frame,
  const base::Vector& position,
  const renderer::TextureAtlas& spritesTextureAtlas
) {
  // World-space tile positions refer to a sprite's bottom left tile,
  // but we need its top left corner for drawing.
  const auto heightTiles = frame.mDimensions.height;
  const auto topLeft = position - base::Vector(0, heightTiles - 1);
  const auto topLeftPx = data::tileVectorToPixelVector(topLeft);
  const auto drawOffsetPx = data::tileVectorToPixelVector(
    frame.mDrawOffset);

  const auto destRect = base::Rect<int>{
    topLeftPx + drawOffsetPx,
    data::tileExtentsToPixelExtents(frame.mDimensions)};
  spritesTextureAtlas.draw(frame.mImageId, destRect);
}


SpriteRenderingSystem::SpriteRenderingSystem(
  renderer::Renderer* pRenderer,
  const renderer::TextureAtlas* pTextureAtlas
)
  : mpRenderer(pRenderer)
  , mpTextureAtlas(pTextureAtlas)
{
}


void SpriteRenderingSystem::update(
  ex::EntityManager& es,
  const base::Extents& viewPortSize
) {
  using std::begin;
  using std::end;
  using std::sort;

  mSpritesByDrawOrder.clear();
  es.each<Sprite, WorldPosition>([&](
    ex::Entity entity,
    Sprite& sprite,
    const WorldPosition& pos
  ) {
    const auto drawTopMost = entity.has_component<DrawTopMost>();
    mSpritesByDrawOrder.emplace_back(entity, &sprite, drawTopMost, pos);
  });
  sort(begin(mSpritesByDrawOrder), end(mSpritesByDrawOrder));

  miForegroundSprites = std::find_if(
    begin(mSpritesByDrawOrder),
    end(mSpritesByDrawOrder),
    std::mem_fn(&SpriteData::mDrawTopMost));
}


void SpriteRenderingSystem::renderRegularSprites(
  const base::Vector& cameraPosition
) const {
  for (auto it = mSpritesByDrawOrder.begin(); it != miForegroundSprites; ++it) {
    renderSprite(*it, cameraPosition);
  }
}


void SpriteRenderingSystem::renderForegroundSprites(
  const base::Vector& cameraPosition
) const {
  for (auto it = miForegroundSprites; it != mSpritesByDrawOrder.end(); ++it) {
    renderSprite(*it, cameraPosition);
  }
}


void SpriteRenderingSystem::renderSprite(
  const SpriteData& data,
  const base::Vector& cameraPosition
) const {
  const auto& pos = data.mPosition;
  const auto& sprite = *data.mpSprite;

  if (!sprite.mShow) {
    return;
  }

  if (data.mEntity.has_component<CustomRenderFunc>()) {
    const auto renderFunc = *data.mEntity.component<const CustomRenderFunc>();
    renderFunc(*mpTextureAtlas, data.mEntity, sprite, pos - cameraPosition);
  } else {
    auto slotIndex = 0;
    for (const auto& baseFrameIndex : sprite.mFramesToRender) {
      assert(baseFrameIndex < int(sprite.mpDrawData->mFrames.size()));

      if (baseFrameIndex == IGNORE_RENDER_SLOT) {
        continue;
      }

      const auto frameIndex = virtualToRealFrame(
        baseFrameIndex, *sprite.mpDrawData, data.mEntity);

      // White flash effect/translucency

      // White flash takes priority over translucency
      if (sprite.mFlashingWhiteStates.test(slotIndex)) {
        mpRenderer->setOverlayColor(base::Color{255, 255, 255, 255});
      } else if (sprite.mTranslucent) {
        mpRenderer->setColorModulation(base::Color{255, 255, 255, 130});
      }

      auto& frame = sprite.mpDrawData->mFrames[frameIndex];

      drawSpriteFrame(frame, pos - cameraPosition, *mpTextureAtlas);

      mpRenderer->setOverlayColor(base::Color{});
      mpRenderer->setColorModulation(base::Color{255, 255, 255, 255});

      ++slotIndex;
    }
  }
}

}
