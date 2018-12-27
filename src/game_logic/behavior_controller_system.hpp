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

#pragma once

#include "game_logic/damage_components.hpp"
#include "game_logic/global_dependencies.hpp"

namespace rigel { namespace engine { namespace events {
  struct CollidedWithWorld;
}}}


namespace rigel { namespace game_logic {

class BehaviorControllerSystem :
  public entityx::Receiver<BehaviorControllerSystem> {
public:
  explicit BehaviorControllerSystem(
    GlobalDependencies dependencies,
    Player* pPlayer,
    const base::Vector* pCameraPosition,
    data::map::Map* pMap);

  void update(entityx::EntityManager& es);

  void receive(const events::ShootableDamaged& event);
  void receive(const events::ShootableKilled& event);
  void receive(const engine::events::CollidedWithWorld& event);

private:
  GlobalDependencies mDependencies;
  GlobalState mGlobalState;
};

}}
