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

#include "laser_turret.hpp"

#include "base/math_tools.hpp"
#include "common/game_service_provider.hpp"
#include "data/player_model.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/entity_factory.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic::behaviors {

namespace {

int angleAdjustment(const int currentAngle, const bool playerIsRight) {
  if (playerIsRight) {
    if (currentAngle > 4) {
      return -1;
    } else if (currentAngle < 4) {
      return 1;
    }
  } else if (currentAngle > 0) {
    return -1;
  }

  return 0;
}


void performBaseHitEffect(GlobalDependencies& d, const base::Vector& position) {
  spawnFloatingOneShotSprite(
    *d.mpEntityFactory,
    data::ActorID::Shot_impact_FX,
    position + base::Vector{-1, 2});
  const auto randomChoice = d.mpRandomGenerator->gen();
  const auto soundId = randomChoice % 2 == 0
    ? data::SoundId::AlternateExplosion
    : data::SoundId::Explosion;
  d.mpServiceProvider->playSound(soundId);
}

}


void LaserTurret::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool isOnScreen,
  entityx::Entity entity
) {
  const auto& position = *entity.component<engine::components::WorldPosition>();
  const auto& playerPosition = s.mpPlayer->orientedPosition();
  auto& sprite = *entity.component<engine::components::Sprite>();

  const auto isSpinning = mSpinningTurnsLeft > 0;
  if (!isSpinning) {
    // Flash the sprite before checking orientation and potentially firing.
    // This mirrors what the original game does. It has the effect that the
    // turret stays in the 'flashed' state for longer than one frame if the
    // player moves while it's about to fire, which seems kind of buggy.
    if (mNextShotCountdown < 7 && mNextShotCountdown % 2 != 0) {
      sprite.flashWhite();
    }

    // See if we need to re-adjust our orientation
    const auto playerIsRight = position.x <= playerPosition.x;
    const auto isInPosition =
      (playerIsRight && mAngle == 4) ||
      (!playerIsRight && mAngle == 0);

    if (isInPosition) {
      // Count down and maybe fire
      --mNextShotCountdown;
      if (mNextShotCountdown <= 0) {
        mNextShotCountdown = 40;

        const auto facingLeft = mAngle == 0;
        const auto offset = facingLeft ? -2 : 2;
        d.mpServiceProvider->playSound(data::SoundId::EnemyLaserShot);
        d.mpEntityFactory->createProjectile(
          game_logic::ProjectileType::EnemyLaserShot,
          position + base::Vector{offset, 0},
          facingLeft
            ? game_logic::ProjectileDirection::Left
            : game_logic::ProjectileDirection::Right);
      }
    } else {
      mAngle += angleAdjustment(mAngle, playerIsRight);
    }
  } else {
    // Spin around
    --mSpinningTurnsLeft;
    int rotationAmount = 0;
    if (mSpinningTurnsLeft > 20) {
      rotationAmount = 2;
    } else if (mSpinningTurnsLeft >= 10) {
      rotationAmount = 1;
    } else {
      const auto doSpin = mSpinningTurnsLeft % 2 == 0;
      rotationAmount = doSpin ? 1 : 0;
    }

    mAngle = (mAngle + rotationAmount) % 8;

    if (mAngle == 5 || mAngle == 6) {
      d.mpServiceProvider->playSound(data::SoundId::Swoosh);
    }

    if (mSpinningTurnsLeft <= 0) {
      // Go back to normal state
      mNextShotCountdown = 40;

      auto& shootable = *entity.component<game_logic::components::Shootable>();
      shootable.mInvincible = false;
      entity.assign<game_logic::components::PlayerDamaging>(1);
    }
  }

  sprite.mFramesToRender[0] = mAngle;
  engine::synchronizeBoundingBoxToSprite(entity);
}


void LaserTurret::onHit(
  GlobalDependencies& d,
  GlobalState& s,
  const base::Point<float>& inflictorVelocity,
  entityx::Entity entity
) {
  // When hit, go into spinning mode
  mSpinningTurnsLeft = 40;

  auto& shootable = *entity.component<game_logic::components::Shootable>();
  shootable.mHealth = 2;
  shootable.mInvincible = true;
  entity.remove<game_logic::components::PlayerDamaging>();

  s.mpPlayer->model().giveScore(1);

  performBaseHitEffect(
    d, *entity.component<engine::components::WorldPosition>());
}


void LaserTurret::onKilled(
  GlobalDependencies& d,
  GlobalState& s,
  const base::Point<float>& inflictorVelocity,
  entityx::Entity entity
) {
  const auto& position = *entity.component<engine::components::WorldPosition>();

  performBaseHitEffect(d, position);

  const auto shotFromRight = inflictorVelocity.x < 0.0f;
  const auto shotFromLeft = inflictorVelocity.x > 0.0f;
  const auto debrisMovement = shotFromRight
    ? SpriteMovement::FlyUpperLeft
    : (shotFromLeft)
      ? SpriteMovement::FlyUpperRight
      : SpriteMovement::FlyUp;
  spawnMovingEffectSprite(
    *d.mpEntityFactory,
    data::ActorID::Laser_turret,
    debrisMovement,
    position);
}

}
