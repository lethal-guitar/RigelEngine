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

#include <engine/timing.hpp>


using namespace rigel::engine;

static_assert(fastTicksToTime(280) == 1.0);
static_assert(fastTicksToTime(280 * 2) == 2.0);
static_assert(slowTicksToTime(140) == 1.0);
static_assert(slowTicksToTime(70) == 0.5);

constexpr auto EPSILON = 0.0000001;
static_assert(timeToFastTicks(4.0) - 280.0*4 < EPSILON);
static_assert(timeToFastTicks(1.0) - 280.0 < EPSILON);
static_assert(timeToSlowTicks(2.0) - 140.0*2 < EPSILON);
static_assert(timeToSlowTicks(1.0) - 140.0 < EPSILON);
