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

#include <base/warnings.hpp>
#include <data/unit_conversions.hpp>
#include <engine/base_components.hpp>
#include <engine/physics_system.hpp>
#include <engine/rendering_system.hpp>
#include <game_logic/collectable_components.hpp>
#include <game_logic/player_control_system.hpp>
#include <map>
#include <utility>


RIGEL_DISABLE_WARNINGS
#include <boost/optional.hpp>
RIGEL_RESTORE_WARNINGS


namespace ex = entityx;


namespace rigel { namespace game_logic {

using namespace data;
using namespace loader;
using namespace std;

using data::map::LevelData;
using data::ActorID;

using engine::components::Animated;
using engine::components::AnimationSequence;
using engine::components::Physical;
using engine::components::Sprite;
using engine::components::SpriteFrame;
using engine::components::WorldPosition;
using namespace game_logic::components;


namespace {

engine::BoundingBox inferBoundingBox(const SpriteFrame& sprite) {
  const auto dimensionsInTiles = pixelExtentsToTileExtents(
    sprite.mImage.extents());

  return {{0, 0}, dimensionsInTiles};
}


// Assign gravity affected physical component
void addDefaultPhysical(
  ex::Entity entity,
  const engine::BoundingBox& boundingBox
) {
  entity.assign<Physical>(Physical{boundingBox, {0.0f, 0.0f}, true});
}


#include "entity_configuration.ipp"


class SpriteTextureFactory {
public:
  SpriteTextureFactory(
    SDL_Renderer* pRenderer,
    const ActorImagePackage& spritePackage)
    : mpRenderer(pRenderer)
    , mpSpritePackage(&spritePackage)
  {
  }

  using VisualsAndBounds =
    std::tuple<boost::optional<Sprite>, engine::BoundingBox>;

  VisualsAndBounds createVisualsAndBoundingBox(
    const LevelData::Actor& actor,
    data::map::Map& map
  ) {
    VisualsAndBounds result;

    if (actor.mAssignedArea) {
      const auto sectionRect = *actor.mAssignedArea;
      for (
        auto mapRow=sectionRect.topLeft.y;
        mapRow<=sectionRect.bottomRight().y;
        ++mapRow
      ) {
        for (
          auto mapCol=sectionRect.topLeft.x;
          mapCol<=sectionRect.bottomRight().x;
          ++mapCol
        ) {
          map.setTileAt(0, mapCol, mapRow, 0);
          map.setTileAt(1, mapCol, mapRow, 0);
        }
      }
    } else if (hasAssociatedSprite(actor.mID)) {
      auto sprite = createSpriteForId(actor.mID);
      std::get<1>(result) = inferBoundingBox(sprite.mFrames[0]);
      std::get<0>(result) = std::move(sprite);
    } else {
      // TODO: Assign bounding box for non-visual entities that have one
    }

    return result;
  }

  Sprite createSpriteForId(const ActorID actorID) {
    const auto actorParts = actorIDListForActor(actorID);
    auto sprite = makeSpriteFromActorIDs(actorParts);
    configureSprite(sprite, actorID);
    return sprite;
  }

  auto releaseTextures() {
    std::vector<sdl_utils::OwningTexture> allTextures;
    for (auto& mapping : mTextureCache) {
      allTextures.emplace_back(std::move(mapping.second));
    }
    mTextureCache.clear();
    return allTextures;
  }

private:
  using IdAndFrameNr = std::pair<ActorID, std::size_t>;

  const sdl_utils::OwningTexture& getOrCreateTexture(
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

  Sprite makeSpriteFromActorIDs(const vector<ActorID>& actorIDs) {
    Sprite sprite;
    int lastFrameCount = 0;

    for (const auto ID : actorIDs) {
      const auto& actorData = mpSpritePackage->loadActor(ID);
      for (auto i=0u; i<actorData.mFrames.size(); ++i) {
        const auto& frameData = actorData.mFrames[i];
        const auto& texture = getOrCreateTexture(make_pair(ID, i));
        auto textureRef = sdl_utils::NonOwningTexture(texture);
        sprite.mFrames.emplace_back(
          std::move(textureRef),
          frameData.mDrawOffset);
      }
      sprite.mDrawOrder = actorData.mDrawIndex;
      sprite.mFramesToRender.push_back(lastFrameCount);
      lastFrameCount += static_cast<int>(actorData.mFrames.size());
    }
    return sprite;
  }

  SDL_Renderer* mpRenderer;
  const ActorImagePackage* mpSpritePackage;

  std::map<IdAndFrameNr, sdl_utils::OwningTexture> mTextureCache;
};

}


EntityBundle createEntitiesForLevel(
  LevelData& level,
  Difficulty chosenDifficulty,
  SDL_Renderer* pRenderer,
  const loader::ActorImagePackage& spritePackage,
  entityx::EntityManager& entityManager
) {
  entityx::Entity playerEntity;

  SpriteTextureFactory spriteFactory(pRenderer, spritePackage);
  for (const auto& actor : level.mActors) {
    // Difficulty/section markers should never appear in the actor descriptions
    // coming from the loader, as they are handled during pre-processing.
    assert(
      actor.mID != 82 && actor.mID != 83 &&
      actor.mID != 103 && actor.mID != 104);

    auto entity = entityManager.create();
    entity.assign<WorldPosition>(actor.mPosition);

    boost::optional<Sprite> maybeSprite;
    engine::BoundingBox boundingBox;
    std::tie(maybeSprite, boundingBox) =
      spriteFactory.createVisualsAndBoundingBox(actor, level.mMap);

    configureEntity(entity, actor.mID, boundingBox);

    if (maybeSprite) {
      entity.assign<Sprite>(std::move(*maybeSprite));
    }

    const auto isPlayer = actor.mID == 5 || actor.mID == 6;
    if (isPlayer) {
      game_logic::initializePlayerEntity(entity, actor.mID == 6);
      playerEntity = entity;
    }
  }

  return {
    playerEntity,
    spriteFactory.releaseTextures()
  };
}

}}
