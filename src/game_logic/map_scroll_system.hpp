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
#include "game_logic/input.hpp"
#include "game_logic/player/components.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

namespace rigel { namespace data { namespace map { class Map; }}}


namespace rigel { namespace game_logic {


class Player;

// TODO: Rename to "Camera"
// TODO: This should own the scroll offset
class MapScrollSystem {
public:
  MapScrollSystem(
    base::Vector* pScrollOffset,
    const Player* pPlayer,
    const data::map::Map& map);

  void update(const PlayerInput& input);
  void centerViewOnPlayer();

private:
  void updateManualScrolling(const PlayerInput& input);
  void updateScrollOffset();
  void setPosition(base::Vector position);

  const Player* mpPlayer;
  base::Vector* mpScrollOffset;
  base::Extents mMaxScrollOffset;
};

}}
