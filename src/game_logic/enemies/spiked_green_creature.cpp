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

#include "spiked_green_creature.hpp"

#include "base/match.hpp"
#include "data/actor_ids.hpp"
#include "data/sound_ids.hpp"
#include "engine/base_components.hpp"
#include "engine/collision_checker.hpp"
#include "engine/physical_components.hpp"
#include "engine/physics.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "frontend/game_service_provider.hpp"
#include "game_logic/effect_components.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/ientity_factory.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic::behaviors
{

namespace
{

using EffectMovement = effects::EffectSprite::Movement;

const effects::EffectSpec SHELL_BURST_FX_LEFT[] = {
  {effects::EffectSprite{
     {0, -2},
     data::ActorID::Spiked_green_creature_stone_debris_1_LEFT,
     EffectMovement::FlyUpperLeft},
   0},
  {effects::EffectSprite{
     {-2, 0},
     data::ActorID::Spiked_green_creature_stone_debris_2_LEFT,
     EffectMovement::FlyLeft},
   0},
  {effects::EffectSprite{
     {2, -2},
     data::ActorID::Spiked_green_creature_stone_debris_3_LEFT,
     EffectMovement::FlyUp},
   0},
  {effects::EffectSprite{
     {},
     data::ActorID::Spiked_green_creature_stone_debris_4_LEFT,
     EffectMovement::FlyUpperRight},
   0},
};


const effects::EffectSpec SHELL_BURST_FX_RIGHT[] = {
  {effects::EffectSprite{
     {0, -2},
     data::ActorID::Spiked_green_creature_stone_debris_1_RIGHT,
     EffectMovement::FlyUp},
   0},
  {effects::EffectSprite{
     {-2, 0},
     data::ActorID::Spiked_green_creature_stone_debris_2_RIGHT,
     EffectMovement::FlyUpperLeft},
   0},
  {effects::EffectSprite{
     {2, -2},
     data::ActorID::Spiked_green_creature_stone_debris_3_RIGHT,
     EffectMovement::FlyUpperRight},
   0},
  {effects::EffectSprite{
     {},
     data::ActorID::Spiked_green_creature_stone_debris_4_RIGHT,
     EffectMovement::FlyRight},
   0},
};


const int POUNCE_ANIM_SEQ[] = {3, 3, 4, 4, 4, 5};
const int POUNCE_MOVEMENT_Y_OFFSETS[] = {0, 0, -2, -1, 0, 0};

constexpr auto MOVEMENT_SPEED = 2;

} // namespace


void SpikedGreenCreature::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool isOnScreen,
  entityx::Entity entity)
{
  using engine::components::Orientation;

  const auto& bbox = *entity.component<engine::components::BoundingBox>();
  auto& position = *entity.component<engine::components::WorldPosition>();
  auto& body = *entity.component<engine::components::MovingBody>();
  auto& orientation = *entity.component<Orientation>();
  auto& animationFrame =
    entity.component<engine::components::Sprite>()->mFramesToRender[0];

  engine::applyPhysics(
    *d.mpCollisionChecker, *s.mpMap, entity, body, position, bbox);

  base::match(
    mState,
    [&, this](Awakening& state) {
      ++state.mFramesElapsed;
      if (state.mFramesElapsed == 5 || state.mFramesElapsed == 9)
      {
        const auto type = orientation == Orientation::Left
          ? data::ActorID::Spiked_green_creature_eye_FX_LEFT
          : data::ActorID::Spiked_green_creature_eye_FX_RIGHT;
        spawnOneShotSprite(*d.mpEntityFactory, type, position);
      }

      if (state.mFramesElapsed == 15)
      {
        d.mpServiceProvider->playSound(data::SoundId::GlassBreaking);
        animationFrame = 1;
        engine::synchronizeBoundingBoxToSprite(entity);

        spawnEffects(
          components::DestructionEffects{
            orientation == Orientation::Left ? SHELL_BURST_FX_LEFT
                                             : SHELL_BURST_FX_RIGHT},
          position,
          *d.mpEntityManager);
        mState = Waiting{};
      }
    },

    [&, this](Waiting& state) {
      orientation = position.x <= s.mpPlayer->orientedPosition().x
        ? Orientation::Right
        : Orientation::Left;

      ++state.mFramesElapsed;
      if (state.mFramesElapsed == 11)
      {
        // There is a slight bug here, in that we stay on frame 2 the first
        // time around, but all subsequent Waiting states switch to frame 3
        // here. Since this affects the enemy's hitbox, I decided against
        // fixing it.
        ++animationFrame;
        engine::synchronizeBoundingBoxToSprite(entity);
      }

      if (state.mFramesElapsed == 15)
      {
        mState = Pouncing{};
      }
    },

    [&, this](Pouncing& state) {
      if (state.mFramesElapsed == 0)
      {
        engine::startAnimationSequence(entity, POUNCE_ANIM_SEQ);
      }

      engine::synchronizeBoundingBoxToSprite(entity);

      if (state.mFramesElapsed < 6)
      {
        position.y += POUNCE_MOVEMENT_Y_OFFSETS[state.mFramesElapsed];
      }

      if (state.mFramesElapsed > 1)
      {
        const auto offset = engine::orientation::toMovement(orientation);
        position.x += offset * MOVEMENT_SPEED;
      }
      ensureNotStuckInWall(d, entity);

      ++state.mFramesElapsed;
      if (state.mFramesElapsed == 7)
      {
        body.mGravityAffected = true;
        body.mVelocity.y = 1.0f;
        mState = Landing{};
      }
    },

    [&, this](const Landing&) {
      const auto hasLanded = body.mVelocity.y == 0.0f;
      if (hasLanded)
      {
        animationFrame = 2;
        engine::synchronizeBoundingBoxToSprite(entity);

        const auto offset = engine::orientation::toMovement(orientation);
        position.x += offset * MOVEMENT_SPEED;
        ensureNotStuckInWall(d, entity);

        body.mGravityAffected = false;
        mState = Waiting{};
      }
      else
      {
        moveWhileFalling(d, entity);
      }
    });
}


void SpikedGreenCreature::ensureNotStuckInWall(
  const GlobalDependencies& d,
  entityx::Entity entity)
{
  using engine::components::Orientation;

  const auto& bbox = *entity.component<engine::components::BoundingBox>();
  auto& orientation = *entity.component<Orientation>();
  auto& position = *entity.component<engine::components::WorldPosition>();

  const auto movementOffset = engine::orientation::toMovement(orientation);
  const auto positionForChecking = position - base::Vec2{movementOffset, 0};

  const auto isCurrentlyColliding = orientation == Orientation::Left
    ? d.mpCollisionChecker->isTouchingLeftWall(positionForChecking, bbox)
    : d.mpCollisionChecker->isTouchingRightWall(positionForChecking, bbox);
  if (isCurrentlyColliding)
  {
    orientation = engine::orientation::opposite(orientation);
    position.x -= movementOffset;
  }
}


void SpikedGreenCreature::moveWhileFalling(
  const GlobalDependencies& d,
  entityx::Entity entity)
{
  using engine::components::Orientation;

  auto& position = *entity.component<engine::components::WorldPosition>();

  const auto orientation = *entity.component<Orientation>();
  const auto offset = engine::orientation::toMovement(orientation);

  position.x += offset;
  engine::moveHorizontallyWithYAdjust(*d.mpCollisionChecker, entity, offset);
}

} // namespace rigel::game_logic::behaviors
