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
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

namespace rigel { struct IGameServiceProvider; }
namespace rigel { namespace data { struct PlayerModel; }}
namespace rigel { namespace engine { class RandomNumberGenerator; }}


namespace rigel { namespace game_logic { namespace interaction {

void configureForceField(entityx::Entity entity);
void configureKeyCardSlot(
  entityx::Entity entity,
  const engine::components::BoundingBox& boundingBox);

/** Disable force-field if possible
 *
 * To be called when the player tries to interact with a key-card slot.
 * If they have the key-card in their inventory, corresponding force-fields
 * will be disabled, and the card removed from their invenotry.
 * Otherwise, nothing happens.
 *
 * @returns true if force field(s) were disabled, false otherwise (i.e. player
 *   didn't have the key-card).
 */
bool disableForceField(
  entityx::EntityManager& es,
  entityx::Entity keyCardSlot,
  data::PlayerModel* pPlayerModel);


void animateForceFields(
  entityx::EntityManager& es,
  engine::RandomNumberGenerator& randomGenerator,
  IGameServiceProvider& serviceProvider);

}}}
