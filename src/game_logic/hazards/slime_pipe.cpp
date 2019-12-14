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

#include "common/game_service_provider.hpp"
#include "engine/base_components.hpp"
#include "engine/entity_tools.hpp"
#include "engine/life_time_components.hpp"
#include "engine/physical_components.hpp"
#include "game_logic/actor_tag.hpp"
#include "game_logic/behavior_controller.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/entity_factory.hpp"


namespace rigel::game_logic::behaviors {

using engine::components::Active;
using engine::components::WorldPosition;


namespace {

const data::ActorID DROP_ACTOR_ID = data::ActorID::Slime_drop;
const auto DROP_FREQUENCY = 25;
const auto DROP_OFFSET = WorldPosition{1, 1};


void createSlimeDrop(
  const base::Vector& position,
  IEntityFactory& entityFactory
) {
  using namespace engine::components;
  using namespace engine::components::parameter_aliases;
  using namespace game_logic::components;
  using namespace game_logic::components::parameter_aliases;

  auto entity = entityFactory.createSprite(
    DROP_ACTOR_ID,
    position + DROP_OFFSET,
    true);
  // Gravity handles the drop's movement, so velocity is initally 0.
  entity.assign<MovingBody>(Velocity{0.0f, 0.0f}, GravityAffected{true});
  entity.assign<AppearsOnRadar>();

  entity.assign<PlayerDamaging>(Damage{1});
  entity.assign<AutoDestroy>(AutoDestroy{
    AutoDestroy::Condition::OnLeavingActiveRegion});
  entity.assign<Active>();
  entity.assign<BehaviorController>(SlimeDrop{});
}

}


void SlimePipe::update(
  GlobalDependencies& d,
  GlobalState& state,
  bool isOnScreen,
  entityx::Entity entity
) {
  const auto& position = *entity.component<WorldPosition>();

  ++mGameFramesSinceLastDrop;
  if (mGameFramesSinceLastDrop >= DROP_FREQUENCY) {
    mGameFramesSinceLastDrop = 0;
    createSlimeDrop(position, *d.mpEntityFactory);
    d.mpServiceProvider->playSound(data::SoundId::WaterDrop);
  }
}


void SlimeDrop::onCollision(
  GlobalDependencies& dependencies,
  GlobalState& state,
  const engine::events::CollidedWithWorld&,
  entityx::Entity entity
) {
  using namespace engine::components;

  auto& sprite = *entity.component<Sprite>();
  sprite.mFramesToRender[0] = 1;

  engine::reassign<AutoDestroy>(entity, AutoDestroy::afterTimeout(1));
}

}
