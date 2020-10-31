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

#include "entity_factory.hpp"

#include "base/container_utils.hpp"
#include "common/game_service_provider.hpp"
#include "data/game_traits.hpp"
#include "data/unit_conversions.hpp"
#include "engine/life_time_components.hpp"
#include "engine/physics_system.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/sprite_factory.hpp"
#include "engine/sprite_tools.hpp"
#include "game_logic/actor_tag.hpp"
#include "game_logic/behavior_controller.hpp"
#include "game_logic/collectable_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/dynamic_geometry_components.hpp"
#include "game_logic/effect_actor_components.hpp"
#include "game_logic/effect_components.hpp"
#include "game_logic/enemies/big_green_cat.hpp"
#include "game_logic/enemies/blue_guard.hpp"
#include "game_logic/enemies/bomber_plane.hpp"
#include "game_logic/enemies/boss_episode_1.hpp"
#include "game_logic/enemies/boss_episode_2.hpp"
#include "game_logic/enemies/boss_episode_3.hpp"
#include "game_logic/enemies/boss_episode_4.hpp"
#include "game_logic/enemies/ceiling_sucker.hpp"
#include "game_logic/enemies/enemy_rocket.hpp"
#include "game_logic/enemies/eyeball_thrower.hpp"
#include "game_logic/enemies/flame_thrower_bot.hpp"
#include "game_logic/enemies/floating_laser_bot.hpp"
#include "game_logic/enemies/grabber_claw.hpp"
#include "game_logic/enemies/green_bird.hpp"
#include "game_logic/enemies/hover_bot.hpp"
#include "game_logic/enemies/laser_turret.hpp"
#include "game_logic/enemies/messenger_drone.hpp"
#include "game_logic/enemies/prisoner.hpp"
#include "game_logic/enemies/red_bird.hpp"
#include "game_logic/enemies/rigelatin_soldier.hpp"
#include "game_logic/enemies/rocket_turret.hpp"
#include "game_logic/enemies/security_camera.hpp"
#include "game_logic/enemies/simple_walker.hpp"
#include "game_logic/enemies/slime_blob.hpp"
#include "game_logic/enemies/small_flying_ship.hpp"
#include "game_logic/enemies/snake.hpp"
#include "game_logic/enemies/spider.hpp"
#include "game_logic/enemies/spike_ball.hpp"
#include "game_logic/enemies/spiked_green_creature.hpp"
#include "game_logic/enemies/unicycle_bot.hpp"
#include "game_logic/enemies/wall_walker.hpp"
#include "game_logic/enemies/watch_bot.hpp"
#include "game_logic/hazards/lava_fountain.hpp"
#include "game_logic/hazards/slime_pipe.hpp"
#include "game_logic/hazards/smash_hammer.hpp"
#include "game_logic/interactive/blowing_fan.hpp"
#include "game_logic/interactive/elevator.hpp"
#include "game_logic/interactive/enemy_radar.hpp"
#include "game_logic/interactive/force_field.hpp"
#include "game_logic/interactive/item_container.hpp"
#include "game_logic/interactive/locked_door.hpp"
#include "game_logic/interactive/missile.hpp"
#include "game_logic/interactive/respawn_checkpoint.hpp"
#include "game_logic/interactive/sliding_door.hpp"
#include "game_logic/interactive/super_force_field.hpp"
#include "game_logic/interactive/tile_burner.hpp"
#include "game_logic/player/level_exit_trigger.hpp"
#include "game_logic/player/ship.hpp"

#include <tuple>
#include <utility>


namespace ex = entityx;


namespace rigel::game_logic {

using namespace data;
using namespace loader;
using namespace std;

using data::ActorID;

using namespace engine::components;
using namespace game_logic::components;


namespace {

// Assign gravity affected moving body component
template<typename EntityLike>
void addDefaultMovingBody(
  EntityLike& entity,
  const BoundingBox& boundingBox
) {
  using namespace engine::components::parameter_aliases;

  entity.template assign<MovingBody>(
    MovingBody{Velocity{0.0f, 0.0f}, GravityAffected{true}});
  entity.template assign<BoundingBox>(boundingBox);
  entity.template assign<ActivationSettings>(
    ActivationSettings::Policy::AlwaysAfterFirstActivation);
}


auto toPlayerProjectileType(const ProjectileType type) {
  using PType = components::PlayerProjectile::Type;
  static_assert(
    int(PType::Normal) == int(ProjectileType::PlayerRegularShot) &&
    int(PType::Laser) == int(ProjectileType::PlayerLaserShot) &&
    int(PType::Rocket) == int(ProjectileType::PlayerRocketShot) &&
    int(PType::Flame) == int(ProjectileType::PlayerFlameShot) &&
    int(PType::ShipLaser) == int(ProjectileType::PlayerShipLaserShot) &&
    int(PType::ReactorDebris) == int(ProjectileType::ReactorDebris));
  return static_cast<PType>(static_cast<int>(type));
}


base::Vector adjustedPosition(
  const ProjectileType type,
  WorldPosition position,
  const ProjectileDirection direction,
  const BoundingBox& boundingBox
) {
  const auto isGoingLeft = direction == ProjectileDirection::Left;

  // Position adjustment for the flame thrower shot
  if (type == ProjectileType::PlayerFlameShot) {
    if (isHorizontal(direction)) {
      position.y += 1;
    } else {
      position.x -= 1;
    }
  }

  // Position adjustment for left-facing projectiles. We want the incoming
  // position to always represent the projectile's origin, which means we need
  // to adjust the position by the projectile's length to match the left-bottom
  // corner positioning system.
  if (isHorizontal(direction) && isGoingLeft) {
    position.x -= boundingBox.size.width - 1;

    if (type == ProjectileType::PlayerFlameShot) {
      position.x += 3;
    }
  }

  return position;
}


const base::Point<float> FLY_RIGHT[] = {
  {3.0f, 0.0f},
  {3.0f, 0.0f},
  {3.0f, 0.0f},
  {2.0f, 0.0f},
  {2.0f, 1.0f},
  {2.0f, 1.0f},
  {2.0f, 2.0f},
  {1.0f, 2.0f},
  {1.0f, 3.0f},
  {1.0f, 3.0f}
};


const base::Point<float> FLY_UPPER_RIGHT[] = {
  {3.0f, -3.0f},
  {2.0f, -2.0f},
  {2.0f, -1.0f},
  {1.0f,  0.0f},
  {1.0f,  0.0f},
  {1.0f,  1.0f},
  {1.0f,  2.0f},
  {1.0f,  2.0f},
  {1.0f,  3.0f},
  {1.0f,  3.0f}
};


const base::Point<float> FLY_UP[] = {
  {0.0f, -3.0f},
  {0.0f, -2.0f},
  {0.0f, -2.0f},
  {0.0f, -1.0f},
  {0.0f, 0.0f},
  {0.0f, 1.0f},
  {0.0f, 1.0f},
  {0.0f, 2.0f},
  {0.0f, 3.0f},
  {0.0f, 3.0f}
};


const base::Point<float> FLY_UPPER_LEFT[] = {
  {-3.0f, -3.0f},
  {-2.0f, -2.0f},
  {-2.0f, -1.0f},
  {-1.0f, 0.0f},
  {-1.0f, 0.0f},
  {-1.0f, 1.0f},
  {-1.0f, 2.0f},
  {-1.0f, 3.0f},
  {-1.0f, 4.0f},
  {-1.0f, 4.0f}
};


const base::Point<float> FLY_LEFT[] = {
  {-3.0f, 0.0f},
  {-3.0f, 0.0f},
  {-3.0f, 0.0f},
  {-2.0f, 0.0f},
  {-2.0f, 1.0f},
  {-2.0f, 1.0f},
  {-2.0f, 2.0f},
  {-1.0f, 3.0f},
  {-1.0f, 3.0f},
  {-1.0f, 3.0f}
};


const base::Point<float> FLY_DOWN[] = {
  {0.0f, 1.0f},
  {0.0f, 2.0f},
  {0.0f, 2.0f},
  {0.0f, 2.0f},
  {0.0f, 3.0f},
  {0.0f, 3.0f},
  {0.0f, 3.0f},
  {0.0f, 3.0f},
  {0.0f, 3.0f},
  {0.0f, 3.0f}
};


const base::Point<float> SWIRL_AROUND[] = {
  {-2.0f, 1.0f},
  {-2.0f, 1.0f},
  {-2.0f, 1.0f},
  {-1.0f, 1.0f},
  {0.0f, 1.0f},
  {1.0f, 1.0f},
  {2.0f, 0.0f},
  {1.0f, -1.0f},
  {-2.0f, -1.0f},
  {-2.0f, 1.0f}
};


const base::ArrayView<base::Point<float>> MOVEMENT_SEQUENCES[] = {
  FLY_RIGHT,
  FLY_UPPER_RIGHT,
  FLY_UP,
  FLY_UPPER_LEFT,
  FLY_LEFT,
  FLY_DOWN,
  SWIRL_AROUND
};

}


#include "entity_configuration.ipp"


EntityFactory::EntityFactory(
  engine::ISpriteFactory* pSpriteFactory,
  ex::EntityManager* pEntityManager,
  IGameServiceProvider* pServiceProvider,
  engine::RandomNumberGenerator* pRandomGenerator,
  const data::Difficulty difficulty)
  : mpSpriteFactory(pSpriteFactory)
  , mpEntityManager(pEntityManager)
  , mpServiceProvider(pServiceProvider)
  , mpRandomGenerator(pRandomGenerator)
  , mDifficulty(difficulty)
{
}


Sprite EntityFactory::createSpriteForId(const ActorID actorID) {
  return mpSpriteFactory->createSprite(actorID);
}


entityx::Entity EntityFactory::spawnSprite(
  const data::ActorID actorID,
  const bool assignBoundingBox
) {
  auto entity = mpEntityManager->create();
  auto sprite = createSpriteForId(actorID);
  entity.assign<Sprite>(sprite);

  if (assignBoundingBox) {
    entity.assign<BoundingBox>(mpSpriteFactory->actorFrameRect(actorID, 0));
  }

  if (actorID == data::ActorID::Explosion_FX_1) {
    // TODO: Eliminate duplication with code in effects_system.cpp
    const auto randomChoice = mpRandomGenerator->gen();
    const auto soundId = randomChoice % 2 == 0
      ? data::SoundId::AlternateExplosion
      : data::SoundId::Explosion;
    mpServiceProvider->playSound(soundId);
  }

  return entity;
}

entityx::Entity EntityFactory::spawnSprite(
  const data::ActorID actorID,
  const base::Vector& position,
  const bool assignBoundingBox
) {
  auto entity = spawnSprite(actorID, assignBoundingBox);
  entity.assign<WorldPosition>(position);
  return entity;
}


entityx::Entity EntityFactory::spawnProjectile(
  const ProjectileType type,
  const WorldPosition& pos,
  const ProjectileDirection direction
) {
  auto entity = spawnSprite(actorIdForProjectile(type, direction), true);
  entity.assign<Active>();

  const auto& boundingBox = *entity.component<BoundingBox>();
  entity.assign<WorldPosition>(adjustedPosition(
    type, pos, direction, boundingBox));

  configureProjectile(
    entity,
    type,
    pos,
    direction,
    boundingBox);

  return entity;
}


entityx::Entity EntityFactory::spawnActor(
  const data::ActorID id,
  const base::Vector& position
) {
  auto entity = spawnSprite(id, position);
  const auto boundingBox = mpSpriteFactory->actorFrameRect(id, 0);

  configureEntity(entity, id, boundingBox);

  return entity;
}


void EntityFactory::configureProjectile(
  entityx::Entity entity,
  const ProjectileType type,
  WorldPosition position,
  const ProjectileDirection direction,
  const BoundingBox& boundingBox
) {
  using namespace engine::components::parameter_aliases;
  using namespace game_logic::components::parameter_aliases;

  const auto speed = speedForProjectileType(type);
  const auto damageAmount = damageForProjectileType(type);

  entity.assign<MovingBody>(
    Velocity{directionToVector(direction) * speed},
    GravityAffected{false});
  // Some player projectiles do have collisions with walls, but that's
  // handled by player::ProjectileSystem.
  entity.component<MovingBody>()->mIgnoreCollisions = true;
  entity.component<MovingBody>()->mIsActive = false;

  entity.assign<DamageInflicting>(damageAmount, DestroyOnContact{false});
  entity.assign<PlayerProjectile>(toPlayerProjectileType(type));

  if (type == ProjectileType::PlayerShipLaserShot) {
    entity.assign<AnimationLoop>(1);
  }

  entity.assign<AutoDestroy>(AutoDestroy{
    AutoDestroy::Condition::OnLeavingActiveRegion});
}


void EntityFactory::createEntitiesForLevel(
  const data::map::ActorDescriptionList& actors
) {
  for (const auto& actor : actors) {
    // Difficulty/section markers should never appear in the actor descriptions
    // coming from the loader, as they are handled during pre-processing.
    assert(
      actor.mID != ActorID::META_Appear_only_in_med_hard_difficulty && actor.mID != ActorID::META_Appear_only_in_hard_difficulty &&
      actor.mID != ActorID::META_Dynamic_geometry_marker_1 && actor.mID != ActorID::META_Dynamic_geometry_marker_2);

    auto entity = mpEntityManager->create();

    auto position = actor.mPosition;
    if (actor.mAssignedArea) {
      // For dynamic geometry, the original position refers to the top-left
      // corner of the assigned area, but it refers to the bottom-left corner
      // for all other entities. Adjust the position here so that it's also
      // bottom-left.
      position.y += actor.mAssignedArea->size.height - 1;
    }
    entity.assign<WorldPosition>(position);

    BoundingBox boundingBox;
    if (actor.mAssignedArea) {
      const auto mapSectionRect = *actor.mAssignedArea;
      entity.assign<MapGeometryLink>(mapSectionRect);

      boundingBox = mapSectionRect;
      boundingBox.topLeft = {0, 0};
    } else if (hasAssociatedSprite(actor.mID)) {
      const auto sprite = createSpriteForId(actor.mID);
      boundingBox = mpSpriteFactory->actorFrameRect(actor.mID, 0);
      entity.assign<Sprite>(sprite);
    }

    configureEntity(entity, actor.mID, boundingBox);
  }
}


entityx::Entity spawnOneShotSprite(
  IEntityFactory& factory,
  const ActorID id,
  const base::Vector& position
) {
  auto entity = factory.spawnSprite(id, position, true);
  const auto numAnimationFrames = static_cast<int>(
    entity.component<Sprite>()->mpDrawData->mFrames.size());
  if (numAnimationFrames > 1) {
    engine::startAnimationLoop(entity, 1, 0, std::nullopt);
  }
  entity.assign<AutoDestroy>(AutoDestroy::afterTimeout(numAnimationFrames));
  assignSpecialEffectSpriteProperties(entity, id);

  return entity;
}


entityx::Entity spawnFloatingOneShotSprite(
  IEntityFactory& factory,
  const data::ActorID id,
  const base::Vector& position
) {
  using namespace engine::components::parameter_aliases;

  auto entity = spawnOneShotSprite(factory, id, position);
  entity.assign<MovingBody>(MovingBody{
    Velocity{0, -1.0f},
    GravityAffected{false},
    IgnoreCollisions{true}});
  return entity;
}


entityx::Entity spawnMovingEffectSprite(
  IEntityFactory& factory,
  const ActorID id,
  const SpriteMovement movement,
  const base::Vector& position
) {
  auto entity = factory.spawnSprite(id, position, true);
  configureMovingEffectSprite(entity, movement);
  if (entity.component<Sprite>()->mpDrawData->mFrames.size() > 1) {
    entity.assign<AnimationLoop>(1);
  }
  assignSpecialEffectSpriteProperties(entity, id);
  return entity;
}


void spawnFloatingScoreNumber(
  IEntityFactory& factory,
  const ScoreNumberType type,
  const base::Vector& position
) {
  using namespace engine::components::parameter_aliases;

  auto entity = factory.spawnSprite(scoreNumberActor(type), position, true);
  engine::startAnimationSequence(entity, SCORE_NUMBER_ANIMATION_SEQUENCE);
  entity.assign<MovementSequence>(SCORE_NUMBER_MOVE_SEQUENCE);
  entity.assign<MovingBody>(
    Velocity{},
    GravityAffected{false},
    IgnoreCollisions{true});
  entity.assign<AutoDestroy>(AutoDestroy::afterTimeout(SCORE_NUMBER_LIFE_TIME));
  entity.assign<Active>();
}


void spawnFireEffect(
  entityx::EntityManager& entityManager,
  const base::Vector& position,
  const BoundingBox& coveredArea,
  const data::ActorID actorToSpawn
) {
  // TODO: The initial offset should be based on the size of the actor
  // that's to be spawned. Currently, it's hard-coded for actor ID 3
  // (small explosion).
  auto offset = base::Vector{-1, 1};

  auto spawner = entityManager.create();
  SpriteCascadeSpawner spawnerConfig;
  spawnerConfig.mBasePosition = position + offset + coveredArea.topLeft;
  spawnerConfig.mCoveredArea = coveredArea.size;
  spawnerConfig.mActorId = actorToSpawn;
  spawner.assign<SpriteCascadeSpawner>(spawnerConfig);
  spawner.assign<AutoDestroy>(AutoDestroy::afterTimeout(18));
}


void spawnEnemyLaserShot(
  IEntityFactory& factory,
  base::Vector position,
  const engine::components::Orientation orientation
) {
  const auto isFacingLeft = orientation == Orientation::Left;
  if (isFacingLeft) {
    position.x -= 1;
  }

  auto entity = factory.spawnActor(
    isFacingLeft
      ? data::ActorID::Enemy_laser_shot_LEFT
      : data::ActorID::Enemy_laser_shot_RIGHT,
    position);
  entity.assign<Active>();

  // For convenience, the enemy laser shot muzzle flash is created along with
  // the projectile.
  const auto muzzleFlashSpriteId = isFacingLeft
    ? data::ActorID::Enemy_laser_muzzle_flash_1
    : data::ActorID::Enemy_laser_muzzle_flash_2;
  auto muzzleFlash = factory.spawnSprite(muzzleFlashSpriteId, position);
  muzzleFlash.assign<AutoDestroy>(AutoDestroy::afterTimeout(1));
}

}
