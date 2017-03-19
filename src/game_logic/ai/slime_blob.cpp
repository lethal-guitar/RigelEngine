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

#include "slime_blob.hpp"

#include "engine/base_components.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/entity_factory.hpp"


namespace rigel { namespace game_logic { namespace ai {

using engine::components::BoundingBox;
using engine::components::Sprite;
using game_logic::components::Shootable;


namespace {

const auto NUM_BREAK_ANIMATION_STEPS = 6;
const auto BREAK_ANIM_SPEED = 3; // frames between animation steps

const auto SLIME_BLOB_SPAWN_OFFSET = base::Vector{2, 0};

}


void configureSlimeContainer(entityx::Entity entity) {
  // Render slots: Main part, roof, animated glass contents
  entity.component<Sprite>()->mFramesToRender = {2, 8, 0};
  entity.assign<BoundingBox>(BoundingBox{{1, -2}, {3, 3}});
  entity.assign<components::SlimeContainer>();

  entity.component<Shootable>()->mDestroyWhenKilled = false;
}


SlimeBlobSystem::SlimeBlobSystem(
  entityx::Entity player,
  EntityFactory* pEntityFactory,
  engine::RandomNumberGenerator* pRandomGenerator
)
  : mPlayer(player)
  , mpEntityFactory(pEntityFactory)
  , mpRandomGenerator(pRandomGenerator)
{
}


void SlimeBlobSystem::update(
  entityx::EntityManager& es,
  entityx::EventManager& events,
  entityx::TimeDelta dt
) {
  using engine::components::Active;
  using engine::components::WorldPosition;

  es.each<Sprite, WorldPosition, components::SlimeContainer, Active>(
    [this](
      entityx::Entity entity,
      Sprite& sprite,
      const WorldPosition& position,
      components::SlimeContainer& state,
      const engine::components::Active&
    ) {
      const auto stillIntact = entity.has_component<Shootable>();
      if (stillIntact) {
        // Animate slime blob inside
        sprite.mFramesToRender[2] = mpRandomGenerator->gen() % 2;
      } else {
        ++state.mAnimationStepCounter;
        if (state.mAnimationStepCounter >= BREAK_ANIM_SPEED) {
          state.mAnimationStepCounter = 0;
          ++state.mBreakAnimationStep;
        }

        if (state.mBreakAnimationStep >= NUM_BREAK_ANIMATION_STEPS) {
          entity.remove<components::SlimeContainer>();
          entity.remove<BoundingBox>();
          entity.remove<Active>();

          mpEntityFactory->createActor(67, position + SLIME_BLOB_SPAWN_OFFSET);
        }

        if (state.mBreakAnimationStep < NUM_BREAK_ANIMATION_STEPS) {
          sprite.mFramesToRender[0] = state.mBreakAnimationStep + 2;
        }
      }
    });

  // TODO: Slime blob enemy logic
}


void SlimeBlobSystem::onEntityHit(entityx::Entity entity) {
  if (
    !entity.has_component<components::SlimeContainer>() ||
    !entity.has_component<Shootable>()
  ) {
    return;
  }

  entity.remove<Shootable>();

  auto& sprite = *entity.component<Sprite>();
  sprite.mFramesToRender.pop_back();
  sprite.mFramesToRender[0] = 2;

  sprite.flashWhite();
}

}}}
