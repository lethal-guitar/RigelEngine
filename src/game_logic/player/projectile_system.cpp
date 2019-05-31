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

#include "projectile_system.hpp"

#include "common/game_service_provider.hpp"
#include "data/map.hpp"
#include "engine/entity_tools.hpp"
#include "engine/life_time_components.hpp"
#include "engine/physical_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/entity_factory.hpp"


namespace rigel { namespace game_logic { namespace player {

namespace {

void deactivateProjectile(entityx::Entity entity) {
  using engine::components::AutoDestroy;
  using engine::components::MovingBody;
  using game_logic::components::DamageInflicting;

  engine::reassign<AutoDestroy>(entity, AutoDestroy::afterTimeout(1));
  entity.remove<DamageInflicting>();
  entity.remove<MovingBody>();
}


base::Vector regularShotDebrisOffset(const base::Point<float>& velocity) {
  const auto isHorizontal = velocity.x != 0.0f;
  return {isHorizontal ? 0 : -1, 1};
}


void spawnRegularShotImpactEffect(
  EntityFactory& entityFactory,
  const base::Vector& position,
  const base::Point<float> velocity
) {
  const auto debrisPosition = position + regularShotDebrisOffset(velocity);
  spawnFloatingOneShotSprite(entityFactory, 3, debrisPosition);
}


base::Vector rocketSmokeOffset(const base::Point<float>& velocity) {
  const auto isFacingOpposite = velocity.x < 0.0f || velocity.y > 0.0f;
  if (isFacingOpposite) {
    const auto isHorizontal = velocity.x != 0.0f;
    if (isHorizontal) {
      return {3, 0};
    } else {
      return {0, 3};
    }
  }

  return {};
}


void generateRocketSmoke(
  EntityFactory& entityFactory,
  const base::Vector& position,
  const base::Point<float> velocity
) {
  const auto offset = rocketSmokeOffset(velocity);
  spawnOneShotSprite(entityFactory, 11, position + offset);
}


base::Vector rocketWallImpactOffset(const base::Point<float>& velocity) {
  const auto isHorizontal = velocity.x != 0.0f;
  if (isHorizontal) {
    return {-1, 2};
  } else {
    return {-2, 1};
  }
}


void spawnRocketWallImpactEffect(
  EntityFactory& entityFactory,
  const base::Vector& position,
  const base::Point<float> velocity
) {
  const auto offset = rocketWallImpactOffset(velocity);
  spawnOneShotSprite(entityFactory, 2, position + offset);
}


void spawnRocketEnemyImpactEffect(
  EntityFactory& entityFactory,
  const base::Vector& position
) {
  spawnOneShotSprite(entityFactory, 2, position + base::Vector{-3, 3});
}

}


ProjectileSystem::ProjectileSystem(
  EntityFactory* pEntityFactory,
  IGameServiceProvider* pServiceProvider,
  data::map::Map& map
)
  : mpEntityFactory(pEntityFactory)
  , mpServiceProvider(pServiceProvider)
  , mMapSize(map.width(), map.height())
{
}


void ProjectileSystem::update(entityx::EntityManager& es) {
  using namespace engine::components;
  using namespace game_logic::components;

  es.each<
    PlayerProjectile,
    MovingBody,
    WorldPosition,
    DamageInflicting,
    Active
  >(
    [this](
      entityx::Entity entity,
      PlayerProjectile& projectile,
      MovingBody& body,
      const WorldPosition& position,
      DamageInflicting& damage,
      const Active&
    ) {
      if (!body.mIsActive) {
        body.mIsActive = true;
      }

      // TODO: Impact explosions for reactor debris?
      if (
        projectile.mType == PlayerProjectile::Type::Laser ||
        projectile.mType == PlayerProjectile::Type::ReactorDebris
      ) {
        // These projectiles pass through enemies and walls, so there's nothing
        // we have to do.
        return;
      }

      const auto isRocket = projectile.mType == PlayerProjectile::Type::Rocket;

      // Check if we hit an enemy, deactivate if so.
      if (damage.mHasCausedDamage) {
        if (isRocket) {
          spawnRocketEnemyImpactEffect(*mpEntityFactory, position);
        }
        deactivateProjectile(entity);
        return;
      }

      if (projectile.mType == PlayerProjectile::Type::Flame) {
        // The flame thrower passes through walls, so no further checking
        // necessary.
        return;
      }

      // Check if we hit a wall, and deactivate if so.
      if (entity.has_component<CollidedWithWorld>()) {
        spawnWallImpactEffect(entity, position, body.mVelocity, isRocket);
        deactivateProjectile(entity);
        return;
      }

      // If projectile survived all of the above, generate smoke for rockets.
      if (isRocket) {
        generateRocketSmoke(*mpEntityFactory, position, body.mVelocity);
      }
    });
}


void ProjectileSystem::spawnWallImpactEffect(
  entityx::Entity entity,
  const base::Vector& position,
  const base::Point<float>& velocity,
  const bool isRocket
) {
  const auto& bbox = *entity.component<engine::components::BoundingBox>();
  const auto insideMap =
    position.x >= 0 &&
    (position.x + bbox.size.width) < mMapSize.width;
  if (!insideMap) {
    return;
  }

  if (isRocket) {
    mpServiceProvider->playSound(data::SoundId::Explosion);
    spawnRocketWallImpactEffect(*mpEntityFactory, position, velocity);
  } else {
    spawnRegularShotImpactEffect(*mpEntityFactory, position, velocity);
  }
}

}}}
