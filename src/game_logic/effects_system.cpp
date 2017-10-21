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

#include "effects_system.hpp"

#include "engine/base_components.hpp"
#include "engine/life_time_components.hpp"
#include "engine/particle_system.hpp"
#include "engine/physical_components.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/effect_components.hpp"
#include "game_logic/entity_factory.hpp"
#include "loader/palette.hpp"

#include "game_service_provider.hpp"

RIGEL_DISABLE_WARNINGS
#include <atria/variant/match_boost.hpp>
RIGEL_RESTORE_WARNINGS


namespace rigel { namespace game_logic {

namespace {

using components::DestructionEffects;
using components::SpriteCascadeSpawner;

}


EffectsSystem::EffectsSystem(
  IGameServiceProvider* pServiceProvider,
  engine::RandomNumberGenerator* pRandomGenerator,
  entityx::EntityManager* pEntityManager,
  EntityFactory* pEntityFactory,
  engine::ParticleSystem* pParticles,
  entityx::EventManager& events
)
  : mpServiceProvider(pServiceProvider)
  , mpRandomGenerator(pRandomGenerator)
  , mpEntityManager(pEntityManager)
  , mpEntityFactory(pEntityFactory)
  , mpParticles(pParticles)
{
  events.subscribe<events::ShootableKilled>(*this);
}


void EffectsSystem::update(entityx::EntityManager& es) {
  using namespace engine::components;

  es.each<DestructionEffects, WorldPosition>(
    [this](
      entityx::Entity entity,
      DestructionEffects& effects,
      const WorldPosition& position
    ) {
      if (effects.mActivated) {
        processEffectsAndAdvance(position, effects);
      }
    });

  es.each<SpriteCascadeSpawner>(
    [this](entityx::Entity, SpriteCascadeSpawner& spawner) {
      if (!spawner.mSpawnedLastFrame) {
        const auto xOffset =
          mpRandomGenerator->gen() % spawner.mCoveredArea.width;
        const auto yOffset =
          mpRandomGenerator->gen() % spawner.mCoveredArea.height;
        const auto spawnPosition =
          spawner.mBasePosition + base::Vector{xOffset, -yOffset};

        createFloatingOneShotSprite(
          *mpEntityFactory,
          spawner.mActorId,
          spawnPosition);
      }

      spawner.mSpawnedLastFrame = !spawner.mSpawnedLastFrame;
    });
}


void EffectsSystem::receive(const events::ShootableKilled& event) {
  using namespace engine::components;

  auto entity = event.mEntity;
  if (!entity.has_component<DestructionEffects>()) {
    return;
  }

  auto effects = *entity.component<DestructionEffects>();
  effects.mActivated = true;
  const auto position = *entity.component<WorldPosition>();
  processEffectsAndAdvance(position, effects);

  auto effectSpawner = mpEntityManager->create();
  effectSpawner.assign<DestructionEffects>(effects);
  effectSpawner.assign<WorldPosition>(position);

  const auto iHighestDelaySpec = std::max_element(
    std::cbegin(effects.mEffectSpecs),
    std::cend(effects.mEffectSpecs),
    [](const auto& a, const auto& b) {
      return a.mDelay < b.mDelay;
    });
  const auto timeToLive = iHighestDelaySpec->mDelay;
  effectSpawner.assign<AutoDestroy>(AutoDestroy::afterTimeout(timeToLive));
}


void EffectsSystem::processEffectsAndAdvance(
  const base::Vector& position,
  DestructionEffects& effects
) {
  using namespace effects;
  using namespace engine::components;
  using namespace engine::components::parameter_aliases;

  for (auto& spec : effects.mEffectSpecs) {
    if (effects.mFramesElapsed != spec.mDelay) {
      continue;
    }

    atria::variant::match(spec.mEffect,
      [this](const Sound& sound) {
        mpServiceProvider->playSound(sound.mId);
      },

      [this](const RandomExplosionSound&) {
        const auto randomChoice = mpRandomGenerator->gen();
        const auto soundId = randomChoice % 2 == 0
          ? data::SoundId::AlternateExplosion
          : data::SoundId::Explosion;
        mpServiceProvider->playSound(soundId);
      },

      [&, this](const Particles& particles) {
        const auto color = particles.mColor
          ? *particles.mColor
          : loader::INGAME_PALETTE[mpRandomGenerator->gen() % 16];

        mpParticles->spawnParticles(
          position + particles.mOffset,
          color,
          particles.mVelocityScaleX);
      },

      [&, this](const EffectSprite& sprite) {
        if (sprite.mMovement == EffectSprite::Movement::None) {
          createOneShotSprite(
            *mpEntityFactory,
            sprite.mActorId,
            position + sprite.mOffset);
        } else if (sprite.mMovement == EffectSprite::Movement::FloatUp) {
          createFloatingOneShotSprite(
            *mpEntityFactory,
            sprite.mActorId,
            position + sprite.mOffset);
        } else {
          using M = EffectSprite::Movement;
          static_assert(
            int(M::FlyRight) == int(SpriteMovement::FlyRight) &&
            int(M::FlyUpperRight) == int(SpriteMovement::FlyUpperRight) &&
            int(M::FlyUp) == int(SpriteMovement::FlyUp) &&
            int(M::FlyUpperLeft) == int(SpriteMovement::FlyUpperLeft) &&
            int(M::FlyLeft) == int(SpriteMovement::FlyLeft) &&
            int(M::FlyDown) == int(SpriteMovement::FlyDown),
            "");

          const auto movementType =
            static_cast<SpriteMovement>(static_cast<int>(sprite.mMovement));
          spawnMovingEffectSprite(
            *mpEntityFactory,
            sprite.mActorId,
            movementType,
            position + sprite.mOffset);
        }
      },

      [&, this](const SpriteCascade& cascade) {
        // TODO: The initial offset should be based on the size of the actor
        // that's to be spawned. Currently, it's hard-coded for actor ID 3
        // (small explosion).
        auto offset = base::Vector{-1, 1};
        auto coveredArea = effects.mCascadePlacementBox
          ? *effects.mCascadePlacementBox
          : BoundingBox{};

        auto spawner = mpEntityManager->create();
        SpriteCascadeSpawner spawnerConfig;
        spawnerConfig.mBasePosition = position + offset + coveredArea.topLeft;
        spawnerConfig.mCoveredArea = coveredArea.size;
        spawnerConfig.mActorId = cascade.mActorId;
        spawner.assign<SpriteCascadeSpawner>(spawnerConfig);
        spawner.assign<AutoDestroy>(AutoDestroy::afterTimeout(18));
      },

      [&, this](const ScoreNumber& scoreNumber) {
        // NOTE: We have to offset the score number's Y position by -1 here,
        // because the damage infliction system (which triggers destruction
        // effects usually) runs after the physics system. This means that any
        // score number spawned by this system won't have any movement applied
        // to it on the frame it was spawned on. In the original game, this is
        // different though, as score numbers are part of the "effect actors",
        // which are always updated after the main actors. To compensate,
        // we apply this offset.
        spawnFloatingScoreNumber(
          *mpEntityFactory,
          scoreNumber.mType,
          position + scoreNumber.mOffset + base::Vector{0, -1});
      });
  }

  ++effects.mFramesElapsed;
}

}}
