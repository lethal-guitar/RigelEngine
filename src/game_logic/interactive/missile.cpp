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

#include "missile.hpp"

#include "common/game_service_provider.hpp"
#include "data/game_traits.hpp"
#include "data/sound_ids.hpp"
#include "engine/base_components.hpp"
#include "engine/movement.hpp"
#include "engine/particle_system.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/effect_components.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/ientity_factory.hpp"


namespace rigel::game_logic::behaviors
{

namespace
{

constexpr int FALL_OVER_ANIM_LEFT[] = {0, 1, 2, 3, 2, 3, 4, 3};

constexpr int FALL_OVER_ANIM_RIGHT[] = {0, 5, 6, 7, 6, 7, 8, 7};

constexpr auto MISSILE_HEIGHT = 12;


void startFlameAnimation(entityx::Entity entity)
{
  using namespace engine::components;

  // Start the missile fire animation in render slot 2, moving the missile
  // body sprite to render slot 3.
  // This is necessary because the missle body must be rendered on top of the
  // flame for things to look correct, but we can't put the flame in render
  // slot 1, since animations in render slot 1 cause the entities bounding box
  // to adapt to the sprite, which would be incorrect in this case (we want
  // the bounding box to encompass only the missile body).
  //
  // This is a bit more complicated than you might expect, but living with this
  // complexity in this one edge case makes 99% of the other cases simpler.
  // This seems preferable to making animation system more complex, e.g. by
  // allowing to configure which render slot is used for the bounding box.
  entity.component<Sprite>()->mFramesToRender[0] = engine::IGNORE_RENDER_SLOT;
  entity.component<Sprite>()->mFramesToRender[1] = 1;
  entity.component<Sprite>()->mFramesToRender[2] = 0;
  engine::startAnimationLoop(entity, 1, 1, 2, 1);
}


void triggerHitEffect(entityx::Entity entity, engine::ParticleSystem& particles)
{
  using namespace engine::components;

  const auto& position = *entity.component<WorldPosition>();
  entity.component<Sprite>()->flashWhite();
  particles.spawnParticles(
    position + base::Vec2{5, 0}, data::GameTraits::INGAME_PALETTE[15]);
}

} // namespace


void Missile::update(
  GlobalDependencies& d,
  GlobalState& state,
  const bool isOnScreen,
  entityx::Entity entity)
{
  using namespace engine::components;

  if (!mIsActive)
  {
    return;
  }

  const auto& position = *entity.component<WorldPosition>();

  auto detonate = [&]() {
    const auto impactPosition = position - base::Vec2{0, MISSILE_HEIGHT};
    d.mpEvents->emit(rigel::events::MissileDetonated{impactPosition});
    triggerEffects(entity, *d.mpEntityManager);
  };


  if (mFramesElapsed == 0)
  {
    // Ignition animation
    spawnOneShotSprite(
      *d.mpEntityFactory,
      data::ActorID::White_circle_flash_FX,
      position + base::Vec2{-2, 1});
    spawnOneShotSprite(
      *d.mpEntityFactory,
      data::ActorID::White_circle_flash_FX,
      position + base::Vec2{1, 1});
  }
  else if (mFramesElapsed == 5)
  {
    startFlameAnimation(entity);
  }
  else if (mFramesElapsed >= 5)
  {
    d.mpServiceProvider->playSound(data::SoundId::FlameThrowerShot);

    const auto speed = mFramesElapsed >= 8 ? 2 : 1;
    const auto movementResult =
      engine::moveVertically(*d.mpCollisionChecker, entity, -speed);

    if (movementResult != engine::MovementResult::Completed)
    {
      detonate();
      entity.destroy();
      return;
    }
  }

  ++mFramesElapsed;
}


void Missile::onKilled(
  GlobalDependencies& d,
  GlobalState& state,
  const base::Vec2T<float>& inflictorVelocity,
  entityx::Entity entity)
{
  if (!mIsActive)
  {
    mIsActive = true;
    triggerHitEffect(entity, *d.mpParticles);
  }
}


void BrokenMissile::update(
  GlobalDependencies& d,
  GlobalState& state,
  const bool isOnScreen,
  entityx::Entity entity)
{
  using namespace engine::components;

  if (!mIsActive)
  {
    return;
  }

  auto detonate = [&]() {
    const auto& position = *entity.component<WorldPosition>();
    d.mpEvents->emit(
      rigel::events::ScreenFlash{data::GameTraits::INGAME_PALETTE[15]});
    triggerEffects(entity, *d.mpEntityManager);
    spawnOneShotSprite(
      *d.mpEntityFactory, data::ActorID::Nuclear_explosion, position);
  };


  if (mFramesElapsed == 2 || mFramesElapsed == 4 || mFramesElapsed == 6)
  {
    // This is meant to play sound effects along with the animation, i.e.
    // every time the missile hits the ground
    d.mpServiceProvider->playSound(data::SoundId::DukeAttachClimbable);
  }
  else if (mFramesElapsed == 11)
  {
    detonate();
    entity.destroy();
    return;
  }

  ++mFramesElapsed;
}


void BrokenMissile::onKilled(
  GlobalDependencies& d,
  GlobalState& state,
  const base::Vec2T<float>& inflictorVelocity,
  entityx::Entity entity)
{
  if (!mIsActive)
  {
    mIsActive = true;

    const auto shotFromLeft = inflictorVelocity.x > 0;
    const auto fallingLeft = !shotFromLeft;

    triggerHitEffect(entity, *d.mpParticles);

    engine::startAnimationSequence(
      entity, fallingLeft ? FALL_OVER_ANIM_LEFT : FALL_OVER_ANIM_RIGHT);
  }
}

} // namespace rigel::game_logic::behaviors
