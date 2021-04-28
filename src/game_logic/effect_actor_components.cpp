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

#include "effect_actor_components.hpp"

#include "common/game_service_provider.hpp"
#include "data/game_traits.hpp"
#include "engine/base_components.hpp"
#include "engine/random_number_generator.hpp"
#include "game_logic/effect_components.hpp"
#include "game_logic/ientity_factory.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic::components
{

namespace
{

constexpr auto MAX_Y_OFFSET = 16;

}


void WindBlownSpiderGenerator::update(
  GlobalDependencies& d,
  GlobalState& s,
  const bool isOnScreen,
  entityx::Entity entity)
{
  const auto& position = *entity.component<engine::components::WorldPosition>();
  if (
    position.y > s.mpPlayer->position().y &&
    d.mpRandomGenerator->gen() % 2 != 0 && s.mpPerFrameState->mIsOddFrame)
  {
    const auto rightScreenEdge =
      s.mpPerFrameState->mCurrentViewPortSize.width - 1;
    const auto effectActorId = 241 + d.mpRandomGenerator->gen() % 3;
    const auto xPos = s.mpCameraPosition->x + rightScreenEdge;
    const auto yPos =
      s.mpCameraPosition->y + d.mpRandomGenerator->gen() % MAX_Y_OFFSET;
    const auto movementType = d.mpRandomGenerator->gen() % 2 != 0
      ? SpriteMovement::SwirlAround
      : SpriteMovement::FlyLeft;

    spawnMovingEffectSprite(
      *d.mpEntityFactory,
      static_cast<data::ActorID>(effectActorId),
      movementType,
      {xPos, yPos});
  }
}


void WaterDropGenerator::update(
  GlobalDependencies& d,
  GlobalState& state,
  const bool isOnScreen,
  entityx::Entity entity)
{
  const auto& position = *entity.component<engine::components::WorldPosition>();
  if (state.mpPerFrameState->mIsOddFrame && d.mpRandomGenerator->gen() >= 220)
  {
    auto drop =
      d.mpEntityFactory->spawnActor(data::ActorID::Water_drop, position);
    drop.assign<engine::components::Active>();

    if (isOnScreen)
    {
      d.mpServiceProvider->playSound(data::SoundId::WaterDrop);
    }
  }
}


void ExplosionEffect::update(
  GlobalDependencies& d,
  GlobalState&,
  bool,
  entityx::Entity entity)
{
  triggerEffects(entity, *d.mpEntityManager);
  entity.destroy();
}


void AirLockDeathTrigger::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool,
  entityx::Entity entity)
{
  using engine::components::Orientation;

  const auto& position = *entity.component<engine::components::WorldPosition>();
  const auto orientation = *entity.component<Orientation>();

  const auto xToCheck =
    orientation == Orientation::Left ? position.x - 3 : position.x + 3;
  if (s.mpMap->tileAt(0, xToCheck, position.y) == 0)
  {
    d.mpEvents->emit(events::AirLockOpened{orientation});
    entity.destroy();
  }
}

} // namespace rigel::game_logic::components
