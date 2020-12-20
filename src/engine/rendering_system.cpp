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

#include "data/game_traits.hpp"
#include "data/unit_conversions.hpp"
#include "engine/physics_system.hpp"
#include "engine/sprite_tools.hpp"
#include "game_logic/actor_tag.hpp"
#include "game_logic/dynamic_geometry_components.hpp"
#include "renderer/texture_atlas.hpp"

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


struct WaterEffectArea {
  base::Rect<int> mArea;
  bool mIsAnimated;
};


std::vector<WaterEffectArea> collectWaterEffectAreas(
  entityx::EntityManager& es,
  const base::Vector& cameraPosition,
  const base::Extents& viewPortSize
) {
  using engine::components::BoundingBox;
  using game_logic::components::ActorTag;

  std::vector<WaterEffectArea> result;

  const auto screenBox = BoundingBox{cameraPosition, viewPortSize};

  es.each<ActorTag, WorldPosition, BoundingBox>(
    [&](
      entityx::Entity,
      const ActorTag& tag,
      const WorldPosition& position,
      const BoundingBox& bbox
    ) {
      using T = ActorTag::Type;

      const auto isWaterArea =
        tag.mType == T::AnimatedWaterArea || tag.mType == T::WaterArea;
      if (isWaterArea) {
        const auto worldSpaceBbox = engine::toWorldSpace(bbox, position);

        if (screenBox.intersects(worldSpaceBbox)) {
          const auto topLeftPx =
            data::tileVectorToPixelVector(
              worldSpaceBbox.topLeft - cameraPosition);
          const auto sizePx =
            data::tileExtentsToPixelExtents(worldSpaceBbox.size);
          const auto hasAnimatedSurface = tag.mType == T::AnimatedWaterArea;

          result.push_back(
            WaterEffectArea{{topLeftPx, sizePx}, hasAnimatedSurface});
        }
      }
    });

  return result;
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


struct RenderingSystem::SpriteData {
  SpriteData(
    const ex::Entity entity,
    const Sprite* pSprite,
    const bool drawTopMost,
    const WorldPosition& position
  )
    : mEntity(entity)
    , mPosition(position)
    , mpSprite(pSprite)
    , mDrawOrder(
        entity.has_component<components::OverrideDrawOrder>()
        ? entity.component<const components::OverrideDrawOrder>()->mDrawOrder
        : pSprite->mpDrawData->mDrawOrder)
    , mDrawTopMost(drawTopMost)
  {
  }

  bool operator<(const SpriteData& rhs) const {
    return
      std::tie(mDrawTopMost, mDrawOrder) <
      std::tie(rhs.mDrawTopMost, rhs.mDrawOrder);
  }

  entityx::Entity mEntity;
  WorldPosition mPosition;
  const Sprite* mpSprite;
  int mDrawOrder;
  bool mDrawTopMost;
};


RenderingSystem::RenderingSystem(
  const base::Vector* pCameraPosition,
  renderer::Renderer* pRenderer,
  const renderer::TextureAtlas* pSpritesTextureAtlas,
  const data::map::Map* pMap,
  MapRenderer::MapRenderData&& mapRenderData
)
  : mpRenderer(pRenderer)
  , mpTextureAtlas(pSpritesTextureAtlas)
  , mRenderTarget(
      pRenderer,
      pRenderer->maxWindowSize().width,
      pRenderer->maxWindowSize().height)
  , mMapRenderer(pRenderer, pMap, std::move(mapRenderData))
  , mpCameraPosition(pCameraPosition)
{
}


void RenderingSystem::update(
  ex::EntityManager& es,
  const std::optional<base::Color>& backdropFlashColor,
  const base::Extents& viewPortSize
) {
  using namespace std;
  using game_logic::components::TileDebris;

  // Collect sprites, then order by draw index
  vector<SpriteData> spritesByDrawOrder;
  es.each<Sprite, WorldPosition>([&spritesByDrawOrder](
    ex::Entity entity,
    Sprite& sprite,
    const WorldPosition& pos
  ) {
    const auto drawTopMost = entity.has_component<DrawTopMost>();
    spritesByDrawOrder.emplace_back(entity, &sprite, drawTopMost, pos);
  });
  sort(begin(spritesByDrawOrder), end(spritesByDrawOrder));

  const auto firstTopMostIt = find_if(
    begin(spritesByDrawOrder),
    end(spritesByDrawOrder),
    mem_fn(&SpriteData::mDrawTopMost));

  auto renderBackgroundLayers = [&]() {
    if (backdropFlashColor) {
      mpRenderer->setOverlayColor(*backdropFlashColor);
      mMapRenderer.renderBackdrop(*mpCameraPosition, viewPortSize);
      mpRenderer->setOverlayColor({});
    } else {
      mMapRenderer.renderBackdrop(*mpCameraPosition, viewPortSize);
    }

    mMapRenderer.renderBackground(*mpCameraPosition, viewPortSize);

    for (auto it = spritesByDrawOrder.begin(); it != firstTopMostIt; ++it) {
      renderSprite(*it);
    }
  };


  const auto waterEffectAreas =
    collectWaterEffectAreas(es, *mpCameraPosition, viewPortSize);
  if (waterEffectAreas.empty()) {
    renderBackgroundLayers();
  } else {
    {
      renderer::RenderTargetTexture::Binder bindRenderTarget(mRenderTarget, mpRenderer);
      renderBackgroundLayers();
    }

    {
      auto saved = renderer::Renderer::StateSaver(mpRenderer);
      mpRenderer->setGlobalScale({1.0f, 1.0f});
      mpRenderer->setGlobalTranslation({});
      mRenderTarget.render(mpRenderer, 0, 0);
    }

    for (const auto& area : waterEffectAreas) {
      mpRenderer->drawWaterEffect(
        area.mArea,
        mRenderTarget.data(),
        area.mIsAnimated ? std::optional<int>(mWaterAnimStep) : std::nullopt);
    }
  }

  mMapRenderer.renderForeground(*mpCameraPosition, viewPortSize);

  // top most
  for (auto it = firstTopMostIt; it != spritesByDrawOrder.end(); ++it) {
    renderSprite(*it);
  }

  mSpritesRendered = spritesByDrawOrder.size();

  // tile debris
  es.each<TileDebris, WorldPosition>(
    [this](ex::Entity, const TileDebris& debris, const WorldPosition& pos) {
      mMapRenderer.renderSingleTile(debris.mTileIndex, pos, *mpCameraPosition);
    });
}


void RenderingSystem::renderSprite(const SpriteData& data) const {
  const auto& pos = data.mPosition;
  const auto& sprite = *data.mpSprite;

  if (!sprite.mShow) {
    return;
  }

  if (data.mEntity.has_component<CustomRenderFunc>()) {
    const auto renderFunc = *data.mEntity.component<const CustomRenderFunc>();
    renderFunc(*mpTextureAtlas, data.mEntity, sprite, pos - *mpCameraPosition);
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

      drawSpriteFrame(frame, pos - *mpCameraPosition, *mpTextureAtlas);

      mpRenderer->setOverlayColor(base::Color{});
      mpRenderer->setColorModulation(base::Color{255, 255, 255, 255});

      ++slotIndex;
    }
  }
}

}
