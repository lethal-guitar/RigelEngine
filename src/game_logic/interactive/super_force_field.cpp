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

#include "super_force_field.hpp"

#include "data/game_traits.hpp"
#include "data/player_model.hpp"
#include "data/strings.hpp"
#include "engine/particle_system.hpp"
#include "engine/physical_components.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "frontend/game_service_provider.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/ientity_factory.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic::behaviors
{

void SuperForceField::update(
  GlobalDependencies& d,
  GlobalState& s,
  const bool isOnScreen,
  entityx::Entity entity)
{
  using namespace engine::components;

  const auto& position = *entity.component<WorldPosition>();
  const auto& bbox = *entity.component<engine::components::BoundingBox>();
  auto& sprite = *entity.component<Sprite>();
  auto& playerPos = s.mpPlayer->position();

  if (mFizzleFramesElapsed)
  {
    auto& animationFrame = sprite.mFramesToRender[0];

    auto& framesElapsed = *mFizzleFramesElapsed;
    ++framesElapsed;

    animationFrame = (s.mpPerFrameState->mIsOddFrame ? 0 : 1) + 1;
    if ((d.mpRandomGenerator->gen() / 8) % 2 != 0)
    {
      d.mpServiceProvider->playSound(data::SoundId::ForceFieldFizzle);
      sprite.flashWhite(0);
    }

    if (framesElapsed == 19)
    {
      animationFrame = 0;
      mFizzleFramesElapsed = std::nullopt;
    }

    engine::synchronizeBoundingBoxToSprite(entity);
  }

  if (mDestructionFramesElapsed)
  {
    auto& framesElapsed = *mDestructionFramesElapsed;
    ++framesElapsed;

    if (framesElapsed % 2 != 0)
    {
      d.mpServiceProvider->playSound(data::SoundId::GlassBreaking);
      d.mpParticles->spawnParticles(
        position + base::Vec2{1, -framesElapsed + 14},
        data::GameTraits::INGAME_PALETTE[11]);
      spawnFloatingScoreNumber(
        *d.mpEntityFactory,
        ScoreNumberType::S500,
        position + base::Vec2{0, -framesElapsed + 18});
      s.mpPlayer->model().giveScore(500);
    }

    if (framesElapsed == 10)
    {
      d.mpEvents->emit(
        rigel::events::PlayerMessage{data::Messages::ForceFieldDestroyed});
      d.mpServiceProvider->playSound(data::SoundId::BigExplosion);
      spawnMovingEffectSprite(
        *d.mpEntityFactory,
        data::ActorID::Explosion_FX_2,
        SpriteMovement::FlyUpperLeft,
        position + base::Vec2{-1, 5});
      spawnMovingEffectSprite(
        *d.mpEntityFactory,
        data::ActorID::Explosion_FX_2,
        SpriteMovement::FlyUpperRight,
        position + base::Vec2{-1, 5});
      spawnMovingEffectSprite(
        *d.mpEntityFactory,
        data::ActorID::Explosion_FX_2,
        SpriteMovement::FlyDown,
        position + base::Vec2{-1, 5});
      entity.destroy();
      return;
    }
  }

  const auto worldBbox = engine::toWorldSpace(bbox, position);
  const auto touchingPlayer =
    worldBbox.intersects(s.mpPlayer->worldSpaceHitBox());
  if (touchingPlayer)
  {
    if (s.mpPlayer->isCloaked())
    {
      mDestructionFramesElapsed = 0;
    }
    else
    {
      startFizzle();
      s.mpPlayer->takeDamage(1);
      d.mpEvents->emit(
        rigel::events::TutorialMessage{data::TutorialMessageId::CloakNeeded});

      if (playerPos.x + 2 <= position.x)
      {
        --playerPos.x;
      }

      if (playerPos.x + 2 > position.x)
      {
        ++playerPos.x;
      }
    }
  }
}


void SuperForceField::onHit(
  GlobalDependencies& d,
  GlobalState& s,
  entityx::Entity,
  entityx::Entity entity)
{
  using game_logic::components::Shootable;

  startFizzle();
  entity.component<Shootable>()->mHealth = 100;
}


void SuperForceField::startFizzle()
{
  if (!mFizzleFramesElapsed)
  {
    mFizzleFramesElapsed = 0;
  }
}

} // namespace rigel::game_logic::behaviors
