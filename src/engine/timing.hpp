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

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS


namespace rigel { namespace engine {

using TimeDelta = double;

using TimePoint = double;


constexpr TimeDelta slowTicksToTime(const int ticks) {
  return ticks * (1.0 / 140.0);
}


constexpr double timeToSlowTicks(const TimeDelta time) {
  return time / (1.0 / 140.0);
}


constexpr TimeDelta fastTicksToTime(const int ticks) {
  return ticks * (1.0 / 280.0);
}


constexpr double timeToFastTicks(const TimeDelta time) {
  return time / (1.0 / 280.0);
}


constexpr double timeToGameFrames(const TimeDelta time) {
  return time / (fastTicksToTime(1) * 16.0);
}


constexpr TimeDelta gameFramesToTime(const int frames) {
  return frames * (fastTicksToTime(1) * 16.0);
}


void initGlobalTimer();


TimePoint currentGlobalTime();

}}
