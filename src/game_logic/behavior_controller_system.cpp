/* Copyright (C) 2018, Nikolai Wuttke. All rights reserved.
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

#include "behavior_controller_system.hpp"

#include "engine/base_components.hpp"
#include "game_logic/behavior_controller.hpp"


namespace rigel { namespace game_logic {

BehaviorControllerSystem::BehaviorControllerSystem(
  GlobalDependencies dependencies
)
  : mDependencies(dependencies)
{
}


void BehaviorControllerSystem::update(entityx::EntityManager& es) {
  using engine::components::Active;
  using game_logic::components::BehaviorController;

  es.each<BehaviorController, Active>([this](
    entityx::Entity entity,
    BehaviorController& controller,
    const Active& active
  ) {
    controller.update(
      mDependencies,
      mIsOddFrame,
      active.mIsOnScreen,
      entity);
  });

  mIsOddFrame = !mIsOddFrame;
}

}}
