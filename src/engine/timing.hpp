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

namespace detail {

// The original game re-programs the PIT (programmable interrupt timer)
// using 0x10A1 as counter. This gives a tick rate of roughly 280 Hz
// (1193180 / 4257 ~= 280.29).
//
// The game's actual frame rate is derived from that by dividing by 16, which
// gives 17.5 FPS. Note that this is exactly 1/4th of 70 Hz, which was actually
// the usual monitor refresh rate at the time.

constexpr auto FAST_TICK_RATE = 280.0;
constexpr auto SLOW_TICK_RATE = FAST_TICK_RATE / 2.0;
constexpr auto GAME_FRAME_RATE = FAST_TICK_RATE / 16.0;

}


using TimeDelta = double;
using TimePoint = double;


constexpr TimeDelta slowTicksToTime(const int ticks) {
  return ticks / detail::SLOW_TICK_RATE;
}


constexpr double timeToSlowTicks(const TimeDelta time) {
  return time * detail::SLOW_TICK_RATE;
}


constexpr TimeDelta fastTicksToTime(const int ticks) {
  return ticks / detail::FAST_TICK_RATE;
}


constexpr double timeToFastTicks(const TimeDelta time) {
  return time * detail::FAST_TICK_RATE;
}


constexpr TimeDelta gameFramesToTime(const int frames) {
  return frames / detail::GAME_FRAME_RATE;
}


constexpr double timeToGameFrames(const TimeDelta time) {
  return time * detail::GAME_FRAME_RATE;
}


void initGlobalTimer();

TimePoint currentGlobalTime();

}}
