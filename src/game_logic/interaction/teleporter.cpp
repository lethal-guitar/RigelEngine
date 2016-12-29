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

#include "teleporter.hpp"

#include "engine/base_components.hpp"
#include "game_logic/player/components.hpp"

namespace ex = entityx;


namespace rigel { namespace game_logic { namespace interaction {

using engine::components::WorldPosition;


namespace {

const auto PLAYER_OFFSET = 1;


base::Vector findTeleporterTargetPosition(
  ex::EntityManager& es,
  ex::Entity teleporter
) {
  const auto sourceTeleporterPosition =
    *teleporter.component<WorldPosition>();

  base::Vector targetPosition;

  ex::ComponentHandle<components::Interactable> interactable;
  ex::ComponentHandle<WorldPosition> position;

  ex::Entity targetTeleporter;
  for (auto entity : es.entities_with_components(interactable, position)) {
    if (
      interactable->mType == components::InteractableType::Teleporter &&
      *position != sourceTeleporterPosition
    ) {
      targetTeleporter = entity;
    }
  }

  return *targetTeleporter.component<WorldPosition>();
}

}


void teleportPlayer(
  ex::EntityManager& es,
  ex::Entity player,
  ex::Entity teleporter
) {
  const auto targetPosition = findTeleporterTargetPosition(es, teleporter);

  *player.component<WorldPosition>() =
    targetPosition + base::Vector{PLAYER_OFFSET, 0};
}

}}}
