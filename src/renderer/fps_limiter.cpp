/* Copyright (C) 2020, Nikolai Wuttke. All rights reserved.
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

#include "fps_limiter.hpp"

#include <SDL_timer.h>


namespace rigel::renderer {

FpsLimiter::FpsLimiter(const int targetFps)
  : mLastTime(std::chrono::high_resolution_clock::now())
  , mTargetFrameTime(1.0 / targetFps)
{
}


void FpsLimiter::updateAndWait() {
  using namespace std::chrono;

  const auto now = high_resolution_clock::now();
  const auto delta = duration<double>(now - mLastTime).count();
  mLastTime = now;

  mError += mTargetFrameTime - delta;

  const auto timeToWaitFor = mTargetFrameTime + mError;
  if (timeToWaitFor > 0.0) {
    // We use SDL_Delay instead of std::this_thread::sleep_for, because the
    // former is more accurate on some platforms.
    SDL_Delay(static_cast<Uint32>(timeToWaitFor * 1000.0));
  }
}

}
