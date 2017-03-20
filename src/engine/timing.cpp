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

#include "timing.hpp"

#include <chrono>


namespace rigel { namespace engine {

namespace {

// The original game re-programs the PIT (programmable interrupt timer)
// using 0x10A1 as counter. This gives a tick rate of roughly 280 Hz
// (1193180 / 4257 ~= 280.29).
//
// The game's actual frame rate is derived from that by dividing by 16, which
// gives 17.5 FPS. Note that this is exactly 1/4th of 70 Hz, which was actually
// the usual monitor refresh rate at the time.

// TODO: Change this to times 16, and update all the places that currently wait
// for 2 or 4 ticks to use 1/2 instead.


std::chrono::high_resolution_clock::time_point globalTimeStart;

}


void initGlobalTimer() {
  globalTimeStart = std::chrono::high_resolution_clock::now();
}


TimePoint currentGlobalTime() {
  const auto now = std::chrono::high_resolution_clock::now();
  return std::chrono::duration<double>(now - globalTimeStart).count();
}


static_assert(fastTicksToTime(280) == 1.0, "");
static_assert(fastTicksToTime(280 * 2) == 2.0, "");
static_assert(slowTicksToTime(140) == 1.0, "");
static_assert(slowTicksToTime(70) == 0.5, "");
static_assert(gameFramesToTime(70) == 4.0, "");
static_assert(gameFramesToTime(35) == 2.0, "");

constexpr auto EPSILON = 0.0000001;
static_assert(timeToFastTicks(4.0) - 280.0*4 < EPSILON, "");
static_assert(timeToFastTicks(1.0) - 280.0 < EPSILON, "");
static_assert(timeToSlowTicks(2.0) - 140.0*2 < EPSILON, "");
static_assert(timeToSlowTicks(1.0) - 140.0 < EPSILON, "");
static_assert(timeToGameFrames(4.0) - 70.0 < EPSILON, "");
static_assert(timeToGameFrames(1.0) - 17.5 < EPSILON, "");

}}
