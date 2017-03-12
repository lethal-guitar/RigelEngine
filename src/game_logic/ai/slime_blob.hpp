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

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

namespace rigel { namespace engine { class RandomNumberGenerator; }}
namespace rigel { namespace game_logic { class EntityFactory; }}


namespace rigel { namespace game_logic { namespace ai {

namespace components {

struct SlimeContainer {
  int mBreakAnimationStep = 0;
};

}


void configureSlimeContainer(entityx::Entity entity);


class SlimeBlobSystem : public entityx::System<SlimeBlobSystem> {
public:
  SlimeBlobSystem(
    entityx::Entity player,
    EntityFactory* pEntityFactory,
    engine::RandomNumberGenerator* pRandomGenerator);

  void update(
    entityx::EntityManager& es,
    entityx::EventManager& events,
    entityx::TimeDelta dt) override;

  void onEntityHit(entityx::Entity entity);

private:
  entityx::Entity mPlayer;
  EntityFactory* mpEntityFactory;
  engine::RandomNumberGenerator* mpRandomGenerator;
};

}}}
