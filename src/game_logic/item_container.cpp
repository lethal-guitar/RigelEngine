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

#include "item_container.hpp"

#include "engine/base_components.hpp"
#include "engine/life_time_components.hpp"


namespace rigel { namespace game_logic { namespace item_containers {

void onEntityHit(entityx::Entity entity, entityx::EntityManager& es) {
  using namespace engine::components;
  using game_logic::components::ItemContainer;

  if (entity.has_component<ItemContainer>()) {
    const auto& container = *entity.component<const ItemContainer>();

    auto contents = es.create();
    for (auto& component : container.mContainedComponents) {
      component.assignToEntity(contents);
    }

    contents.assign<Active>();
    contents.assign<WorldPosition>(*entity.component<WorldPosition>());
  }
}

}}}
