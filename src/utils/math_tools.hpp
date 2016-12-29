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

#include <algorithm>


namespace rigel { namespace utils {

template<typename T>
T clamp(const T value, const T minValue, const T maxValue) {
  return std::min(maxValue, std::max(value, minValue));
}


template<typename T>
T integerDivCeil(const T value, const T divisor) {
  return (value + divisor - 1) / divisor;
}


template<typename T>
bool inRange(const T value, const T min, const T max) {
  return value >= min && value <= max;
}

}}
