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

#include "rocket_turret.hpp"

#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/entity_factory.hpp"
#include "base/math_tools.hpp"

#include "game_mode.hpp"


namespace rigel { namespace game_logic { namespace ai {

namespace {

using namespace engine::components;


const base::Vector OFFSET_BY_ORIENTATION[] = {
  {1, -1},
  {1, -2},
  {2, -1}
};


const ProjectileDirection DIRECTION_BY_ORIENTATION[] = {
  ProjectileDirection::Left,
  ProjectileDirection::Up,
  ProjectileDirection::Right
};


auto determineOrientation(
  const base::Vector& myPosition,
  const base::Vector& playerPosition
) {
  using Orientation = components::RocketTurret::Orientation;

  if (playerPosition.x + 3 <= myPosition.x) {
    return Orientation::Left;
  } else if (playerPosition.x - 3 >= myPosition.x) {
    return Orientation::Right;
  } else if (playerPosition.y <= myPosition.y) {
    return Orientation::Top;
  }

  return Orientation::Left;
}

}


RocketTurretSystem::RocketTurretSystem(
  entityx::Entity player,
  EntityFactory* pEntityFactory,
  IGameServiceProvider* pServiceProvider
)
  : mPlayer(player)
  , mpEntityFactory(pEntityFactory)
  , mpServiceProvider(pServiceProvider)
{
}


void RocketTurretSystem::update(
  entityx::EntityManager& es,
  entityx::EventManager& events,
  entityx::TimeDelta dt
) {
  const auto& playerPosition = *mPlayer.component<WorldPosition>();

  es.each<components::RocketTurret, WorldPosition, Sprite, Active>(
    [this, &playerPosition](
      entityx::Entity entity,
      components::RocketTurret& state,
      const WorldPosition& myPosition,
      Sprite& sprite,
      const Active&
    ) {
      if (state.mNeedsReorientation) {
        state.mOrientation = determineOrientation(myPosition, playerPosition);
        state.mNeedsReorientation = false;
      } else {
        ++state.mNextShotCountdown;
        if (state.mNextShotCountdown >= 25) {
          state.mNextShotCountdown = 0;
          state.mNeedsReorientation = true;

          fireRocket(myPosition, state.mOrientation);
        }
      }

      sprite.mFramesToRender[0] = static_cast<int>(state.mOrientation);
      engine::synchronizeBoundingBoxToSprite(entity);
    });
}


void RocketTurretSystem::fireRocket(
  const base::Vector& myPosition,
  components::RocketTurret::Orientation myOrientation
) {
  const auto orientationIndex = static_cast<int>(myOrientation);
  auto projectile = mpEntityFactory->createProjectile(
    game_logic::ProjectileType::EnemyRocket,
    myPosition + OFFSET_BY_ORIENTATION[orientationIndex],
    DIRECTION_BY_ORIENTATION[orientationIndex]);

  projectile.assign<game_logic::components::Shootable>(1, 10);
  projectile.component<Sprite>()->mFramesToRender.push_back(0);
  projectile.assign<AnimationLoop>(1, 1, 2, 0);

  mpServiceProvider->playSound(data::SoundId::FlameThrowerShot);
}

}}}
