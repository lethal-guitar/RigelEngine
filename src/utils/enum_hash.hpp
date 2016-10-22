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


/* This is a workaround for libstdc++, which doesn't provide a std::hash for
 * enum class types yet.
 */

#if defined(__GNUC__)

#include <cstddef>


namespace rigel { namespace utils {

template<typename EnumClassT>
std::size_t hashEnumValue(const EnumClassT value) {
  return static_cast<std::size_t>(value);
}

}}


#define RIGEL_PROVIDE_ENUM_CLASS_HASH(EnumClassT) \
  namespace std { \
  template<> \
  struct hash<EnumClassT> { \
    std::size_t operator()(const EnumClassT& value) const { \
      return rigel::utils::hashEnumValue(value); \
    } \
  }; \
  }

#else

#define RIGEL_PROVIDE_ENUM_CLASS_HASH(x)

#endif
