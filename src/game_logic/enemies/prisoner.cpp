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

#include "prisoner.hpp"

#include "data/game_traits.hpp"
#include "engine/life_time_components.hpp"
#include "engine/particle_system.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "frontend/game_service_provider.hpp"
#include "game_logic/behavior_controller.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/ientity_factory.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic::behaviors
{

namespace
{

const int DEATH_SEQUENCE[] = {5, 5, 6, 7};

const auto DEATH_FRAMES_TO_LIVE = 6;

} // namespace


void AggressivePrisoner::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool,
  entityx::Entity entity)
{
  using engine::components::WorldPosition;
  using game_logic::components::PlayerDamaging;
  using game_logic::components::Shootable;

  auto& sprite = *entity.component<engine::components::Sprite>();
  auto& shootable = *entity.component<Shootable>();

  // See if we want to grab
  if (!mIsGrabbing)
  {
    const auto& position = *entity.component<WorldPosition>();
    const auto& playerPos = s.mpPlayer->orientedPosition();
    const auto playerInRange =
      position.x - 4 < playerPos.x && position.x + 7 > playerPos.x;

    if (playerInRange)
    {
      const auto wantsToGrab = (d.mpRandomGenerator->gen() / 16) % 2 != 0 &&
        s.mpPerFrameState->mIsOddFrame;
      if (wantsToGrab)
      {
        mIsGrabbing = true;
        mGrabStep = 0;
        sprite.mFramesToRender[1] = 1;
        shootable.mInvincible = false;
        entity.assign<PlayerDamaging>(1);
      }
    }
  }

  // If we decided to grab, we immediately update accordingly on the
  // same frame (this is how it works in the original game)
  if (mIsGrabbing)
  {
    sprite.mFramesToRender[1] = (mGrabStep + 1) % 5;

    if (mGrabStep >= 4)
    {
      mIsGrabbing = false;
      sprite.mFramesToRender[1] = engine::IGNORE_RENDER_SLOT;
      shootable.mInvincible = true;
      entity.remove<PlayerDamaging>();
    }

    // Do this *after* checking whether the grab sequence is finished.
    // This is required in order to get exactly the same sequence as in the
    // original game.
    if (s.mpPerFrameState->mIsOddFrame)
    {
      ++mGrabStep;
    }
  }
}


void AggressivePrisoner::onKilled(
  GlobalDependencies& d,
  GlobalState& s,
  const base::Vec2f& inflictorVelocity,
  entityx::Entity entity)
{
  using engine::components::AutoDestroy;
  using engine::components::Sprite;
  using engine::components::WorldPosition;

  auto& sprite = *entity.component<Sprite>();

  if (mIsGrabbing)
  {
    sprite.mFramesToRender[1] = engine::IGNORE_RENDER_SLOT;
    entity.remove<game_logic::components::PlayerDamaging>();
  }

  engine::startAnimationSequence(entity, DEATH_SEQUENCE);
  entity.assign<AutoDestroy>(AutoDestroy::afterTimeout(DEATH_FRAMES_TO_LIVE));

  const auto shotFromLeft = inflictorVelocity.x > 0.0f;
  const auto debrisMovement =
    shotFromLeft ? SpriteMovement::FlyUpperRight : SpriteMovement::FlyUpperLeft;
  const auto& position = *entity.component<WorldPosition>();
  spawnMovingEffectSprite(
    *d.mpEntityFactory,
    data::ActorID::Prisoner_hand_debris,
    debrisMovement,
    position);
  d.mpParticles->spawnParticles(
    position + base::Vec2{3, 0}, data::GameTraits::INGAME_PALETTE[5]);
  d.mpServiceProvider->playSound(data::SoundId::BiologicalEnemyDestroyed);

  entity.remove<components::BehaviorController>();
}


void PassivePrisoner::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool,
  entityx::Entity entity)
{
  const auto shakeIronBars = (d.mpRandomGenerator->gen() / 4) % 2 != 0;

  // The animation has two frames, 0 is "idle" and 1 is "shaking".
  auto& sprite = *entity.component<engine::components::Sprite>();
  sprite.mFramesToRender[0] = int{shakeIronBars};
}

} // namespace rigel::game_logic::behaviors
