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
namespace rigel::data { class PlayerModel; }
namespace rigel::engine { class RandomNumberGenerator; }
namespace rigel::game_logic { class EntityFactory; }
namespace rigel::game_logic::events {
  struct ShootableDamaged;
  struct ShootableKilled;
}


namespace rigel::game_logic::ai {

namespace components {

struct LaserTurret {
  int mAngle = 0;
  int mSpinningTurnsLeft = 20;
  int mNextShotCountdown = 0;
};

}


void configureLaserTurret(entityx::Entity& entity, int givenScore);


class LaserTurretSystem : public entityx::Receiver<LaserTurretSystem> {
public:
  LaserTurretSystem(
    entityx::Entity player,
    data::PlayerModel* pPlayerModel,
    EntityFactory* pEntityFactory,
    engine::RandomNumberGenerator* pRandomGenerator,
    IGameServiceProvider* pServiceProvider,
    entityx::EventManager& events);

  void receive(const events::ShootableDamaged& event);
  void receive(const events::ShootableKilled& event);

  void update(entityx::EntityManager& es);

private:
  void performBaseHitEffect(entityx::Entity entity);

  entityx::Entity mPlayer;
  data::PlayerModel* mpPlayerModel;
  EntityFactory* mpEntityFactory;
  engine::RandomNumberGenerator* mpRandomGenerator;
  IGameServiceProvider* mpServiceProvider;
};

}
