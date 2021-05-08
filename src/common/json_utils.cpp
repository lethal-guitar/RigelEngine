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


#include "json_utils.hpp"

#include <stdexcept>


namespace rigel {

namespace {

constexpr auto CANNOT_MERGE_MESSAGE =
  "JSON trees are not structurally equivalent";


bool isArrayOfPrimitives(const nlohmann::json& item) {
  using std::all_of;
  using std::begin;
  using std::end;

// clang-format off
  return
    item.is_array() &&
    all_of(begin(item), end(item), [](const nlohmann::json& element) {
      return !element.is_structured();
    });
// clang-format on
}

}


nlohmann::json merge(nlohmann::json base, nlohmann::json extension) {
  if (!base.is_structured() || isArrayOfPrimitives(base)) {
    return extension;
  }

  if (base.is_object()) {
    if (!extension.is_object()) {
      throw std::invalid_argument(CANNOT_MERGE_MESSAGE);
    }

    // First, make sure both base and extension have the same set of properties.
    // Note that nlohmann::json::insert() does _not_ overwrite properties that
    // already exist in the target object.
    base.insert(extension.begin(), extension.end());
    extension.insert(base.begin(), base.end());

    // Now recursively merge all properties. This will overwrite any primitive
    // values in base with their extension counterparts.
    for (auto& [key, value] : base.items()) {
      value = merge(value, extension[key]);
    }
  } else { // array
    if (!extension.is_array() || base.size() != extension.size()) {
      throw std::invalid_argument(CANNOT_MERGE_MESSAGE);
    }

    auto index = 0u;
    for (auto& value : base) {
      value = merge(value, extension[index]);
      ++index;
    }
  }

  return base;
}

}
