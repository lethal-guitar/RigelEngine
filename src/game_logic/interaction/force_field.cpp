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

#include "force_field.hpp"

#include "data/player_data.hpp"
#include "engine/base_components.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/player/components.hpp"


namespace ex = entityx;

namespace rigel { namespace game_logic { namespace interaction {

using namespace engine::components;
using namespace game_logic::components;

void configureForceField(entityx::Entity entity) {
  engine::startAnimationLoop(entity, 1, 2, 4);
  entity.assign<BoundingBox>(BoundingBox{{0, -4}, {2, 10}});
  entity.assign<CircuitCardForceField>();
}


void configureKeyCardSlot(
  entityx::Entity entity,
  const BoundingBox& boundingBox
) {
  entity.assign<Interactable>(InteractableType::ForceFieldCardReader);
  entity.assign<AnimationLoop>(1);
  entity.assign<BoundingBox>(boundingBox);
}


bool disableForceField(
  entityx::EntityManager& es,
  entityx::Entity keyCardSlot,
  data::PlayerModel* pPlayerModel
) {
  const auto canDisable =
    pPlayerModel->hasItem(data::InventoryItemType::CircuitBoard);

  if (canDisable) {
    pPlayerModel->removeItem(data::InventoryItemType::CircuitBoard);

    // TODO: Turn off force fields in the order they were placed in the level,
    // to accurately follow the original behavior
    es.each<CircuitCardForceField>(
      [](ex::Entity entity, const CircuitCardForceField&) {
        entity.destroy();
      });

    keyCardSlot.remove<Interactable>();
    keyCardSlot.remove<AnimationLoop>();
    keyCardSlot.remove<BoundingBox>();
  }

  return canDisable;
}

}}}
