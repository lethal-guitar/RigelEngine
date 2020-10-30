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

#include "boss_episode_4.hpp"

#include "base/math_tools.hpp"
#include "common/game_service_provider.hpp"
#include "common/global.hpp"
#include "engine/base_components.hpp"
#include "engine/life_time_components.hpp"
#include "engine/physical_components.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/ientity_factory.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic::behaviors {

namespace {

constexpr auto OFFSET_TO_TARGET = base::Vector{-4, -4};
constexpr auto PROJECTILE_OFFSET_TO_TARGET = base::Vector{1, -1};
constexpr auto SHOT_OFFSET = base::Vector{4, 2};

}


void BossEpisode4::update(
  GlobalDependencies& d,
  GlobalState& s,
  const bool isOnScreen,
  entityx::Entity entity
) {
  using engine::components::WorldPosition;

  auto& position = *entity.component<WorldPosition>();
  //auto& sprite = *entity.component<Sprite>();
  const auto& playerPos = s.mpPlayer->orientedPosition();

  auto moveTowardsPlayer = [&]() {
    if (!s.mpPerFrameState->mIsOddFrame) {
      return false;
    }

    const auto targetPosition = playerPos + OFFSET_TO_TARGET;
    const auto movementVec = targetPosition - position;
    position +=
      base::Vector{base::sgn(movementVec.x), base::sgn(movementVec.y)};

    return movementVec != base::Vector{0, 0};
  };

  auto fireShot = [&]() {
    d.mpEntityFactory->spawnActor(
      data::ActorID::BOSS_Episode_4_projectile,
      position + SHOT_OFFSET);
  };


  if (!mHasBeenSighted) {
    d.mpEvents->emit(rigel::events::BossActivated{entity});
    mHasBeenSighted = true;
  }

  if (mCoolDownFrames > 0) {
    --mCoolDownFrames;
    return;
  }

  if (moveTowardsPlayer()) {
    ++mFramesSinceLastShot;

    if (mFramesSinceLastShot == 12) {
      mFramesSinceLastShot = 0;
      mCoolDownFrames = 12;
      fireShot();
    }
  }
}


void BossEpisode4::onKilled(
  GlobalDependencies& d,
  GlobalState&,
  const base::Point<float>&,
  entityx::Entity entity
) {
  d.mpEvents->emit(rigel::events::BossDestroyed{entity});
}


void BossEpisode4Projectile::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool,
  entityx::Entity entity
) {
  using engine::components::AutoDestroy;
  using engine::components::BoundingBox;
  using engine::components::WorldPosition;

  auto& position = *entity.component<WorldPosition>();
  const auto& bbox = *entity.component<BoundingBox>();
  const auto& playerPos = s.mpPlayer->orientedPosition();

  if (mFramesElapsed < 8) {
    ++mFramesElapsed;
  } else if (d.mpRandomGenerator->gen() % 4 != 0) {
    const auto targetPosition = playerPos + PROJECTILE_OFFSET_TO_TARGET;
    const auto movementVec = targetPosition - position;
    position +=
      base::Vector{base::sgn(movementVec.x), base::sgn(movementVec.y)};
  }

  const auto worldSpaceBbox = engine::toWorldSpace(bbox, position);
  if (s.mpPlayer->worldSpaceHitBox().intersects(worldSpaceBbox)) {
    // TODO: Eliminate duplication with code in effects_system.cpp
    const auto randomChoice = d.mpRandomGenerator->gen();
    const auto soundId = randomChoice % 2 == 0
      ? data::SoundId::AlternateExplosion
      : data::SoundId::Explosion;
    d.mpServiceProvider->playSound(soundId);

    spawnOneShotSprite(
      *d.mpEntityFactory,
      data::ActorID::Explosion_FX_1,
      position);
    entity.assign<AutoDestroy>(AutoDestroy::afterTimeout(0));
  }
}

}
