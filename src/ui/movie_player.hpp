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

#include "data/movie.hpp"
#include "engine/timing.hpp"
#include "renderer/texture.hpp"

#include <functional>
#include <optional>
#include <vector>


namespace rigel::ui {

class MoviePlayer {
public:
  /** The frame callback will be invoked each time a new animation frame is
   * shown. If it returns a number, it will be used to set the new frame delay
   * for all subsequent frames.
   */
  using FrameCallbackFunc = std::function<std::optional<int>(int)>;

  explicit MoviePlayer(renderer::Renderer* pRenderer);

  void playMovie(
    const data::Movie& movie,
    int frameDelayInFastTicks,
    const std::optional<int>& repetitions = std::nullopt,
    FrameCallbackFunc frameCallback = nullptr);

  void updateAndRender(engine::TimeDelta timeDelta);
  bool hasCompletedPlayback() const;

private:
  struct FrameData {
    renderer::OwningTexture mImage;
    int mStartRow;
  };

  void invokeFrameCallbackIfPresent(int whichFrame);

private:
  renderer::Renderer* mpRenderer;
  renderer::RenderTargetTexture mCanvas;
  std::vector<FrameData> mAnimationFrames;
  FrameCallbackFunc mFrameCallback = nullptr;

  bool mHasShownFirstFrame = false;
  int mCurrentFrame = 0;
  std::optional<int> mRemainingRepetitions = 0;
  engine::TimeDelta mFrameDelay = 0.0;
  engine::TimeDelta mElapsedTime = 0.0;
};

}
