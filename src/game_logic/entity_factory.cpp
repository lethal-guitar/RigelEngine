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

#include "data/game_traits.hpp"
#include "data/unit_conversions.hpp"
#include "engine/life_time_components.hpp"
#include "engine/physics_system.hpp"
#include "engine/sprite_tools.hpp"
#include "game_logic/ai/laser_turret.hpp"
#include "game_logic/ai/messenger_drone.hpp"
#include "game_logic/ai/prisoner.hpp"
#include "game_logic/ai/rocket_turret.hpp"
#include "game_logic/ai/security_camera.hpp"
#include "game_logic/ai/sliding_door.hpp"
#include "game_logic/ai/slime_blob.hpp"
#include "game_logic/ai/slime_pipe.hpp"
#include "game_logic/collectable_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/dynamic_geometry_components.hpp"
#include "game_logic/interaction/elevator.hpp"
#include "game_logic/interaction/force_field.hpp"
#include "game_logic/player_movement_system.hpp"
#include "game_logic/trigger_components.hpp"

#include <utility>


namespace ex = entityx;


namespace rigel { namespace game_logic {

using namespace data;
using namespace loader;
using namespace std;

using data::ActorID;

using namespace engine::components;
using namespace game_logic::components;


namespace {

// The game draws player projectiles after drawing all regular actors, which
// makes them appear on top of everything. But in our case, they are rendered
// using the same mechanism as the other sprites, so we have to explicitly
// assign an order (which is higher than all regular actors' draw order).
const auto PROJECTILE_DRAW_ORDER = data::GameTraits::maxDrawOrder + 1;


// Assign gravity affected physical component
void addDefaultPhysical(
  ex::Entity entity,
  const BoundingBox& boundingBox
) {
  entity.assign<Physical>(Physical{{0.0f, 0.0f}, true});
  entity.assign<BoundingBox>(boundingBox);
  entity.assign<ActivationSettings>(
    ActivationSettings::Policy::AlwaysAfterFirstActivation);
}


#include "entity_configuration.ipp"

}

EntityFactory::EntityFactory(
  engine::Renderer* pRenderer,
  ex::EntityManager* pEntityManager,
  const ActorImagePackage* pSpritePackage,
  const data::Difficulty difficulty)
  : mpRenderer(pRenderer)
  , mpEntityManager(pEntityManager)
  , mpSpritePackage(pSpritePackage)
  , mDifficulty(difficulty)
{
}


Sprite EntityFactory::createSpriteForId(const ActorID actorID) {
  const auto actorParts = actorIDListForActor(actorID);
  auto sprite = makeSpriteFromActorIDs(actorParts);
  configureSprite(sprite, actorID);
  return sprite;
}


const engine::OwningTexture& EntityFactory::getOrCreateTexture(
  const IdAndFrameNr& textureId
) {
  auto it = mTextureCache.find(textureId);
  if (it == mTextureCache.end()) {
    const auto& actorData = mpSpritePackage->loadActor(textureId.first);
    const auto& frameData = actorData.mFrames[textureId.second];
    auto texture = engine::OwningTexture(
      mpRenderer, frameData.mFrameImage);
    it = mTextureCache.emplace(textureId, std::move(texture)).first;
  }
  return it->second;
}


Sprite EntityFactory::makeSpriteFromActorIDs(const vector<ActorID>& actorIDs) {
  Sprite sprite;
  int lastFrameCount = 0;

  for (const auto ID : actorIDs) {
    const auto& actorData = mpSpritePackage->loadActor(ID);
    for (auto i=0u; i<actorData.mFrames.size(); ++i) {
      const auto& frameData = actorData.mFrames[i];
      const auto& texture = getOrCreateTexture(make_pair(ID, i));
      const auto textureRef = engine::NonOwningTexture(texture);
      sprite.mFrames.emplace_back(textureRef, frameData.mDrawOffset);
    }
    sprite.mDrawOrder = actorData.mDrawIndex;
    sprite.mFramesToRender.push_back(lastFrameCount);
    lastFrameCount += static_cast<int>(actorData.mFrames.size());
  }
  return sprite;
}


entityx::Entity EntityFactory::createSprite(
  const data::ActorID actorID,
  const bool assignBoundingBox
) {
  auto entity = mpEntityManager->create();
  auto sprite = createSpriteForId(actorID);
  entity.assign<Sprite>(sprite);

  if (assignBoundingBox) {
    entity.assign<BoundingBox>(engine::inferBoundingBox(sprite.mFrames[0]));
  }
  return entity;
}

entityx::Entity EntityFactory::createSprite(
  const data::ActorID actorID,
  const base::Vector& position,
  const bool assignBoundingBox
) {
  auto entity = createSprite(actorID, assignBoundingBox);
  entity.assign<WorldPosition>(position);
  return entity;
}


entityx::Entity EntityFactory::createProjectile(
  const ProjectileType type,
  const WorldPosition& pos,
  const ProjectileDirection direction
) {
  auto entity = mpEntityManager->create();
  auto sprite = createSpriteForId(actorIdForProjectile(type, direction));
  if (isPlayerProjectile(type)) {
    sprite.mDrawOrder = PROJECTILE_DRAW_ORDER;
  }

  const auto boundingBox = engine::inferBoundingBox(sprite.mFrames[0]);

  // TODO: Player projectiles shouldn't move on the frame they were created
  entity.assign<Active>();
  entity.assign<Sprite>(sprite);
  entity.assign<BoundingBox>(boundingBox);

  configureProjectile(entity, type, pos, direction, boundingBox);

  return entity;
}


entityx::Entity EntityFactory::createActor(
  const data::ActorID id,
  const base::Vector& position
) {
  auto entity = createSprite(id, position);
  auto& sprite = *entity.component<Sprite>();
  const auto boundingBox = engine::inferBoundingBox(sprite.mFrames[0]);

  configureEntity(entity, id, boundingBox, mDifficulty);

  return entity;
}


void EntityFactory::configureProjectile(
  entityx::Entity entity,
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

  const auto speed = speedForProjectileType(type);
  const auto damageAmount = damageForProjectileType(type);
  entity.assign<WorldPosition>(position);
  entity.assign<Physical>(
    Physical{directionToVector(direction) * speed, false});
  if (isPlayerProjectile(type)) {
    entity.assign<DamageInflicting>(damageAmount);
  } else {
    entity.assign<PlayerDamaging>(damageAmount, false, true);
  }

  entity.assign<AutoDestroy>(AutoDestroy{
    AutoDestroy::Condition::OnWorldCollision,
    AutoDestroy::Condition::OnLeavingActiveRegion});

  // For convenience, the enemy laser shot muzzle flash is created along with
  // the projectile.
  if (type == ProjectileType::EnemyLaserShot) {
    const auto muzzleFlashSpriteId = static_cast<data::ActorID>(
      direction == ProjectileDirection::Left ? 147 : 148);
    auto muzzleFlash = createSprite(muzzleFlashSpriteId);
    muzzleFlash.assign<WorldPosition>(position);
    muzzleFlash.assign<AutoDestroy>(AutoDestroy::afterTimeout(1));
  }
}


entityx::Entity EntityFactory::createEntitiesForLevel(
  const data::map::ActorDescriptionList& actors
) {
  entityx::Entity playerEntity;

  for (const auto& actor : actors) {
    // Difficulty/section markers should never appear in the actor descriptions
    // coming from the loader, as they are handled during pre-processing.
    assert(
      actor.mID != 82 && actor.mID != 83 &&
      actor.mID != 103 && actor.mID != 104);

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
      boundingBox = engine::inferBoundingBox(sprite.mFrames[0]);
      entity.assign<Sprite>(sprite);
    }

    configureEntity(entity, actor.mID, boundingBox, mDifficulty);

    const auto isPlayer = actor.mID == 5 || actor.mID == 6;
    if (isPlayer) {
      game_logic::initializePlayerEntity(entity, actor.mID == 6);
      playerEntity = entity;
    }
  }

  return playerEntity;
}

}}
