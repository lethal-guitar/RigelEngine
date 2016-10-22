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

#include "game_traits.hpp"

#include <base/warnings.hpp>


namespace rigel { namespace data {

RIGEL_DISABLE_GLOBAL_CTORS_WARNING

const base::Vector GameTraits::inGameViewPortOffset(8, 8);

const base::Extents GameTraits::inGameViewPortSize(
  GameTraits::viewPortWidthPx - 16,
  GameTraits::viewPortHeightPx - 8);

const base::Extents GameTraits::mapViewPortSize(
  GameTraits::mapViewPortWidthTiles,
  GameTraits::mapViewPortHeightTiles);

RIGEL_RESTORE_WARNINGS

}}
