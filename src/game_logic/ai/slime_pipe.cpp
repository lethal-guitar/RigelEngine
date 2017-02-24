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

#include "slime_pipe.hpp"

#include "engine/base_components.hpp"
#include "engine/life_time_components.hpp"
#include "engine/physical_components.hpp"
#include "game_logic/entity_factory.hpp"
#include "game_logic/damage_components.hpp"
#include "game_mode.hpp"


namespace rigel { namespace game_logic { namespace ai {

using engine::components::Active;
using engine::components::WorldPosition;


namespace {

const data::ActorID DROP_ACTOR_ID = 118;
const auto DROP_FREQUENCY = 25;
const auto DROP_OFFSET = WorldPosition{1, 0};

}


SlimePipeSystem::SlimePipeSystem(
  EntityFactory* pEntityFactory,
  IGameServiceProvider* pServiceProvider
)
  : mpEntityFactory(pEntityFactory)
  , mpServiceProvider(pServiceProvider)
{
}


void SlimePipeSystem::update(
  entityx::EntityManager& es,
  entityx::EventManager& events,
  entityx::TimeDelta dt
) {
  if (!engine::updateAndCheckIfDesiredTicksElapsed(mTimeStepper, 2, dt)) {
    return;
  }

  es.each<components::SlimePipe, WorldPosition, Active>(
    [this](
      entityx::Entity,
      components::SlimePipe& state,
      const WorldPosition& position,
      const Active&
    ) {
      ++state.mGameFramesSinceLastDrop;
      if (state.mGameFramesSinceLastDrop >= DROP_FREQUENCY) {
        state.mGameFramesSinceLastDrop = 0;
        createSlimeDrop(position);
        mpServiceProvider->playSound(data::SoundId::WaterDrop);
      }
    });
}


void SlimePipeSystem::createSlimeDrop(const base::Vector& position) {
  using namespace engine::components;

  auto entity = mpEntityFactory->createSprite(
    DROP_ACTOR_ID,
    position + DROP_OFFSET,
    true);
  // Gravity handles the drop's movement, so velocity is initally 0.
  entity.assign<Physical>(base::Point<float>{0.0f, 0.0f}, true);

  entity.assign<game_logic::components::PlayerDamaging>(1);
  entity.assign<AutoDestroy>(AutoDestroy::Condition::OnWorldCollision);
  entity.assign<Active>();
}

}}}
