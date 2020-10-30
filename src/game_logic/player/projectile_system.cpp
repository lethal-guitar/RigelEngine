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
#include "engine/collision_checker.hpp"
#include "engine/entity_tools.hpp"
#include "engine/life_time_components.hpp"
#include "engine/physical_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/ientity_factory.hpp"


namespace rigel::game_logic::player {

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
  IEntityFactory& entityFactory,
  const base::Vector& position,
  const base::Point<float> velocity
) {
  const auto debrisPosition = position + regularShotDebrisOffset(velocity);
  spawnFloatingOneShotSprite(entityFactory, data::ActorID::Shot_impact_FX, debrisPosition);
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
  IEntityFactory& entityFactory,
  const base::Vector& position,
  const base::Point<float> velocity
) {
  const auto offset = rocketSmokeOffset(velocity);
  spawnOneShotSprite(entityFactory, data::ActorID::Smoke_puff_FX, position + offset);
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
  IEntityFactory& entityFactory,
  const base::Vector& position,
  const engine::components::BoundingBox& bbox,
  const base::Point<float> velocity
) {
  const auto offset = rocketWallImpactOffset(velocity);
  spawnOneShotSprite(
    entityFactory, data::ActorID::Explosion_FX_2, position + offset);
  spawnFireEffect(
    entityFactory.entityManager(),
    position,
    bbox,
    data::ActorID::Shot_impact_FX);
}


void spawnEnemyImpactEffect(
  IEntityFactory& entityFactory,
  const base::Vector& position
) {
  spawnOneShotSprite(
    entityFactory,
    data::ActorID::Explosion_FX_2,
    position + base::Vector{-3, 3});
}

}


ProjectileSystem::ProjectileSystem(
  IEntityFactory* pEntityFactory,
  IGameServiceProvider* pServiceProvider,
  const engine::CollisionChecker* pCollisionChecker,
  const data::map::Map* pMap
)
  : mpEntityFactory(pEntityFactory)
  , mpServiceProvider(pServiceProvider)
  , mpCollisionChecker(pCollisionChecker)
  , mpMap(pMap)
{
}


void ProjectileSystem::update(entityx::EntityManager& es) {
  using namespace engine::components;
  using namespace game_logic::components;

  es.each<
    PlayerProjectile,
    MovingBody,
    WorldPosition,
    BoundingBox,
    DamageInflicting,
    Active
  >(
    [this](
      entityx::Entity entity,
      PlayerProjectile& projectile,
      MovingBody& body,
      const WorldPosition& position,
      const BoundingBox& bbox,
      DamageInflicting& damage,
      const Active&
    ) {
      if (!body.mIsActive) {
        body.mIsActive = true;
      }

      if (
        projectile.mType == PlayerProjectile::Type::Laser ||
        projectile.mType == PlayerProjectile::Type::ShipLaser ||
        projectile.mType == PlayerProjectile::Type::ReactorDebris
      ) {
        if (
          damage.mHasCausedDamage &&
          (projectile.mType == PlayerProjectile::Type::ReactorDebris ||
           projectile.mType == PlayerProjectile::Type::ShipLaser)
        ) {
          damage.mHasCausedDamage = false;
          spawnEnemyImpactEffect(*mpEntityFactory, position);
        }

        // These projectiles pass through enemies and walls, so there's nothing
        // more we have to do.
        return;
      }

      const auto isRocket = projectile.mType == PlayerProjectile::Type::Rocket;

      // Check if we hit an enemy, deactivate if so.
      if (damage.mHasCausedDamage) {
        if (isRocket) {
          spawnEnemyImpactEffect(*mpEntityFactory, position);
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
      if (isCollidingWithWorld(engine::toWorldSpace(bbox, position))) {
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
    (position.x + bbox.size.width) < mpMap->width();
  if (!insideMap) {
    return;
  }

  if (isRocket) {
    mpServiceProvider->playSound(data::SoundId::Explosion);
    spawnRocketWallImpactEffect(*mpEntityFactory, position, bbox, velocity);
  } else {
    spawnRegularShotImpactEffect(*mpEntityFactory, position, velocity);
  }
}


bool ProjectileSystem::isCollidingWithWorld(const base::Rect<int>& bbox) {
  // Collision detection for projectiles works differently than for regular
  // physics objects, and is a bit weird. It only works correctly for "flat"
  // projectiles, which are 1 unit wide when vertical and 1 unit tall when
  // horizontal. This applies to the player's rockets and regular shot, which
  // happen to be the only types of projectiles which collide with the world.
  // I could imagine that this was done as a performance optimization, and that
  // the flame thrower passing through walls is mainly because doing collision
  // detection on non-flat objects would have been too expensive. But there's
  // no way to know for sure, this is just a guess on my part.
  //
  // The way this works is that the bottom-most row and left-most column of the
  // projectile are tested for collision against any type of solid edge. If
  // we have a 4x4 bounding box, this would look like the following:
  //
  //     +---+---+---+---+
  //     | X |   |   |   |
  //     +---------------+
  //     | X |   |   |   |
  //     +---------------+
  //     | X |   |   |   |
  //     +---------------+
  //     | X | X | X | X |
  //     +---+---+---+---+
  //
  // All the tiles marked with an X are checked for collision, the others are
  // ignored. This would not work correctly for a non-flat projectile that's
  // flying upwards or to the right.
  //
  // In addition, collision detection is negative if _any_ of the tested tiles
  // is a 'composite' tile (content on both layers). This can cause projectiles
  // to fly through walls in very specific circumstances (multiple composite
  // tiles followed by a 1 unit wide solid wall). It seems like a bug, but to
  // replicate the original game's behavior, we do the same here.
  auto hasCompositeTileAt = [this](const int x, const int y) {
    if (x < 0 || x >= mpMap->width() || y < 0 || y >= mpMap->height()) {
      return false;
    }

    return mpMap->tileAt(0, x, y) != 0 && mpMap->tileAt(1, x, y) != 0;
  };

  auto hasCompositeTilesBottomRow = [&]() {
    for (int x = bbox.left(); x <= bbox.right(); ++x) {
      if (hasCompositeTileAt(x, bbox.bottom())) {
        return true;
      }
    }

    return false;
  };

  auto hasCompositeTilesLeftColumn = [&]() {
    for (int y = bbox.top(); y <= bbox.bottom(); ++y) {
      if (hasCompositeTileAt(bbox.left(), y)) {
        return true;
      }
    }

    return false;
  };


  if (bbox.top() < 0 || bbox.bottom() == 0) {
    return false;
  }

  const auto hasCollisionOnBottomRow = mpCollisionChecker->testHorizontalSpan(
    bbox.left(),
    bbox.right(),
    bbox.bottom(),
    data::map::SolidEdge::any());
  const auto hasCollisionOnLeftColumn = mpCollisionChecker->testVerticalSpan(
    bbox.top(),
    bbox.bottom(),
    bbox.left(),
    data::map::SolidEdge::any());
  return
    (hasCollisionOnBottomRow || hasCollisionOnLeftColumn) &&
    !hasCompositeTilesBottomRow() &&
    !hasCompositeTilesLeftColumn();
}

}
