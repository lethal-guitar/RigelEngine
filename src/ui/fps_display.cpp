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

#include "fps_display.hpp"

#include "base/math_tools.hpp"

#include "utils.hpp"

#include <iomanip>
#include <string>
#include <sstream>


namespace rigel::ui {

namespace {

const auto PRE_FILTER_WEIGHT = 0.7f;
const auto FILTER_WEIGHT = 0.9f;

}


void FpsDisplay::updateAndRender(const engine::TimeDelta totalElapsed) {
  mPreFilteredFrameTime = base::lerp(
    static_cast<float>(totalElapsed), mPreFilteredFrameTime, PRE_FILTER_WEIGHT);
  mFilteredFrameTime = base::lerp(
    mPreFilteredFrameTime, mFilteredFrameTime, FILTER_WEIGHT);

  const auto smoothedFps = base::round(1.0f / mFilteredFrameTime);

  std::stringstream statsReport;
  // clang-format off
  statsReport
    << smoothedFps << " FPS, "
    << std::setw(4) << std::fixed << std::setprecision(2)
    << totalElapsed * 1000.0 << " ms";
  // clang-format on

  const auto reportString = statsReport.str();
  drawText(reportString, 0, 0, {255, 255, 255, 255});
}

}
