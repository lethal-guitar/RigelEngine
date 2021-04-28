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

namespace rigel
{
struct IGameServiceProvider;
}

namespace rigel::data
{
class PlayerModel;
}


namespace rigel::game_logic
{

class DamageInflictionSystem
{
public:
  DamageInflictionSystem(
    data::PlayerModel* pPlayerModel,
    IGameServiceProvider* pServiceProvider,
    entityx::EventManager* pEvents);

  void update(entityx::EntityManager& es);

private:
  void inflictDamage(
    entityx::Entity inflictorEntity,
    components::DamageInflicting& damage,
    entityx::Entity shootableEntity,
    components::Shootable& shootable);

  data::PlayerModel* mpPlayerModel;
  IGameServiceProvider* mpServiceProvider;
  entityx::EventManager* mpEvents;
};

} // namespace rigel::game_logic
