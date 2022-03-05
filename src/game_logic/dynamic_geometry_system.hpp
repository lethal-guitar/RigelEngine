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

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

namespace rigel
{
struct IGameServiceProvider;
namespace data::map
{
class Map;
}
namespace engine
{
class RandomNumberGenerator;
}
namespace events
{
struct DoorOpened;
struct MissileDetonated;
struct TileBurnedAway;
} // namespace events
namespace game_logic::events
{
struct ShootableKilled;
}
} // namespace rigel


namespace rigel::game_logic
{

class DynamicGeometrySystem : public entityx::Receiver<DynamicGeometrySystem>
{
public:
  DynamicGeometrySystem(
    IGameServiceProvider* pServiceProvider,
    entityx::EntityManager* pEntityManager,
    data::map::Map* pMap,
    engine::RandomNumberGenerator* pRandomGenerator,
    entityx::EventManager* pEvents);

  void receive(const events::ShootableKilled& event);
  void receive(const rigel::events::DoorOpened& event);
  void receive(const rigel::events::MissileDetonated& event);
  void receive(const rigel::events::TileBurnedAway& event);

private:
  IGameServiceProvider* mpServiceProvider;
  entityx::EntityManager* mpEntityManager;
  data::map::Map* mpMap;
  engine::RandomNumberGenerator* mpRandomGenerator;
  entityx::EventManager* mpEvents;
};

} // namespace rigel::game_logic
