/* Copyright (C) 2017, Nikolai Wuttke. All rights reserved.
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

#include "base/spatial_types.hpp"
#include "base/warnings.hpp"
#include "engine/physical_components.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

namespace rigel { struct IGameServiceProvider; }
namespace rigel { namespace engine { class CollisionChecker; }}
namespace rigel { namespace game_logic { namespace events {
  struct ShootableDamaged;
}}}


namespace rigel { namespace game_logic { namespace ai {

namespace components {

struct SpikeBall {
  int mJumpBackCooldown = 0;
};

}


void configureSpikeBall(entityx::Entity entity);


class SpikeBallSystem : public entityx::Receiver<SpikeBallSystem> {
public:
  SpikeBallSystem(
    const engine::CollisionChecker* pCollisionChecker,
    IGameServiceProvider* pServiceProvider,
    entityx::EventManager& events);

  void update(entityx::EntityManager& es);

  void receive(const events::ShootableDamaged& event);
  void receive(const engine::events::CollidedWithWorld& event);

private:
  void jump(entityx::Entity entity);

  const engine::CollisionChecker* mpCollisionChecker;
  IGameServiceProvider* mpServiceProvider;
};

}}}
