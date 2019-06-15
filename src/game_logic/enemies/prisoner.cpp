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

#include "common/game_service_provider.hpp"
#include "engine/life_time_components.hpp"
#include "engine/particle_system.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/entity_factory.hpp"


namespace rigel { namespace game_logic { namespace ai {

using engine::components::Sprite;
using engine::components::WorldPosition;
using game_logic::components::Shootable;

namespace {

const int DEATH_SEQUENCE[] = {5, 5, 6, 7};

const auto DEATH_FRAMES_TO_LIVE = 6;

}


PrisonerSystem::PrisonerSystem(
  entityx::Entity player,
  EntityFactory* pEntityFactory,
  IGameServiceProvider* pServiceProvider,
  engine::ParticleSystem* pParticles,
  engine::RandomNumberGenerator* pRandomGenerator,
  entityx::EventManager& events
)
  : mPlayer(player)
  , mpEntityFactory(pEntityFactory)
  , mpServiceProvider(pServiceProvider)
  , mpParticles(pParticles)
  , mpRandomGenerator(pRandomGenerator)
{
  events.subscribe<events::ShootableKilled>(*this);
}


void PrisonerSystem::update(entityx::EntityManager& es) {
  using engine::components::Active;

  mIsOddFrame = !mIsOddFrame;

  es.each<Sprite, WorldPosition, components::Prisoner, Active>(
    [this](
      entityx::Entity entity,
      Sprite& sprite,
      const WorldPosition& position,
      components::Prisoner& state,
      const engine::components::Active&
    ) {
      if (state.mIsAggressive) {
        updateAggressivePrisoner(entity, position, state, sprite);
      } else {
        const auto shakeIronBars = (mpRandomGenerator->gen() / 4) % 2 != 0;
        // The animation has two frames, 0 is "idle" and 1 is "shaking".
        sprite.mFramesToRender[0] = int{shakeIronBars};
      }
    });
}


void PrisonerSystem::updateAggressivePrisoner(
  entityx::Entity entity,
  const WorldPosition& position,
  components::Prisoner& state,
  Sprite& sprite
) {
  using game_logic::components::PlayerDamaging;
  auto& shootable = *entity.component<Shootable>();

  // See if we want to grab
  if (!state.mIsGrabbing) {
    // TODO: Adjust player position according to orientation to replicate
    // original positioning?
    const auto& playerPos = *mPlayer.component<WorldPosition>();
    const auto playerInRange =
      position.x - 4 < playerPos.x &&
      position.x + 7 > playerPos.x;

    if (playerInRange) {
      const auto wantsToGrab =
        (mpRandomGenerator->gen() / 16) % 2 != 0 && mIsOddFrame;
      if (wantsToGrab) {
        state.mIsGrabbing = true;
        state.mGrabStep = 0;
        sprite.mFramesToRender.push_back(1);
        shootable.mInvincible = false;
        entity.assign<PlayerDamaging>(1);
      }
    }
  }

  // If we decided to grab, we immediately update accordingly on the
  // same frame (this is how it works in the original game)
  if (state.mIsGrabbing) {
    sprite.mFramesToRender[1] = (state.mGrabStep + 1) % 5;

    if (state.mGrabStep >= 4) {
      state.mIsGrabbing = false;
      sprite.mFramesToRender.pop_back();
      shootable.mInvincible = true;
      entity.remove<PlayerDamaging>();
    }

    // Do this *after* checking whether the grab sequence is finished.
    // This is required in order to get exactly the same sequence as in the
    // original game.
    if (mIsOddFrame) {
      ++state.mGrabStep;
    }
  }
}


void PrisonerSystem::receive(const events::ShootableKilled& event) {
  using engine::components::AutoDestroy;

  auto entity = event.mEntity;
  if (!entity.has_component<components::Prisoner>()) {
    return;
  }

  const auto& position = *entity.component<WorldPosition>();
  auto& sprite = *entity.component<Sprite>();
  auto& state = *entity.component<components::Prisoner>();

  if (state.mIsGrabbing) {
    sprite.mFramesToRender.pop_back();
    entity.remove<game_logic::components::PlayerDamaging>();
  }

  engine::startAnimationSequence(entity, DEATH_SEQUENCE);
  entity.assign<AutoDestroy>(AutoDestroy::afterTimeout(DEATH_FRAMES_TO_LIVE));
  entity.remove<components::Prisoner>();

  const auto shotFromLeft = event.mInflictorVelocity.x > 0.0f;
  const auto debrisMovement = shotFromLeft
    ? SpriteMovement::FlyUpperRight
    : SpriteMovement::FlyUpperLeft;
  spawnMovingEffectSprite(*mpEntityFactory, data::ActorID::Prisoner_hand_debris, debrisMovement, position);

  mpParticles->spawnParticles(
    position + base::Vector{3, 0},
    loader::INGAME_PALETTE[5]);

  mpServiceProvider->playSound(data::SoundId::BiologicalEnemyDestroyed);
}

}}}
