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

#include "base/warnings.hpp"
#include "base/spatial_types.hpp"
#include "engine/base_components.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

namespace rigel { namespace data { namespace map { class Map; }}}

namespace rigel { namespace game_logic { namespace components {
  struct PlayerControlled;
}}}


namespace rigel { namespace game_logic {

class MapScrollSystem {
public:
  MapScrollSystem(
    base::Vector* pScrollOffset,
    entityx::Entity player,
    const data::map::Map& map);

  void updateManualScrolling();
  void updateScrollOffset();

private:
  entityx::Entity mPlayer;
  base::Vector* mpScrollOffset;
  base::Extents mMaxScrollOffset;
};

}}
