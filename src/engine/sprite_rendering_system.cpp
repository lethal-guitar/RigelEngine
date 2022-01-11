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

#include "sprite_rendering_system.hpp"

#include "data/unit_conversions.hpp"
#include "engine/motion_smoothing.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "renderer/texture_atlas.hpp"
#include "renderer/upscaling_utils.hpp"

#include <algorithm>
#include <functional>


namespace ex = entityx;


namespace rigel::engine
{

using components::AnimationLoop;
using components::AnimationSequence;
using components::InterpolateMotion;
using components::Orientation;
using components::Sprite;
using components::WorldPosition;


namespace
{

void advanceAnimation(Sprite& sprite, AnimationLoop& animated)
{
  const auto numFrames = static_cast<int>(sprite.mpDrawData->mFrames.size());
  const auto endFrame =
    animated.mEndFrame ? *animated.mEndFrame : numFrames - 1;
  assert(endFrame >= 0 && endFrame < numFrames);
  assert(endFrame > animated.mStartFrame);
  // Animations must have at least two frames
  assert(
    animated.mRenderSlot >= 0 &&
    animated.mRenderSlot < int(sprite.mFramesToRender.size()));

  auto newFrameNr = sprite.mFramesToRender[animated.mRenderSlot] + 1;
  if (newFrameNr > endFrame)
  {
    newFrameNr = animated.mStartFrame;
  }

  assert(newFrameNr >= 0 && newFrameNr < numFrames);
  sprite.mFramesToRender[animated.mRenderSlot] = newFrameNr;
}


void collectVisibleSprites(
  ex::EntityManager& es,
  const base::Vec2& cameraPosition,
  const base::Extents& viewportSize,
  std::vector<SortableDrawSpec>& output,
  const float interpolationFactor)
{
  using components::BoundingBox;
  using components::DrawTopMost;
  using components::ExtendedFrameList;
  using components::OverrideDrawOrder;

  const auto screenBox = BoundingBox{{}, viewportSize};

  auto submit = [&](
                  const SpriteFrame& frame,
                  const base::Vec2& previousPosition,
                  const base::Vec2& position,
                  const bool flashingWhite,
                  const bool translucent,
                  const bool drawTopmost,
                  const int drawOrder) {
    // World-space tile positions refer to a sprite's bottom left tile,
    // but we need its top left corner for drawing.
    auto drawPosition = [&frame, &cameraPosition](const base::Vec2& pos) {
      const auto heightTiles = frame.mDimensions.height;
      return pos - cameraPosition - base::Vec2(0, heightTiles - 1) +
        frame.mDrawOffset;
    };

    const auto topLeft = drawPosition(position);

    // Discard sprites outside visible area
    const auto frameBox = BoundingBox{topLeft, frame.mDimensions};
    if (!frameBox.intersects(screenBox))
    {
      return;
    }

    const auto previousTopLeft = drawPosition(previousPosition);

    const auto destRect = base::Rect<int>{
      engine::interpolatedPixelPosition(
        previousTopLeft, topLeft, interpolationFactor),
      data::tileExtentsToPixelExtents(frame.mDimensions)};
    const auto drawSpec =
      SpriteDrawSpec{destRect, frame.mImageId, flashingWhite, translucent};

    output.push_back({drawSpec, drawOrder, drawTopmost});
  };


  es.each<Sprite, WorldPosition>(
    [&](
      ex::Entity entity, const Sprite& sprite, const WorldPosition& position) {
      if (!sprite.mShow)
      {
        return;
      }

      const auto& previousPosition = entity.has_component<InterpolateMotion>()
        ? entity.component<InterpolateMotion>()->mPreviousPosition
        : position;

      const auto drawTopmost = entity.has_component<DrawTopMost>();
      const auto drawOrder = entity.has_component<OverrideDrawOrder>()
        ? entity.component<const OverrideDrawOrder>()->mDrawOrder
        : sprite.mpDrawData->mDrawOrder;

      auto slotIndex = 0;
      for (const auto& baseFrameIndex : sprite.mFramesToRender)
      {
        assert(baseFrameIndex < int(sprite.mpDrawData->mFrames.size()));

        if (baseFrameIndex == IGNORE_RENDER_SLOT)
        {
          continue;
        }

        const auto frameIndex =
          virtualToRealFrame(baseFrameIndex, *sprite.mpDrawData, entity);
        submit(
          sprite.mpDrawData->mFrames[frameIndex],
          previousPosition,
          position,
          sprite.mFlashingWhiteStates.test(slotIndex),
          sprite.mTranslucent,
          drawTopmost,
          drawOrder);
        ++slotIndex;
      }

      if (entity.has_component<ExtendedFrameList>())
      {
        const auto& extendedList =
          entity.component<ExtendedFrameList>()->mFrames;
        for (const auto& item : extendedList)
        {
          const auto frameIndex =
            virtualToRealFrame(item.mFrame, *sprite.mpDrawData, entity);
          submit(
            sprite.mpDrawData->mFrames[frameIndex],
            previousPosition + item.mOffset,
            position + item.mOffset,
            false,
            sprite.mTranslucent,
            drawTopmost,
            drawOrder);
        }
      }
    });
}

} // namespace


int virtualToRealFrame(
  const int virtualFrame,
  const SpriteDrawData& drawData,
  const entityx::Entity entity)
{
  const auto orientation = entity.has_component<const Orientation>()
    ? std::make_optional(*entity.component<const Orientation>())
    : std::optional<Orientation>{};

  return virtualToRealFrame(virtualFrame, drawData, orientation);
}


int virtualToRealFrame(
  const int virtualFrame,
  const SpriteDrawData& drawData,
  const std::optional<Orientation>& orientation)
{
  auto realFrame = virtualFrame;
  if (
    drawData.mOrientationOffset && orientation &&
    *orientation == Orientation::Right)
  {
    realFrame += *drawData.mOrientationOffset;
  }

  if (!drawData.mVirtualToRealFrameMap.empty())
  {
    realFrame = drawData.mVirtualToRealFrameMap[realFrame];
  }

  return realFrame;
}


void updateAnimatedSprites(ex::EntityManager& es)
{
  es.each<Sprite, AnimationLoop>(
    [](ex::Entity entity, Sprite& sprite, AnimationLoop& animated) {
      ++animated.mFramesElapsed;
      if (animated.mFramesElapsed >= animated.mDelayInFrames)
      {
        animated.mFramesElapsed = 0;
        advanceAnimation(sprite, animated);

        if (
          entity.has_component<components::BoundingBox>() &&
          animated.mRenderSlot == 0)
        {
          engine::synchronizeBoundingBoxToSprite(entity);
        }
      }
    });

  es.each<Sprite, AnimationSequence>(
    [](ex::Entity entity, Sprite& sprite, AnimationSequence& sequence) {
      ++sequence.mCurrentFrame;
      if (sequence.mCurrentFrame >= sequence.mFrames.size())
      {
        if (sequence.mRepeat)
        {
          sequence.mCurrentFrame = 0;
        }
        else
        {
          entity.remove<AnimationSequence>();
          return;
        }
      }

      sprite.mFramesToRender[sequence.mRenderSlot] =
        sequence.mFrames[sequence.mCurrentFrame];

      if (
        entity.has_component<components::BoundingBox>() &&
        sequence.mRenderSlot == 0)
      {
        engine::synchronizeBoundingBoxToSprite(entity);
      }
    });

  es.each<Sprite>([](ex::Entity entity, Sprite& sprite) {
    sprite.mFlashingWhiteStates.reset();
  });
}


SpriteRenderingSystem::SpriteRenderingSystem(
  renderer::Renderer* pRenderer,
  const renderer::TextureAtlas* pTextureAtlas)
  : mpRenderer(pRenderer)
  , mpTextureAtlas(pTextureAtlas)
{
}


void SpriteRenderingSystem::update(
  ex::EntityManager& es,
  const base::Extents& viewPortSize,
  const base::Vec2& cameraPosition,
  const float interpolationFactor)
{
  using std::back_inserter;
  using std::begin;
  using std::end;

  mSortBuffer.clear();
  collectVisibleSprites(
    es, cameraPosition, viewPortSize, mSortBuffer, interpolationFactor);
  std::sort(begin(mSortBuffer), end(mSortBuffer));

  mSprites.clear();
  mSprites.reserve(mSortBuffer.size());
  std::transform(
    begin(mSortBuffer),
    end(mSortBuffer),
    back_inserter(mSprites),
    [](const SortableDrawSpec& sortableSpec) { return sortableSpec.mSpec; });

  const auto iFirstTopMostSprite = std::find_if(
    begin(mSortBuffer),
    end(mSortBuffer),
    std::mem_fn(&SortableDrawSpec::mDrawTopMost));

  miForegroundSprites = std::next(
    begin(mSprites), std::distance(begin(mSortBuffer), iFirstTopMostSprite));
}


void SpriteRenderingSystem::renderRegularSprites() const
{
  for (auto it = mSprites.begin(); it != miForegroundSprites; ++it)
  {
    renderSprite(*it);
  }
}


void SpriteRenderingSystem::renderForegroundSprites() const
{
  for (auto it = miForegroundSprites; it != mSprites.end(); ++it)
  {
    renderSprite(*it);
  }
}


void SpriteRenderingSystem::renderSprite(const SpriteDrawSpec& spec) const
{
  // White flash takes priority over translucency
  if (spec.mIsFlashingWhite)
  {
    const auto saved = renderer::saveState(mpRenderer);
    mpRenderer->setOverlayColor(base::Color{255, 255, 255, 255});
    mpTextureAtlas->draw(spec.mImageId, spec.mDestRect);
  }
  else if (spec.mIsTranslucent)
  {
    const auto saved = renderer::saveState(mpRenderer);
    mpRenderer->setColorModulation(base::Color{255, 255, 255, 130});
    mpTextureAtlas->draw(spec.mImageId, spec.mDestRect);
  }
  else
  {
    mpTextureAtlas->draw(spec.mImageId, spec.mDestRect);
  }
}

} // namespace rigel::engine
