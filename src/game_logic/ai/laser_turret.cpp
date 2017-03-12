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

#include "data/player_data.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/entity_factory.hpp"
#include "base/math_tools.hpp"

#include "game_mode.hpp"


namespace rigel { namespace game_logic { namespace ai {

using game_logic::components::Shootable;


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

}


void configureLaserTurret(entityx::Entity& entity, const int givenScore) {
  using game_logic::components::Shootable;

  auto shootable = Shootable{2, givenScore};
  shootable.mInvincible = true;
  shootable.mEnableHitFeedback = false;
  entity.assign<Shootable>(shootable);
  entity.assign<ai::components::LaserTurret>();
}


LaserTurretSystem::LaserTurretSystem(
  entityx::Entity player,
  data::PlayerModel* pPlayerModel,
  EntityFactory* pEntityFactory,
  IGameServiceProvider* pServiceProvider
)
  : mPlayer(player)
  , mpPlayerModel(pPlayerModel)
  , mpEntityFactory(pEntityFactory)
  , mpServiceProvider(pServiceProvider)
{
}


void LaserTurretSystem::update(
  entityx::EntityManager& es,
  entityx::EventManager& events,
  entityx::TimeDelta dt
) {
  using namespace engine::components;
  using game_logic::components::PlayerDamaging;

  const auto& playerPosition = *mPlayer.component<WorldPosition>();

  es.each<components::LaserTurret, WorldPosition, Sprite, Shootable, Active>(
    [this, &playerPosition](
      entityx::Entity entity,
      components::LaserTurret& state,
      const WorldPosition& myPosition,
      Sprite& sprite,
      Shootable& shootable,
      const Active&
    ) {
      const auto isSpinning = state.mSpinningTurnsLeft > 0;
      if (!isSpinning) {
        // Flash the sprite before checking orientation and potentially firing.
        // This mirrors what the original game does. It has the effect that the
        // turret stays in the 'flashed' state for longer than one frame if the
        // player moves while it's about to fire, which seems kind of buggy.
        if (state.mNextShotCountdown < 7 && state.mNextShotCountdown % 2 != 0) {
          sprite.flashWhite();
        }

        // See if we need to re-adjust our orientation
        const auto playerIsRight = myPosition.x <= playerPosition.x;
        const auto isInPosition =
          (playerIsRight && state.mAngle == 4) ||
          (!playerIsRight && state.mAngle == 0);

        if (isInPosition) {
          // Count down and maybe fire
          --state.mNextShotCountdown;
          if (state.mNextShotCountdown <= 0) {
            state.mNextShotCountdown = 40;

            const auto facingLeft = state.mAngle == 0;
            const auto offset = facingLeft ? -2 : 2;
            mpServiceProvider->playSound(data::SoundId::EnemyLaserShot);
            mpEntityFactory->createProjectile(
              game_logic::ProjectileType::EnemyLaserShot,
              myPosition + base::Vector{offset, 0},
              facingLeft
                ? game_logic::ProjectileDirection::Left
                : game_logic::ProjectileDirection::Right);
          }
        } else {
          state.mAngle += angleAdjustment(state.mAngle, playerIsRight);
        }
      } else {
        // Spin around
        --state.mSpinningTurnsLeft;
        int rotationAmount = 0;
        if (state.mSpinningTurnsLeft > 20) {
          rotationAmount = 2;
        } else if (state.mSpinningTurnsLeft >= 10) {
          rotationAmount = 1;
        } else {
          const auto doSpin = state.mSpinningTurnsLeft % 2 == 0;
          rotationAmount = doSpin ? 1 : 0;
        }

        state.mAngle = (state.mAngle + rotationAmount) % 8;

        if (state.mAngle == 5 || state.mAngle == 6) {
          mpServiceProvider->playSound(data::SoundId::Swoosh);
        }

        if (state.mSpinningTurnsLeft <= 0) {
          // Go back to normal state
          state.mNextShotCountdown = 40;
          shootable.mInvincible = false;
          entity.assign<PlayerDamaging>(1);
        }
      }

      sprite.mFramesToRender[0] = state.mAngle;
      engine::synchronizeBoundingBoxToSprite(entity);
    });
}


void LaserTurretSystem::onEntityHit(entityx::Entity entity) {
  if (!entity.has_component<components::LaserTurret>()) {
    return;
  }

  // When hit, go into spinning mode
  entity.component<components::LaserTurret>()->mSpinningTurnsLeft = 40;

  auto& shootable = *entity.component<Shootable>();
  shootable.mHealth = 2;
  shootable.mInvincible = true;
  entity.remove<game_logic::components::PlayerDamaging>();

  mpPlayerModel->mScore += 1;
}

}}}
