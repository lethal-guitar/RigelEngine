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


namespace rigel { namespace engine {

template<typename TagComponent>
void setTag(entityx::Entity entity, const bool assignTag) {
  if (!entity.has_component<TagComponent>() && assignTag) {
    entity.assign<TagComponent>();
  }
  if (entity.has_component<TagComponent>() && !assignTag) {
    entity.remove<TagComponent>();
  }
}


/** Like Entity::assign, but first removes the component if already present. */
template<typename ComponentT, typename... Args>
void reassign(entityx::Entity entity, Args&&... args) {
  if (entity.has_component<ComponentT>()) {
    entity.remove<ComponentT>();
  }

  entity.assign<ComponentT>(std::forward<Args>(args)...);
}


inline bool isOnScreen(const entityx::Entity entity) {
  return
      entity.has_component<components::Active>() &&
      entity.component<const components::Active>()->mIsOnScreen;
}

}}
