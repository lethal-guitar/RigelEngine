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

#include "enemy_radar.hpp"


namespace ex = entityx;

namespace rigel { namespace game_logic {

RadarDishCounter::RadarDishCounter(
  ex::EntityManager& entities,
  ex::EventManager& events)
{
  events.subscribe<ex::ComponentAddedEvent<components::RadarDish>>(*this);
  events.subscribe<ex::ComponentRemovedEvent<components::RadarDish>>(*this);
}


int RadarDishCounter::numRadarDishes() const {
  return mNumRadarDishes;
}


bool RadarDishCounter::radarDishesPresent() const {
  return mNumRadarDishes != 0;
}


void RadarDishCounter::receive(
  const ex::ComponentAddedEvent<components::RadarDish>& event
) {
  ++mNumRadarDishes;
}


void RadarDishCounter::receive(
  const ex::ComponentRemovedEvent<components::RadarDish>& event
) {
  --mNumRadarDishes;
}

}}
