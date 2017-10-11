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
#include "game_logic/player/components.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

namespace rigel { struct IGameServiceProvider; }
namespace rigel { namespace engine { namespace components { struct MovingBody; }}}
namespace rigel { namespace game_logic { namespace components {
  struct PlayerControlled;
}}}


namespace rigel { namespace game_logic { namespace interaction {

namespace components {

struct Elevator {};

}


void configureElevator(entityx::Entity entity);


class ElevatorSystem {
public:
  ElevatorSystem(
    entityx::Entity player,
    IGameServiceProvider* pServiceProvider);

  void update(entityx::EntityManager& es, const PlayerInputState& inputState);

  bool isPlayerAttached() const;

private:
  entityx::Entity findAttachableElevator(entityx::EntityManager& es) const;

  void updateElevatorAttachment(
    entityx::EntityManager& es,
    const game_logic::components::PlayerControlled& playerState);

  void updateElevatorMovement(
    int movement,
    engine::components::MovingBody& playerMovingBody);

private:
  entityx::Entity mPlayer;
  IGameServiceProvider* mpServiceProvider;
  entityx::Entity mAttachedElevator;

  int mPreviousMovement;
  bool mIsOddFrame = false;
};

}}}
