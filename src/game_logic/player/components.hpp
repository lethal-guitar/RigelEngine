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
#include "engine/base_components.hpp"

RIGEL_DISABLE_WARNINGS
#include <boost/optional.hpp>
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS


namespace rigel { namespace game_logic {

constexpr auto PLAYER_WIDTH = 3;
constexpr auto PLAYER_HEIGHT = 5;
constexpr auto PLAYER_HEIGHT_CROUCHED = 4;

// Interestingly, Duke's head is outside of his hitbox when crouching,
// so shots/enemies hitting Duke's head won't do any damage.
// It's not clear if that's intentional or by accident, but there is some code
// in the original executable to set the hitbox's height to 3 when the player
// is crouching.
constexpr auto PLAYER_HITBOX_HEIGHT_CROUCHED = 3;

constexpr auto DEFAULT_PLAYER_BOUNDS = engine::components::BoundingBox{
  {0, 0},
  {PLAYER_WIDTH, PLAYER_HEIGHT}
};

inline void assignPlayerComponents(
  entityx::Entity entity,
  const engine::components::Orientation orientation
) {
  using namespace engine::components;

  entity.assign<Orientation>(orientation);
  entity.assign<BoundingBox>(DEFAULT_PLAYER_BOUNDS);
}


namespace events {

struct ElevatorAttachmentChanged {
  explicit ElevatorAttachmentChanged(entityx::Entity attachedElevator)
    : mAttachedElevator(attachedElevator)
  {
  }

  entityx::Entity mAttachedElevator;
};

}


namespace components {

enum class InteractableType {
  Teleporter,
  ForceFieldCardReader,
  HintMachine
};


struct Interactable {
  explicit Interactable(const InteractableType type)
    : mType(type)
  {
  }

  InteractableType mType;
};


struct CircuitCardForceField {};

struct RespawnCheckpoint {
  bool mInitialized = false;
  boost::optional<int> mActivationCountdown;
};

}

}}
