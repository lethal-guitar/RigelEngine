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

#include "data/unit_conversions.hpp"
#include "engine/base_components.hpp"
#include "engine/physics_system.hpp"
#include "game_logic/collectable_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/dynamic_geometry_components.hpp"
#include "game_logic/player_movement_system.hpp"
#include "game_logic/trigger_components.hpp"

#include <utility>


namespace ex = entityx;


namespace rigel { namespace game_logic {

using namespace data;
using namespace loader;
using namespace std;

using data::ActorID;

using engine::components::Animated;
using engine::components::AnimationSequence;
using engine::components::BoundingBox;
using engine::components::DrawTopMost;
using engine::components::Physical;
using engine::components::Sprite;
using engine::components::SpriteFrame;
using engine::components::WorldPosition;
using namespace game_logic::components;


namespace {

BoundingBox inferBoundingBox(const SpriteFrame& sprite) {
  const auto dimensionsInTiles = pixelExtentsToTileExtents(
    sprite.mImage.extents());

  return {sprite.mDrawOffset, dimensionsInTiles};
}


// Assign gravity affected physical component
void addDefaultPhysical(
  ex::Entity entity,
  const BoundingBox& boundingBox
) {
  entity.assign<Physical>(Physical{{0.0f, 0.0f}, true});
  entity.assign<BoundingBox>(boundingBox);
}


#include "entity_configuration.ipp"

}

EntityFactory::EntityFactory(
  SDL_Renderer* pRenderer,
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


const sdl_utils::OwningTexture& EntityFactory::getOrCreateTexture(
  const IdAndFrameNr& textureId
) {
  auto it = mTextureCache.find(textureId);
  if (it == mTextureCache.end()) {
    const auto& actorData = mpSpritePackage->loadActor(textureId.first);
    const auto& frameData = actorData.mFrames[textureId.second];
    auto texture = sdl_utils::OwningTexture(
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
      const auto textureRef = sdl_utils::NonOwningTexture(texture);
      sprite.mFrames.emplace_back(textureRef, frameData.mDrawOffset);
    }
    sprite.mDrawOrder = actorData.mDrawIndex;
    sprite.mFramesToRender.push_back(lastFrameCount);
    lastFrameCount += static_cast<int>(actorData.mFrames.size());
  }
  return sprite;
}


entityx::Entity EntityFactory::createProjectile(
  const ProjectileType type,
  const WorldPosition& pos,
  const base::Point<float>& directionVector
) {
  auto entity = mpEntityManager->create();
  auto sprite = createSpriteForId(actorIdForProjectile(type, directionVector));
  const auto boundingBox = inferBoundingBox(sprite.mFrames[0]);
  entity.assign<Sprite>(sprite);
  entity.assign<BoundingBox>(boundingBox);

  configureProjectile(entity, type, pos, directionVector, boundingBox);

  return entity;
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
      boundingBox = inferBoundingBox(sprite.mFrames[0]);
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
