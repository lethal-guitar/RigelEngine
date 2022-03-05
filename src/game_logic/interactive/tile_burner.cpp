/* Copyright (C) 2019, Nikolai Wuttke. All rights reserved.
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

#include "tile_burner.hpp"

#include "data/map.hpp"
#include "engine/base_components.hpp"
#include "engine/random_number_generator.hpp"
#include "game_logic/behavior_controller.hpp"
#include "game_logic/ientity_factory.hpp"

#include <array>


namespace rigel::game_logic::behaviors
{

namespace
{

constexpr base::Vec2 TILE_BURN_AREA_OFFSETS[] =
  {{0, 0}, {0, -1}, {0, -2}, {1, -2}, {2, -2}, {2, -1}, {2, 0}, {1, 0}};

}


void TileBurner::update(
  GlobalDependencies& d,
  GlobalState& s,
  const bool isOnScreen,
  entityx::Entity entity)
{
  using components::BehaviorController;
  using engine::components::WorldPosition;

  const auto& myPosition = *entity.component<WorldPosition>();

  if (mFramesElapsed == 0)
  {
    for (const auto& offset : TILE_BURN_AREA_OFFSETS)
    {
      const auto [x, y] = myPosition + offset;

      if (s.mpMap->attributes(x, y).isFlammable())
      {
        d.mpEvents->emit(rigel::events::TileBurnedAway{{x, y}});

        const auto spawnPosition = base::Vec2{x - 1, y + 1};
        const auto spawnDelay = d.mpRandomGenerator->gen() % 4;
        mBurnersToSpawn.push_back(NewBurnerInfo{spawnPosition, spawnDelay});
      }
    }
  }

  for (const auto& info : mBurnersToSpawn)
  {
    if (mFramesElapsed == info.mFramesToWait)
    {
      spawnOneShotSprite(
        *d.mpEntityFactory, data::ActorID::Shot_impact_FX, info.mPosition);
    }
  }

  ++mFramesElapsed;
}

} // namespace rigel::game_logic::behaviors
