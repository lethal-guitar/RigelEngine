/* Copyright (C) 2019, Nikolai Wuttke. All rights reserved.
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
#include <nlohmann/json.hpp>
RIGEL_RESTORE_WARNINGS

namespace rigel {

inline nlohmann::json merge(nlohmann::json primary, nlohmann::json secondary) {
  if (!primary.is_structured()) {
    return primary;
  }

  if (primary.is_object()) {
    primary.insert(secondary.begin(), secondary.end());

    for (auto& [key, value] : primary.items()) {
      value = merge(secondary[key], value);
    }
  } else { // array
    auto index = 0u;
    for (auto& value : primary) {
      value = merge(secondary[index], value);
      ++index;
    }
  }

  return primary;
}

}
