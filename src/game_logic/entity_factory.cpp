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


/* NOTE-WORTHY ACTOR IDs
 *
 * 1-2: Explosions
 * 3: Explosion when Duke's normal shot hits wall
 * 4: Technical blinking thing, maybe UI-element or part of some enemy???
 * 7-10: Duke's rocket shot
 * 11: Debris/puff of smoke
 * 12-13: Sentry robot drone debris
 * 15-18: Barrel debris
 * 21: Duke's flame thrower shot (upwards)
 * 24-25: Duke's laser shot
 * 26-27: Duke's default shot
 * 71: Debris flying upwards from dying Duke
 * 72/73: Halves of a broken bonus globe
 * 74: White circle ???
 * 75: Collapsing slime bundle, after shooting nuclear waste barrel
 * 76: Small red bomb being dropped
 * 84: Smoke cloud, dissolving (maybe for turkey?)
 * 85/86: debris/particles from destroying shootable reactor
 * 91: Player ship shots?
 * 92: exhaust flames (for player ship maybe?)
 * 94: flying slime parts (after shooting moving green slime enemy, or suction plant)
 * 95: Rocket toppling over
 * 96: debris/parts of above rocket
 * 100: eye-ball (thrown by enemy)
 * 118: Green slime drop (for pipe dripping slime)
 * 123-127: Floating score numbers
 * 136: Green laser shot
 * 137-143: ??? maybe UI elements?
 * 144: Standing rocket, flies up if shot
 * 147/148: Green laser shot muzzle flash
 * 149: Rocket exhaust flame
 * 152/153: Debris/parts of destroyed ceiling grabber claw
 * 165-167: Explosion/particle effect animations
 * 169/170: Coke can debris/parts
 * 175: The end - until duke 3d
 * 191: Technical blinking thing, maybe UI-element or part of some enemy???
 * 192-199: Stone cat debris pieces
 * 204-206: Flame thrower shots
 * 232: Single frame of spider enemy.. huh?
 * 241-242: small puff of smoke, or debris parts
 * 243: More spider
 * 255: Hand of caged monster, flying after shooting it off
 * 256: Small rocket flying upwards
 * 259: Small rocket flying downwards
 * 280: WTF ???
 * 290-294: HUD elements for collected letters
 * 300: Green slime ball, thrown by Rigelatin enemies
 */

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


// Assign gravity affected physical component, using sprite size to determine
// bounding box
void addDefaultPhysical(ex::Entity entity, const SpriteFrame& spriteFrame) {
  entity.assign<Physical>(
      Physical{inferBoundingBox(spriteFrame), {0.0f, 0.0f}, true});

}


/** Returns list of actor IDs making up a compound actor
 */
auto actorIDListForActor(const ActorID ID) {
  std::vector<ActorID> actorParts;

  switch (ID) {
    case 5: // player facing left
    case 6: // player facing right
      actorParts.push_back(5);
      actorParts.push_back(6);
      break;

    case 45:
    case 46:
    case 47:
    case 48:
      actorParts.push_back(ID);
      actorParts.push_back(44);
      break;

    case 50:
      actorParts.push_back(51);
      break;

    case 58:
      actorParts.push_back(58);
      actorParts.push_back(59);
      break;

    case 130:
      actorParts.push_back(260);
      actorParts.push_back(130);
      break;

    case 171:
    case 217:
      actorParts.push_back(159);
      break;

    case 201:
      actorParts.push_back(202);
      break;

    case 213:
    case 214:
    case 215:
    case 216:
    case 220:
      actorParts.push_back(107);
      actorParts.push_back(108);
      actorParts.push_back(109);
      actorParts.push_back(110);
      actorParts.push_back(111);
      actorParts.push_back(112);
      actorParts.push_back(113);
      actorParts.push_back(ID);
      break;

    default:
      actorParts.push_back(ID);
      break;
  }
  return actorParts;
}


class SpriteEntityCreator {
public:
  SpriteEntityCreator(
    SDL_Renderer* pRenderer,
    const ActorImagePackage& spritePackage)
    : mpRenderer(pRenderer)
    , mpSpritePackage(&spritePackage)
  {
  }

  Sprite createSpriteForId(const ActorID actorID) {
    const auto actorParts = actorIDListForActor(actorID);
    auto sprite = makeSpriteFromActorIDs(actorParts);

    if (actorID == 5 || actorID == 6) {
      for (int i=0; i<39; ++i) {
        sprite.mFrames[i].mDrawOffset.x -= 1;
      }
    }

    switch (actorID) {
      case 0:
        sprite.mFramesToRender = {0, 6};
        break;

      case 62:
        sprite.mFramesToRender = {1, 0};
        break;

      case 68:
        sprite.mFramesToRender = {0, 2, 8};
        break;

      case 93:
        sprite.mFramesToRender = {1, 3};
        break;

      case 115:
        sprite.mFramesToRender = {0, 4};
        break;

      case 128:
        sprite.mFramesToRender = {0, 5, 6, 7, 8};
        break;

      case 150:
        sprite.mFramesToRender = {1};
        break;

      case 154:
        sprite.mFramesToRender = {6};
        break;

      case 171:
        sprite.mFramesToRender = {6};
        break;

      case 119:
        sprite.mFramesToRender = {0, 1, 2};
        break;

      case 200:
        sprite.mFramesToRender = {0, 2};
        break;

      case 209:
        sprite.mFramesToRender = {5, 0};
        break;

      case 217:
        sprite.mFramesToRender = {12};
        break;

      case 231:
        sprite.mFramesToRender = {3};
        break;

      case 237:
        sprite.mFramesToRender = {0, 1, 2, 3};
        break;

      case 279:
        sprite.mFramesToRender = {0, 2};
        break;
    }

    return sprite;
  }

  auto releaseCollectedTextures() {
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


void configureEntity(
  ex::Entity entity,
  const ActorID actorID,
  Sprite&& sprite
) {
  switch (actorID) {
    // Bonus globes
    case 45:
      entity.assign<Animated>(Animated{{AnimationSequence(2, 0, 3, 0)}});
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 500;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 46:
      entity.assign<Animated>(Animated{{AnimationSequence(2, 0, 3, 0)}});
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 1000;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 47:
      entity.assign<Animated>(Animated{{AnimationSequence(2, 0, 3, 0)}});
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 5000;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 48:
      entity.assign<Animated>(Animated{{AnimationSequence(2, 0, 3, 0)}});
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 1000;
        entity.assign<CollectableItem>(item);
      }
      break;

    // Circuit card force field
    case 119:
      entity.assign<Animated>(Animated{{AnimationSequence(2, 2, 4, 2)}});
      break;

    // Keyhole (blue key)
    case 122:
      entity.assign<Animated>(Animated{{AnimationSequence(2, 4)}});
      break;

    // ----------------------------------------------------------------------
    // Empty boxes
    // ----------------------------------------------------------------------
    case 162: // Empty green box
    case 163: // Empty red box
    case 164: // Empty blue box
    case 161: // Empty white box
      addDefaultPhysical(entity, sprite.mFrames[0]);
      break;

    // ----------------------------------------------------------------------
    // White boxes
    // ----------------------------------------------------------------------
    case 37: // Circuit board
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 500;
        item.mGivenItem = InventoryItemType::CircuitBoard;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 121: // Blue key
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 500;
        item.mGivenItem = InventoryItemType::BlueKey;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 53: // Rapid fire item
      {
        CollectableItem item;
        item.mGivenScore = 500;
        item.mGivenItem = InventoryItemType::RapidFire;
        item.mGivenPlayerBuff = PlayerBuff::RapidFire;
        entity.assign<CollectableItem>(item);
      }
      entity.assign<Animated>(Animated{{AnimationSequence(2)}});
      addDefaultPhysical(entity, sprite.mFrames[0]);
      break;

    case 114: // Cloaking device
      entity.assign<Animated>(Animated{{AnimationSequence(2)}});
      {
        CollectableItem item;
        item.mGivenScore = 500;
        item.mGivenItem = InventoryItemType::CloakingDevice;
        item.mGivenPlayerBuff = PlayerBuff::Cloak;
        entity.assign<CollectableItem>(item);
      }
      addDefaultPhysical(entity, sprite.mFrames[0]);
      break;

    // ----------------------------------------------------------------------
    // Red boxes
    // ----------------------------------------------------------------------
    case 168: // Soda can
      entity.assign<Animated>(Animated{{AnimationSequence(2, 0, 5)}});
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 100; // 2000 if shot and grabbed while flying
        item.mGivenHealth = 1;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 174: // 6-pack soda
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 100; // 10000 when shot
        item.mGivenHealth = 6;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 201: // Turkey
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 100;
        item.mGivenHealth = 1; // 2 if cooked
        entity.assign<CollectableItem>(item);
      }
      break;

    // ----------------------------------------------------------------------
    // Green boxes
    // ----------------------------------------------------------------------
    case 19: // Rocket launcher
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 2000;
        item.mGivenWeapon = WeaponType::Rocket;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 20: // Flame thrower
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 2000;
        item.mGivenWeapon = WeaponType::FlameThrower;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 22: // Default weapon
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenWeapon = WeaponType::Normal;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 23: // Laser
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 2000;
        item.mGivenWeapon = WeaponType::Laser;
        entity.assign<CollectableItem>(item);
      }
      break;

    // ----------------------------------------------------------------------
    // Blue boxes
    // ----------------------------------------------------------------------
    case 28: // Health molecule
      entity.assign<Animated>(Animated{{AnimationSequence(2)}});
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 500; // 10000 when at full health
        item.mGivenHealth = 1;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 155: // Collectable letter N in blue box
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 101000;
        item.mGivenCollectableLetter = CollectableLetterType::N;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 156: // Collectable letter U in blue box
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 101000;
        item.mGivenCollectableLetter = CollectableLetterType::U;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 157: // Collectable letter K in blue box
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 101000;
        item.mGivenCollectableLetter = CollectableLetterType::K;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 158: // Collectable letter E in blue box
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 101000;
        item.mGivenCollectableLetter = CollectableLetterType::E;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 187: // Collectable letter M in blue box
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 101000;
        item.mGivenCollectableLetter = CollectableLetterType::M;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 160: // Video game cartridge in blue box
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 500;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 172: // Sunglasses in blue box
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 100;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 173: // Phone in blue box
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 2000;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 181: // Boom box in blue box
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 1000;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 182: // Game disk in blue box
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 500;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 183: // TV in blue box
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 1500;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 184: // Camera in blue box
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 2500;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 185: // Computer in blue box
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 3000;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 186: // CD in blue box
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 500;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 274: // T-Shirt in blue box
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 5000;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 275: // Video tape in blue box
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 500;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 50: // teleporter
    case 51: // teleporter
      entity.assign<Animated>(Animated{{AnimationSequence(2)}});
      entity.assign<Interactable>(Interactable{InteractableType::Teleporter});
      addDefaultPhysical(entity, sprite.mFrames[0]);
      break;

    case 239: // Special hint globe
      entity.assign<Animated>(Animated{{AnimationSequence(2)}});
      addDefaultPhysical(entity, sprite.mFrames[0]);
      {
        CollectableItem item;
        item.mGivenScore = 10000;
        item.mGivenItem = InventoryItemType::SpecialHintGlobe;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 66: // Destroyable reactor
    case 117:
    case 120: // Keyhole (circuit board)
    case 188: // rotating floor spikes
    case 208: // floating exit sign to right
    case 252: // floating exit sign to left
    case 296: // floating arrow
    case 210:
    case 212:
    case 222:
    case 223:
    case 224:
    case 225:
    case 228:
    case 229:
    case 230:
    case 235:
    case 236:
    case 257:
    case 258:
    case 262:
    case 263:
      entity.assign<Animated>(Animated{{AnimationSequence(2)}});
      break;

    // Flying message ships
    case 213:
    case 214:
    case 215:
    case 216:
    case 220:
      //slot  0   1   2   3   4       5      6
      //frame 0   1   2   3   4   5   6   7  8   9
      //actor 107 108 109 110 111     112    113
      entity.assign<Animated>(Animated{{
        AnimationSequence(2, 4, 5, 4),
        AnimationSequence(2, 6, 7, 5),
        AnimationSequence(2, 8, 9, 6)}});
      break;

    case 231: // Lava riser
      entity.assign<Animated>(Animated{{AnimationSequence(2, 3, 5)}});
      break;

    case 62:
      entity.assign<Animated>(Animated{{AnimationSequence(2, 1, 2)}});
      break;

    case 246:
    case 247:
    case 248:
    case 249:
      entity.assign<Animated>(Animated{{AnimationSequence(4)}});
      break;

    default:
      break;
  }

  entity.assign<Sprite>(std::move(sprite));

  if (actorID == 5 || actorID == 6) {
    game_logic::initializePlayerEntity(entity, actorID == 6);
  }
}

}

EntityBundle createEntitiesForLevel(
  LevelData& level,
  Difficulty chosenDifficulty,
  SDL_Renderer* pRenderer,
  const loader::ActorImagePackage& spritePackage,
  entityx::EntityManager& entityManager
) {
  entityx::Entity playerEntity;

  SpriteEntityCreator creator(pRenderer, spritePackage);
  for (const auto& actor : level.mActors) {
    // Difficulty/section markers should never appear in the actor descriptions
    // coming from the loader, as they are handled during pre-processing.
    assert(
      actor.mID != 82 && actor.mID != 83 &&
      actor.mID != 103 && actor.mID != 104);

    if (actor.mAssignedArea) {
      const auto sectionRect = *actor.mAssignedArea;
      // TODO: Create correct entity
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
          level.mMap.setTileAt(0, mapCol, mapRow, 0);
          level.mMap.setTileAt(1, mapCol, mapRow, 0);
        }
      }

      continue;
    }

    switch (actor.mID) {
      case 116:
      case 137:
      case 138:
      case 139: // level exit
      case 142:
      case 143:
      case 221: // water
      case 233: // water surface
      case 234: // water surface
      case 241: // windblown-spider generator
      case 250:
      case 251:
      case 254:
        continue;
    }

    auto entity = entityManager.create();
    entity.assign<WorldPosition>(actor.mPosition);
    configureEntity(entity, actor.mID, creator.createSpriteForId(actor.mID));

    const auto isPlayer = actor.mID == 5 || actor.mID == 6;
    if (isPlayer) {
      playerEntity = entity;
    }
  }

  return {
    playerEntity,
    creator.releaseCollectedTextures()
  };
}

}}
