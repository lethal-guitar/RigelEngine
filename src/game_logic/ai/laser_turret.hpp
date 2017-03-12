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

#include "base/spatial_types.hpp"
#include "base/warnings.hpp"
#include "game_logic/damage_components.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <array>


namespace rigel { struct IGameServiceProvider; }
namespace rigel { namespace data { struct PlayerModel; } }
namespace rigel { namespace game_logic { class EntityFactory; } }


namespace rigel { namespace game_logic { namespace ai {

namespace components {

struct LaserTurret {
  int mAngle = 0;
  int mSpinningTurnsLeft = 20;
  int mNextShotCountdown = 0;
};

}


void configureLaserTurret(entityx::Entity& entity, int givenScore);


class LaserTurretSystem : public entityx::System<LaserTurretSystem> {
public:
  LaserTurretSystem(
    entityx::Entity player,
    data::PlayerModel* pPlayerModel,
    EntityFactory* pEntityFactory,
    IGameServiceProvider* pServiceProvider);

  void onEntityHit(entityx::Entity entity);

  void update(
    entityx::EntityManager& es,
    entityx::EventManager& events,
    entityx::TimeDelta dt) override;

private:
  entityx::Entity mPlayer;
  data::PlayerModel* mpPlayerModel;
  EntityFactory* mpEntityFactory;
  IGameServiceProvider* mpServiceProvider;
};

}}}
