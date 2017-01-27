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

#include "engine/timing.hpp"
#include "utility"
#include "utils/container_tools.hpp"

#include <cassert>


namespace rigel { namespace ui {

using engine::fastTicksToTime;


MoviePlayer::MoviePlayer(SDL_Renderer* pRenderer)
  : mpRenderer(pRenderer)
{
}


void MoviePlayer::playMovie(
  const data::Movie& movie,
  const int frameDelayInFastTicks,
  const boost::optional<int>& repetitions,
  FrameCallbackFunc frameCallback
) {
  assert(frameDelayInFastTicks >= 1);

  mBaseImage = engine::OwningTexture(mpRenderer, movie.mBaseImage);
  mAnimationFrames = utils::transformed(movie.mFrames,
    [this](const auto& frame) {
      auto texture =
        engine::OwningTexture(mpRenderer, frame.mReplacementImage);
      return FrameData{
      std::move(texture),
        frame.mStartRow
      };
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
    mBaseImage.render(mpRenderer, 0, 0);
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
    const auto& frameData = mAnimationFrames[mCurrentFrame];
    frameData.mImage.render(mpRenderer, 0, frameData.mStartRow);
  }
}


void MoviePlayer::setFrameDelay(const int fastTicks) {
  mFrameDelay = fastTicksToTime(fastTicks);
}


bool MoviePlayer::hasCompletedPlayback() const {
  return mRemainingRepetitions && *mRemainingRepetitions == 0;
}


void MoviePlayer::invokeFrameCallbackIfPresent(const int frameNumber) {
  if (mFrameCallback) {
    const auto maybeNewFrameDelay = mFrameCallback(frameNumber);

    if (maybeNewFrameDelay) {
      setFrameDelay(*maybeNewFrameDelay);
    }
  }
}

}}
