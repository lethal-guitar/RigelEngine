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

#include "movie_player.hpp"

#include "base/container_utils.hpp"
#include "data/game_traits.hpp"
#include "engine/timing.hpp"
#include "utility"

#include <cassert>


namespace rigel::ui {

using engine::fastTicksToTime;


MoviePlayer::MoviePlayer(renderer::Renderer* pRenderer)
  : mpRenderer(pRenderer)
  , mCanvas(
      pRenderer,
      data::GameTraits::viewPortWidthPx,
      data::GameTraits::viewPortHeightPx)
{
}


void MoviePlayer::playMovie(
  const data::Movie& movie,
  const int frameDelayInFastTicks,
  const std::optional<int>& repetitions,
  FrameCallbackFunc frameCallback
) {
  assert(frameDelayInFastTicks >= 1);

  {
    const auto saved = mCanvas.bindAndReset();

    auto baseImage = renderer::Texture(mpRenderer, movie.mBaseImage);
    baseImage.render(0, 0);
    mpRenderer->submitBatch();
  }

  mAnimationFrames = utils::transformed(movie.mFrames,
    [this](const auto& frame) {
      auto texture =
        renderer::Texture(mpRenderer, frame.mReplacementImage);
      return FrameData{std::move(texture), frame.mStartRow};
    });

  mFrameCallback = std::move(frameCallback);
  mCurrentFrame = 0;
  mRemainingRepetitions = repetitions;
  mFrameDelay = fastTicksToTime(frameDelayInFastTicks);
  mElapsedTime = 0.0;
  mHasShownFirstFrame = false;
}


void MoviePlayer::updateAndRender(const engine::TimeDelta timeDelta) {
  if (hasCompletedPlayback()) {
    return;
  }

  if (!mHasShownFirstFrame) {
    // TODO: Get rid of this special case, it should be possible to handle it
    // as part of the regular update logic
    invokeFrameCallbackIfPresent(0);
    mHasShownFirstFrame = true;
  }

  mElapsedTime += timeDelta;
  const auto elapsedFrames = static_cast<int>(mElapsedTime / mFrameDelay);

  if (elapsedFrames > 0) {
    mElapsedTime -= elapsedFrames * mFrameDelay;
    ++mCurrentFrame;

    if (mRemainingRepetitions) {
      auto& repetitionsRemaining = *mRemainingRepetitions;

      // We render one frame less during the last repetition, since the first
      // (full) image is to be counted as if it was the first frame.
      const auto framesToRenderThisRepetition =
        static_cast<int>(mAnimationFrames.size()) -
          (repetitionsRemaining == 1 ? 1 : 0);

      if (mCurrentFrame >= framesToRenderThisRepetition) {
        mCurrentFrame = 0;
        --repetitionsRemaining;
      }
    } else {
      // Repeat forever
      mCurrentFrame %= mAnimationFrames.size();
    }

    const int frameNrIncludingFirstImage =
      (mCurrentFrame + 1) % mAnimationFrames.size();
    invokeFrameCallbackIfPresent(frameNrIncludingFirstImage);
  }

  {
    const auto saved = mCanvas.bindAndReset();
    const auto& frameData = mAnimationFrames[mCurrentFrame];
    frameData.mImage.render(0, frameData.mStartRow);
  }

  mCanvas.render(0, 0);
}


bool MoviePlayer::hasCompletedPlayback() const {
  return mRemainingRepetitions && *mRemainingRepetitions == 0;
}


void MoviePlayer::invokeFrameCallbackIfPresent(const int frameNumber) {
  if (mFrameCallback) {
    const auto maybeNewFrameDelay = mFrameCallback(frameNumber);

    if (maybeNewFrameDelay) {
      mFrameDelay = fastTicksToTime(*maybeNewFrameDelay);
    }
  }
}

}
