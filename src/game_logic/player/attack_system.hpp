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
#include "game_logic/entity_factory.hpp"
#include "game_logic/player_movement_system.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <functional>

namespace rigel {
  struct IGameServiceProvider;
}

namespace rigel { namespace data { struct PlayerModel; }}

namespace rigel { namespace game_logic { namespace components {
  struct PlayerControlled;
}}}


namespace rigel { namespace game_logic { namespace player {

class AttackSystem : public entityx::System<AttackSystem> {
public:
  using FireShotFunc = std::function<void(
    ProjectileType type,
    const engine::components::WorldPosition&,
    const base::Point<float>&
  )>;

  AttackSystem(
    entityx::Entity playerEntity,
    data::PlayerModel* pPlayerModel,
    IGameServiceProvider* pServiceProvider,
    FireShotFunc fireShotFunc);

  // TODO: Consider having a special 'updateWithInput' method instead.
  void setInputState(PlayerInputState inputState);

  void update(
    entityx::EntityManager& es,
    entityx::EventManager& events,
    entityx::TimeDelta dt) override;

private:
  void fireShot(
    const engine::components::WorldPosition& playerPosition,
    const components::PlayerControlled& playerState
  );

private:
  entityx::Entity mPlayerEntity;
  data::PlayerModel* mpPlayerModel;
  IGameServiceProvider* mpServiceProvider;
  FireShotFunc mFireShotFunc;
  PlayerInputState mInputState;
};


}}}
