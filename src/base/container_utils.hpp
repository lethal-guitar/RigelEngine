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
#include <iterator>
#include <type_traits>
#include <vector>


namespace rigel::utils {

template<typename RangeT, typename Callable>
auto transformed(const RangeT& range, Callable elementTransform) {
  std::vector<std::invoke_result_t<Callable, decltype(*std::begin(range))>>
    result;

  const auto start = std::begin(range);
  const auto end = std::end(range);
  const auto distance = std::distance(start, end);
  if (distance > 0) {
    result.reserve(distance);
    std::transform(start, end, std::back_inserter(result), elementTransform);
  }

  return result;
}


template<typename ContainerT>
void appendTo(ContainerT& first, const ContainerT& second) {
  first.reserve(first.size() + second.size());
  first.insert(first.end(), std::begin(second), std::end(second));
}


template<typename ContainerT>
auto concatenated(ContainerT&& first, const ContainerT& second) {
  ContainerT combined(std::forward<ContainerT>(first));
  appendTo(combined, second);
  return combined;
}


}
