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

#include "dying_boss.hpp"

#include "common/game_service_provider.hpp"
#include "data/game_traits.hpp"
#include "data/player_model.hpp"
#include "engine/particle_system.hpp"
#include "engine/random_number_generator.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/ientity_factory.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic::behaviors
{

namespace
{

constexpr auto BOSS_KILL_SCORE = 50'000;

}


DyingBoss::DyingBoss(const int episodeNr)
  // In the final episode, the boss explodes completely instead of flying
  // away.
  : mShowSpriteDuringFlyAway(episodeNr != 3)
{
}


void DyingBoss::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool,
  entityx::Entity entity)
{
  auto& position = *entity.component<engine::components::WorldPosition>();
  auto& sprite = *entity.component<engine::components::Sprite>();

  auto bigExplosionEffect = [&]() {
    d.mpEvents->emit(rigel::events::ScreenFlash{});
    d.mpServiceProvider->playSound(data::SoundId::BigExplosion);
  };

  auto rand = [&]() {
    return d.mpRandomGenerator->gen();
  };

  auto randomExplosionSound = [&]() {
    // TODO: Eliminate duplication with code in effects_system.cpp
    const auto randomChoice = d.mpRandomGenerator->gen();
    const auto soundId = randomChoice % 2 == 0
      ? data::SoundId::AlternateExplosion
      : data::SoundId::Explosion;
    d.mpServiceProvider->playSound(soundId);
  };


  if (mFramesElapsed == 0)
  {
    d.mpServiceProvider->stopMusic();
    s.mpPlayer->model().giveScore(BOSS_KILL_SCORE);
  }

  switch (mFramesElapsed)
  {
    // clang-format off
    case 1: case 5: case 12: case 14: case 19: case 23: case 25: case 28:
    case 30: case 34: case 38: case 41: case 46: case 48:
      // clang-format on
      d.mpParticles->spawnParticles(
        position + base::Vector{rand() % 4, -(rand() % 8)},
        data::GameTraits::INGAME_PALETTE[rand() % 16],
        rand() % 2 - 1);
      spawnOneShotSprite(
        *d.mpEntityFactory,
        data::ActorID::Explosion_FX_1,
        position + base::Vector{rand() % 4, -(rand() % 8)});
      spawnMovingEffectSprite(
        *d.mpEntityFactory,
        data::ActorID::Shot_impact_FX,
        SpriteMovement::FlyDown,
        position + base::Vector{rand() % 4, -(rand() % 8)});
      break;

    default:
      break;
  }

  if (mFramesElapsed < 48)
  {
    sprite.mShow = !s.mpPerFrameState->mIsOddFrame;

    if ((rand() / 4) % 2 != 0 && s.mpPerFrameState->mIsOddFrame)
    {
      bigExplosionEffect();
    }
    else
    {
      randomExplosionSound();
    }
  }
  else if (mFramesElapsed == 48)
  {
    sprite.mShow = mShowSpriteDuringFlyAway;
    bigExplosionEffect();
  }
  else
  { // > 50
    if (position.y > 3)
    {
      position.y -= 2;
    }
  }

  if (mFramesElapsed == 58)
  {
    d.mpEvents->emit(rigel::events::ExitReached{false});
  }

  ++mFramesElapsed;
}

} // namespace rigel::game_logic::behaviors
