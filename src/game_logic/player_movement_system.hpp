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

#include "base/spatial_types.hpp"
#include "base/grid.hpp"
#include "base/warnings.hpp"
#include "engine/base_components.hpp"
#include "game_logic/player/components.hpp"

RIGEL_DISABLE_WARNINGS
#include <boost/optional.hpp>
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS


namespace rigel { struct IGameServiceProvider; }

namespace rigel { namespace data { namespace map { class Map; }}}

namespace rigel { namespace engine { namespace components {
  struct Physical;
  struct Sprite;
}}}


namespace rigel { namespace game_logic {

void initializePlayerEntity(entityx::Entity player, bool isFacingRight);


/** Takes inputs from player (e.g. keypresses, gamepad etc.) and controls the
 * avatar (Duke) accordingly.
 *
 */
class PlayerMovementSystem : public entityx::System<PlayerMovementSystem> {
public:
  PlayerMovementSystem(
    entityx::Entity player,
    const PlayerInputState* pInputs,
    const data::map::Map& map);

  void update(
    entityx::EntityManager& es,
    entityx::EventManager& events,
    entityx::TimeDelta dt
  ) override;

private:
  boost::optional<base::Vector> findLadderTouchPoint(
    const engine::components::BoundingBox& worldSpacePlayerBounds
  ) const;

  bool canClimbUp(const engine::components::BoundingBox& worldSpacePlayerBounds) const;
  bool canClimbDown(const engine::components::BoundingBox& worldSpacePlayerBounds) const;

private:
  const PlayerInputState* mpPlayerControlInput;
  entityx::Entity mPlayer;
  bool mWalkRequestedLastFrame;

  base::Grid<std::uint8_t> mLadderFlags;
};

}}
