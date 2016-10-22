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

#include <stdexcept>
#include <string>


namespace rigel { namespace sdl_utils {

class Error : public std::runtime_error {
public:
  /** Will call SDL_GetError() to determine message */
  Error();

  explicit Error(const std::string& message);
};


template<typename CallableT>
void throwIfFailed(CallableT operation) {
  const auto result = operation();
  if (result < 0) {
    throw Error();
  }
}

template<typename CallableT>
auto throwIfCreationFailed(CallableT operation) {
  auto result = operation();
  if (result == nullptr) {
    throw Error();
  }
  return result;
}

}}
