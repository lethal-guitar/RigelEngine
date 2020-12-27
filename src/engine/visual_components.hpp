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

#include "base/array_view.hpp"
#include "base/spatial_types.hpp"
#include "base/warnings.hpp"
#include "engine/base_components.hpp"
#include "engine/timing.hpp"
#include "renderer/renderer.hpp"
#include "renderer/texture.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <array>
#include <bitset>
#include <optional>
#include <vector>


namespace rigel::renderer { class TextureAtlas; }


namespace rigel::engine {

struct SpriteFrame {
  SpriteFrame() = default;
  SpriteFrame(
    const int imageId,
    const base::Vector drawOffset,
    const base::Extents dimensions
  )
    : mImageId(imageId)
    , mDrawOffset(drawOffset)
    , mDimensions(dimensions)
  {
  }

  int mImageId;
  base::Vector mDrawOffset;
  base::Extents mDimensions;
};


struct SpriteDrawData {
  std::vector<SpriteFrame> mFrames;
  base::ArrayView<int> mVirtualToRealFrameMap;
  std::optional<int> mOrientationOffset;
  int mDrawOrder;
};


constexpr auto NUM_RENDER_SLOTS = 8;
constexpr auto IGNORE_RENDER_SLOT = -1;


struct CustomDrawRequest {
  CustomDrawRequest(const int frame, const base::Vector& screenPosition)
    : mScreenPosition(screenPosition)
    , mFrame(frame)
  {
  }

  base::Vector mScreenPosition;
  int mFrame;
};


int virtualToRealFrame(
  const int virtualFrame,
  const SpriteDrawData& drawData,
  const entityx::Entity entity);

int virtualToRealFrame(
  const int virtualFrame,
  const SpriteDrawData& drawData,
  const std::optional<components::Orientation>& orientation);


namespace components {

struct Sprite {
  struct RenderSlot {
    RenderSlot() = default;
    RenderSlot(const int frame)
      : mFrame(static_cast<std::int8_t>(frame))
    {
      assert(
        frame == IGNORE_RENDER_SLOT ||
        (frame >= 0 && frame < std::numeric_limits<std::int8_t>::max()));
    }

    RenderSlot& operator=(const int frame) {
      assert(
        frame == IGNORE_RENDER_SLOT ||
        (frame >= 0 && frame < std::numeric_limits<std::int8_t>::max()));

      mFrame = static_cast<std::int8_t>(frame);
      return *this;
    }

    operator int() const {
      return mFrame;
    }

    RenderSlot& operator++() {
      ++mFrame;
      return *this;
    }

    RenderSlot operator++(int) {
      auto copy = *this;
      ++mFrame;
      return copy;
    }

    RenderSlot& operator--() {
      --mFrame;
      return *this;
    }

    RenderSlot operator--(int) {
      auto copy = *this;
      --mFrame;
      return copy;
    }

    RenderSlot& operator+=(const int offset) {
      mFrame += static_cast<std::int8_t>(offset);
      return *this;
    }

    RenderSlot& operator-=(const int offset) {
      mFrame -= static_cast<std::int8_t>(offset);
      return *this;
    }

    std::int8_t mFrame = IGNORE_RENDER_SLOT;
  };

  Sprite() = default;
  Sprite(const SpriteDrawData* pDrawData, const std::vector<int>& framesToRender)
    : mpDrawData(pDrawData)
  {
    auto index = 0;
    for (const auto frame : framesToRender) {
      mFramesToRender[index] = frame;
      ++index;
    }
  }

  void flashWhite() {
    mFlashingWhiteStates.set();
  }

  void flashWhite(const int renderSlot) {
    mFlashingWhiteStates.set(renderSlot);
  }

  const SpriteDrawData* mpDrawData = nullptr;
  std::array<RenderSlot, NUM_RENDER_SLOTS> mFramesToRender;
  std::bitset<NUM_RENDER_SLOTS> mFlashingWhiteStates;
  bool mTranslucent = false;
  bool mShow = true;
};


/** Extends Sprite with additional render slots

 * If an entity features this component in addition to a a Sprite component,
 * the frames specified via this component will be rendered in addition to the
 * Sprite's render slots. It's also possible to specify a position offset for
 * each frame.
 */
struct ExtendedFrameList {
  struct RenderSpec {
    int mFrame = 0;
    base::Vector mOffset;
  };

  std::vector<RenderSpec> mFrames;
};


/** Indicates that an entity should always be drawn last
 *
 * An entity marked with this component will always have its Sprite drawn after
 * drawing the world, even if it is placed on top of foreground tiles.
 */
struct DrawTopMost {};


struct OverrideDrawOrder {
  explicit OverrideDrawOrder(const int drawOrder)
    : mDrawOrder(drawOrder)
  {
  }

  int mDrawOrder;
};


struct AnimationLoop {
  AnimationLoop() = default;
  explicit AnimationLoop(
    const int delayInFrames,
    std::optional<int> endFrame = std::nullopt
  )
    : AnimationLoop(delayInFrames, 0, endFrame)
  {
  }

  AnimationLoop(
    const int delayInFrames,
    const int startFrame,
    std::optional<int> endFrame,
    const int renderSlot = 0
  )
    : mDelayInFrames(delayInFrames)
    , mStartFrame(startFrame)
    , mEndFrame(endFrame)
    , mRenderSlot(renderSlot)
  {
  }

  int mDelayInFrames = 0;
  int mFramesElapsed = 0;
  int mStartFrame = 0;
  std::optional<int> mEndFrame;
  int mRenderSlot = 0;
};


struct AnimationSequence {
  explicit AnimationSequence(
    const base::ArrayView<int>& frames,
    const int renderSlot = 0,
    const bool repeat = false)
    : mFrames(frames)
    , mRenderSlot(renderSlot)
    , mRepeat(repeat)
  {
  }

  base::ArrayView<int> mFrames;
  decltype(mFrames)::size_type mCurrentFrame = 0;
  int mRenderSlot = 0;
  bool mRepeat = false;
};

}}
