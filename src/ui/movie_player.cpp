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


namespace rigel::ui
{

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
  FrameCallbackFunc frameCallback)
{
  assert(frameDelayInFastTicks >= 1);

  {
    const auto saved = mCanvas.bindAndReset();

    auto baseImage = renderer::Texture(mpRenderer, movie.mBaseImage);
    baseImage.render(0, 0);
  }

  mAnimationFrames =
    utils::transformed(movie.mFrames, [this](const auto& frame) {
      auto texture = renderer::Texture(mpRenderer, frame.mReplacementImage);
      return FrameData{std::move(texture), frame.mStartRow};
    });

  mFrameCallback = std::move(frameCallback);
  mCurrentFrame = 0;
  mRemainingRepetitions = repetitions;
  mFrameDelay = fastTicksToTime(frameDelayInFastTicks);
  mElapsedTime = 0.0;
  mHasShownFirstFrame = false;
}


void MoviePlayer::updateAndRender(const engine::TimeDelta timeDelta)
{
  if (hasCompletedPlayback())
  {
    return;
  }

  if (!mHasShownFirstFrame)
  {
    // TODO: Get rid of this special case, it should be possible to handle it
    // as part of the regular update logic
    invokeFrameCallbackIfPresent(0);
    mHasShownFirstFrame = true;
  }


  auto invokeCallback = [&]() {
    const int frameNrIncludingFirstImage =
      (mCurrentFrame + 1) % mAnimationFrames.size();
    invokeFrameCallbackIfPresent(frameNrIncludingFirstImage);
  };

  mElapsedTime += timeDelta;
  const auto elapsedFrames = static_cast<int>(mElapsedTime / mFrameDelay);

  if (elapsedFrames > 0)
  {
    mElapsedTime -= elapsedFrames * mFrameDelay;

    if (mRemainingRepetitions)
    {
      auto& repetitionsRemaining = *mRemainingRepetitions;
      const auto isLastRepetition = repetitionsRemaining == 1;

      // We render one frame less during the last repetition, since the first
      // (full) image is to be counted as if it was the first frame.
      const auto framesToRenderThisRepetition =
        static_cast<int>(mAnimationFrames.size()) - (isLastRepetition ? 1 : 0);
      const auto isLastFrame =
        mCurrentFrame + 1 >= framesToRenderThisRepetition;

      if (isLastFrame)
      {
        --repetitionsRemaining;

        // If we are on the last repetition, we keep showing the last frame,
        // otherwise, restart from the beginning.
        if (repetitionsRemaining > 0)
        {
          mCurrentFrame = 0;
        }
      }
      else
      {
        ++mCurrentFrame;
      }

      if (!isLastRepetition || !isLastFrame)
      {
        invokeCallback();
      }
    }
    else
    {
      // Repeat forever
      ++mCurrentFrame;
      mCurrentFrame %= mAnimationFrames.size();
      invokeCallback();
    }
  }

  {
    const auto saved = mCanvas.bindAndReset();
    const auto& frameData = mAnimationFrames[mCurrentFrame];
    frameData.mImage.render(0, frameData.mStartRow);
  }

  mCanvas.render(0, 0);
}


bool MoviePlayer::hasCompletedPlayback() const
{
  return mRemainingRepetitions && *mRemainingRepetitions == 0;
}


void MoviePlayer::invokeFrameCallbackIfPresent(const int frameNumber)
{
  if (mFrameCallback)
  {
    const auto maybeNewFrameDelay = mFrameCallback(frameNumber);

    if (maybeNewFrameDelay)
    {
      mFrameDelay = fastTicksToTime(*maybeNewFrameDelay);
    }
  }
}

} // namespace rigel::ui
