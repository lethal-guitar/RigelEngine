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

#include "base/boost_variant.hpp" // Required because signals2 includes variant
#include "base/warnings.hpp"
#include "data/map.hpp"

RIGEL_DISABLE_WARNINGS
#include <boost/signals2/signal.hpp>
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

namespace rigel { struct IGameServiceProvider; }

namespace rigel { namespace data { struct PlayerModel; }}


namespace rigel { namespace game_logic {

class DamageInflictionSystem {
public:
  using EntityHitSignal = boost::signals2::signal<void(
    entityx::Entity,
    const base::Point<float>& inflictorVelocity
  )>;

  DamageInflictionSystem(
    data::PlayerModel* pPlayerModel,
    data::map::Map* pMap,
    IGameServiceProvider* pServiceProvider);

  void update(entityx::EntityManager& es);

  EntityHitSignal& entityHitSignal() {
    return mEntityHitSignal;
  }

private:
  data::PlayerModel* mpPlayerModel;
  data::map::Map* mpMap;
  IGameServiceProvider* mpServiceProvider;

  EntityHitSignal mEntityHitSignal;
};

}}
