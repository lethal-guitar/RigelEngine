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
#include "data/game_session_data.hpp"
#include "engine/timing.hpp"

#include "game_service_provider.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS


namespace rigel { namespace data { struct PlayerModel; }}


namespace rigel { namespace game_logic { namespace player {

class DamageSystem {
public:
  DamageSystem(
    entityx::Entity player,
    data::PlayerModel* pPlayerModel,
    IGameServiceProvider* pServiceProvider,
    data::Difficulty difficulty);

  void update(entityx::EntityManager& es);

private:
  entityx::Entity mPlayer;
  data::PlayerModel* mpPlayerModel;
  IGameServiceProvider* mpServiceProvider;
  int mNumMercyFrames;
};


}}}
