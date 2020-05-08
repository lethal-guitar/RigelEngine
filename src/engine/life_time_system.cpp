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

#include "life_time_system.hpp"

#include "engine/physical_components.hpp"


namespace rigel::engine {

void LifeTimeSystem::update(entityx::EntityManager& es) {
  using Condition = components::AutoDestroy::Condition;
  es.each<components::AutoDestroy>([](
    entityx::Entity entity,
    components::AutoDestroy& autoDestroyProperties
  ) {
    auto entityIsOnScreen = [&]() {
      return entity.has_component<components::Active>() &&
        entity.component<components::Active>()->mIsOnScreen;
    };

    const auto flags = autoDestroyProperties.mConditionFlags;

    const auto conditionIsSet = [&flags](const Condition condition) {
      const auto conditionValue = static_cast<int>(condition);
      return (flags & conditionValue) != 0;
    };

    const auto hasTimeout = conditionIsSet(Condition::OnTimeoutElapsed);
    if (hasTimeout) {
      --autoDestroyProperties.mFramesToLive;
    }

    const auto mustDestroy =
      (conditionIsSet(Condition::OnWorldCollision) &&
        entity.has_component<components::CollidedWithWorld>()) ||
      (conditionIsSet(Condition::OnLeavingActiveRegion) &&
        !entityIsOnScreen()) ||
      (hasTimeout && autoDestroyProperties.mFramesToLive < 0);

    if (mustDestroy) {
      entity.destroy();
    }
  });
}

}

