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


namespace rigel
{

/** Merges values from extension into base
 *
 * This function merges the contents of extension into base by overwriting
 * any properties that exist in both objects with the values from extension.
 * Properties that don't exist in extension will be left unchanged in base.
 *
 * base and extension must be structurally equivalent. This means:
 *
 *   * if a property in one of the two is an object or array, it must also be
 *     an object or array in the other JSON tree, respectively.
 *   * if a property is an array, it must have the same number of elements in
 *     both JSON trees.
 */
nlohmann::json merge(nlohmann::json base, nlohmann::json extension);

} // namespace rigel
