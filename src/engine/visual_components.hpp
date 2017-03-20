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
#include "base/spatial_types.hpp"
#include "engine/base_components.hpp"
#include "engine/renderer.hpp"
#include "engine/texture.hpp"
#include "engine/timing.hpp"

RIGEL_DISABLE_WARNINGS
#include <boost/optional.hpp>
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <vector>


namespace rigel { namespace engine { namespace components {

struct SpriteFrame {
  SpriteFrame() = default;
  SpriteFrame(
    engine::NonOwningTexture image,
    base::Vector drawOffset
  )
    : mImage(std::move(image))
    , mDrawOffset(drawOffset)
  {
  }

  engine::NonOwningTexture mImage;
  base::Vector mDrawOffset;
};


struct Sprite {
  std::vector<SpriteFrame> mFrames;
  int mDrawOrder;

  void flashWhite() {
    mFlashWhiteTime = currentGlobalTime();
  }

  std::vector<int> mFramesToRender;
  bool mShow = true;
  engine::TimePoint mFlashWhiteTime = 0.0;
};


/** Specify a custom rendering function for a sprite
 *
 * When a sprite entity also has this component, the provided function
 * pointer will be invoked instead of rendering the sprite directly.
 *
 * The last argument is the sprite's world position converted to a screen-space
 * pixel position.
 */
using CustomRenderFunc = void(*)(
  engine::Renderer*,
  entityx::Entity,
  const engine::components::Sprite&,
  const base::Vector&);


/** Indicates that an entity should always be drawn last
 *
 * An entity marked with this component will alwyas have its Sprite drawn after
 * drawing the world, even if it is placed on top of foreground tiles.
 */
struct DrawTopMost {};


struct Animated {
  Animated() = default;
  explicit Animated(
    const int delayInTicks,
    boost::optional<int> endFrame = boost::none
  )
    : Animated(delayInTicks, 0, endFrame)
  {
  }

  Animated(
    const int delayInTicks,
    const int startFrame,
    boost::optional<int> endFrame,
    const int renderSlot = 0
  )
    : mDelayInTicks(delayInTicks)
    , mStartFrame(startFrame)
    , mEndFrame(std::move(endFrame))
    , mRenderSlot(renderSlot)
  {
  }

  int mDelayInTicks = 0;
  int mStartFrame = 0;
  boost::optional<int> mEndFrame;
  int mRenderSlot = 0;

  TimeStepper mTimeStepper;
};

}}}
