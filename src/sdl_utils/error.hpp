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


namespace rigel::sdl_utils {

class Error : public std::runtime_error {
public:
  /** Will call SDL_GetError() to determine message */
  Error();

  explicit Error(const std::string& message);
};


inline void check(int result) {
  if (result < 0) {
    throw Error();
  }
}

template <typename ObjectPtr>
auto check(ObjectPtr pObject) {
  if (pObject == nullptr) {
    throw Error();
  }
  return pObject;
}

}
