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

#include "entity_activation_system.hpp"

#include "data/game_traits.hpp"
#include "engine/base_components.hpp"
#include "engine/entity_tools.hpp"
#include "engine/physical_components.hpp"


namespace rigel::engine {

using namespace components;


namespace {

bool determineActiveState(entityx::Entity entity, const bool inActiveRegion) {
  using Policy = ActivationSettings::Policy;

  if (entity.has_component<ActivationSettings>()) {
    auto& settings = *entity.component<ActivationSettings>();

    switch (settings.mPolicy) {
      case Policy::Always:
        return true;

      case Policy::AlwaysAfterFirstActivation:
        if (!settings.mHasBeenActivated && inActiveRegion) {
          settings.mHasBeenActivated = true;
        }

        return settings.mHasBeenActivated;

      default:
        break;
    }
  }

  // Default: Active when on screen
  return inActiveRegion;
}

}


void markActiveEntities(
  entityx::EntityManager& es,
  const base::Vector& cameraPosition
) {
  const BoundingBox activeRegionBox{
    cameraPosition,
    data::GameTraits::mapViewPortSize};

  es.each<WorldPosition, BoundingBox>([&activeRegionBox](
    entityx::Entity entity,
    const WorldPosition& position,
    const BoundingBox& bbox
  ) {
    const auto worldSpaceBbox = toWorldSpace(bbox, position);
    const auto inActiveRegion = worldSpaceBbox.intersects(activeRegionBox);
    const auto active = determineActiveState(entity, inActiveRegion);
    setTag<Active>(entity, active);
    if (active) {
      entity.component<Active>()->mIsOnScreen = inActiveRegion;
    }
  });
}

}
