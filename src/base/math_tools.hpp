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

#include <cmath>
#include <cstdlib>


namespace rigel::base {

template<typename T>
T integerDivCeil(const T value, const T divisor) {
  return (value + divisor - 1) / divisor;
}


template<typename T>
auto lerp(const T a, const T b, const float factor) {
  return a * (1.0f - factor) + b * factor;
}


template<typename T>
bool inRange(const T value, const T min, const T max) {
  return value >= min && value <= max;
}


template<typename U, typename T>
U roundTo(const T value) {
  return static_cast<U>(std::round(value));
}


template<typename T>
int round(const T value) {
  return roundTo<int>(value);
}


inline int sgn(const int value) {
  return value > 0 ? 1 : (value < 0 ? -1 : 0);
}


template<typename T, typename U>
T applyThreshold(const T value, const U threshold)
{
  if (std::abs(value) < threshold) {
    return T{0};
  }

  return value;
}

}
