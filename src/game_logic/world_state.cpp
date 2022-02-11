/* Copyright (C) 2020, Nikolai Wuttke. All rights reserved.
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

#include "world_state.hpp"

#include "assets/resource_loader.hpp"
#include "engine/base_components.hpp"
#include "engine/life_time_components.hpp"
#include "engine/physical_components.hpp"
#include "engine/sprite_factory.hpp"
#include "engine/visual_components.hpp"
#include "frontend/game_service_provider.hpp"
#include "game_logic/actor_tag.hpp"
#include "game_logic/behavior_controller.hpp"
#include "game_logic/collectable_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/dynamic_geometry_components.hpp"
#include "game_logic/effect_components.hpp"
#include "game_logic/interactive/item_container.hpp"
#include "renderer/renderer.hpp"


namespace rigel::game_logic
{

namespace
{

char EPISODE_PREFIXES[] = {'L', 'M', 'N', 'O'};


std::string levelFileName(const int episode, const int level)
{
  assert(episode >= 0 && episode < 4);
  assert(level >= 0 && level < 8);

  std::string fileName;
  fileName += EPISODE_PREFIXES[episode];
  fileName += std::to_string(level + 1);
  fileName += ".MNI";
  return fileName;
}


template <typename T>
void copyComponentIfPresent(entityx::Entity from, entityx::Entity to)
{
  if (from.has_component<T>())
  {
    to.assign<T>(*from.component<const T>());
  }
}


void copyAllComponents(entityx::Entity from, entityx::Entity to)
{
  using namespace engine::components;
  using namespace game_logic::components;

  copyComponentIfPresent<ActivationSettings>(from, to);
  copyComponentIfPresent<Active>(from, to);
  copyComponentIfPresent<ActorTag>(from, to);
  copyComponentIfPresent<AnimationLoop>(from, to);
  copyComponentIfPresent<AnimationSequence>(from, to);
  copyComponentIfPresent<AppearsOnRadar>(from, to);
  copyComponentIfPresent<AutoDestroy>(from, to);
  copyComponentIfPresent<BehaviorController>(from, to);
  copyComponentIfPresent<BoundingBox>(from, to);
  copyComponentIfPresent<CollectableItem>(from, to);
  copyComponentIfPresent<CollectableItemForCheat>(from, to);
  copyComponentIfPresent<CollidedWithWorld>(from, to);
  copyComponentIfPresent<CustomDamageApplication>(from, to);
  copyComponentIfPresent<DamageInflicting>(from, to);
  copyComponentIfPresent<DestructionEffects>(from, to);
  copyComponentIfPresent<DrawTopMost>(from, to);
  copyComponentIfPresent<ExtendedFrameList>(from, to);
  copyComponentIfPresent<Interactable>(from, to);
  copyComponentIfPresent<InterpolateMotion>(from, to);
  copyComponentIfPresent<ItemBounceEffect>(from, to);
  copyComponentIfPresent<ItemContainer>(from, to);
  copyComponentIfPresent<MapGeometryLink>(from, to);
  copyComponentIfPresent<MovementSequence>(from, to);
  copyComponentIfPresent<MovingBody>(from, to);
  copyComponentIfPresent<Orientation>(from, to);
  copyComponentIfPresent<OverrideDrawOrder>(from, to);
  copyComponentIfPresent<PlayerDamaging>(from, to);
  copyComponentIfPresent<PlayerProjectile>(from, to);
  copyComponentIfPresent<RadarDish>(from, to);
  copyComponentIfPresent<Shootable>(from, to);
  copyComponentIfPresent<SolidBody>(from, to);
  copyComponentIfPresent<Sprite>(from, to);
  copyComponentIfPresent<SpriteCascadeSpawner>(from, to);
  copyComponentIfPresent<TileDebris>(from, to);
  copyComponentIfPresent<WorldPosition>(from, to);

  assert(from.component_mask() == to.component_mask());
}

} // namespace


BonusRelatedItemCounts countBonusRelatedItems(entityx::EntityManager& es)
{
  using game_logic::components::ActorTag;
  using AT = ActorTag::Type;

  BonusRelatedItemCounts counts;

  // clang-format off
  es.each<ActorTag>([&counts](entityx::Entity, const ActorTag& tag) {
      switch (tag.mType) {
        case AT::ShootableCamera: ++counts.mCameraCount; break;
        case AT::FireBomb: ++counts.mFireBombCount; break;
        case AT::CollectableWeapon: ++counts.mWeaponCount; break;
        case AT::Merchandise: ++counts.mMerchandiseCount; break;
        case AT::ShootableBonusGlobe: ++counts.mBonusGlobeCount; break;
        case AT::MountedLaserTurret: ++counts.mLaserTurretCount; break;

        default:
          break;
      }
    });
  // clang-format on

  return counts;
}


WorldState::WorldState(
  IGameServiceProvider* pServiceProvider,
  renderer::Renderer* pRenderer,
  const assets::ResourceLoader* pResources,
  data::PlayerModel* pPlayerModel,
  const data::GameOptions* pOptions,
  engine::SpriteFactory* pSpriteFactory,
  const data::GameSessionId sessionId)
  : WorldState(
      pServiceProvider,
      pRenderer,
      pResources,
      pPlayerModel,
      pOptions,
      pSpriteFactory,
      sessionId,
      assets::loadLevel(
        levelFileName(sessionId.mEpisode, sessionId.mLevel),
        *pResources,
        sessionId.mDifficulty))
{
}


WorldState::WorldState(
  IGameServiceProvider* pServiceProvider,
  renderer::Renderer* pRenderer,
  const assets::ResourceLoader* pResources,
  data::PlayerModel* pPlayerModel,
  const data::GameOptions* pOptions,
  engine::SpriteFactory* pSpriteFactory,
  const data::GameSessionId sessionId,
  data::map::LevelData&& loadedLevel)
  : WorldState(
      pServiceProvider,
      pRenderer,
      pResources,
      pPlayerModel,
      pOptions,
      pSpriteFactory,
      sessionId,
      determineDynamicMapSections(loadedLevel.mMap, loadedLevel.mActors),
      std::move(loadedLevel))
{
}


WorldState::WorldState(
  IGameServiceProvider* pServiceProvider,
  renderer::Renderer* pRenderer,
  const assets::ResourceLoader* pResources,
  data::PlayerModel* pPlayerModel,
  const data::GameOptions* pOptions,
  engine::SpriteFactory* pSpriteFactory,
  const data::GameSessionId sessionId,
  DynamicMapSectionData&& dynamicMapSections,
  data::map::LevelData&& loadedLevel)
  : mMap(std::move(loadedLevel.mMap))
  , mEntities(mEventManager)
  , mEntityFactory(
      pSpriteFactory,
      &mEntities,
      pServiceProvider,
      &mRandomGenerator,
      pOptions,
      sessionId.mDifficulty)
  , mRadarDishCounter(mEntities, mEventManager)
  , mCollisionChecker(&mMap, mEntities, mEventManager)
  , mpOptions(pOptions)
  , mPlayer(
      [&]() {
        using engine::components::Orientation;
        auto playerEntity = mEntityFactory.spawnActor(
          data::ActorID::Duke_LEFT, loadedLevel.mPlayerSpawnPosition);
        assignPlayerComponents(
          playerEntity,
          loadedLevel.mPlayerFacingLeft ? Orientation::Left
                                        : Orientation::Right);
        return playerEntity;
      }(),
      sessionId.mDifficulty,
      pPlayerModel,
      pServiceProvider,
      pOptions,
      &mCollisionChecker,
      &mMap,
      &mEntityFactory,
      &mEventManager,
      &mRandomGenerator)
  , mCamera(&mPlayer, mMap, mEventManager)
  , mParticles(&mRandomGenerator, pRenderer)
  , mSpriteRenderingSystem(pRenderer, &pSpriteFactory->textureAtlas())
  , mMapRenderer(
      pRenderer,
      std::move(dynamicMapSections.mMapStaticParts),
      &mMap.attributeDict(),
      engine::MapRenderer::MapRenderData{
        std::move(loadedLevel.mTileSetImage),
        std::move(loadedLevel.mBackdropImage),
        std::move(loadedLevel.mSecondaryBackdropImage),
        loadedLevel.mBackdropScrollMode})
  , mPhysicsSystem(&mCollisionChecker, &mMap, &mEventManager)
  , mDebuggingSystem(pRenderer, &mMap)
  , mPlayerInteractionSystem(
      sessionId,
      &mPlayer,
      pPlayerModel,
      pServiceProvider,
      &mEntityFactory,
      &mEventManager,
      *pResources)
  , mPlayerDamageSystem(&mPlayer)
  , mPlayerProjectileSystem(
      &mEntityFactory,
      pServiceProvider,
      &mCollisionChecker,
      &mMap)
  , mDamageInflictionSystem(pPlayerModel, pServiceProvider, &mEventManager)
  , mDynamicGeometrySystem(
      pServiceProvider,
      &mEntities,
      &mMap,
      &mRandomGenerator,
      &mEventManager,
      &mMapRenderer,
      std::move(dynamicMapSections.mSimpleSections))
  , mEffectsSystem(
      pServiceProvider,
      &mRandomGenerator,
      &mEntities,
      &mEntityFactory,
      &mParticles,
      mEventManager)
  , mItemContainerSystem(&mEntities, &mCollisionChecker, mEventManager)
  , mBehaviorControllerSystem(
      GlobalDependencies{
        &mCollisionChecker,
        &mParticles,
        &mRandomGenerator,
        &mEntityFactory,
        pServiceProvider,
        &mEntities,
        &mEventManager},
      &mPlayer,
      &mCamera.position(),
      &mMap)
  , mLevelMusicFile(loadedLevel.mMusicFile)
  , mBackdropSwitchCondition(loadedLevel.mBackdropSwitchCondition)
{
  mEntityFactory.createEntitiesForLevel(loadedLevel.mActors);
  mDynamicGeometrySystem.initializeDynamicGeometryEntities(
    dynamicMapSections.mFallingSections);

  const auto counts = countBonusRelatedItems(mEntities);
  mBonusInfo.mInitialCameraCount = counts.mCameraCount;
  mBonusInfo.mInitialMerchandiseCount = counts.mMerchandiseCount;
  mBonusInfo.mInitialWeaponCount = counts.mWeaponCount;
  mBonusInfo.mInitialLaserTurretCount = counts.mLaserTurretCount;
  mBonusInfo.mInitialBonusGlobeCount = counts.mBonusGlobeCount;

  if (loadedLevel.mEarthquake)
  {
    mEarthQuakeEffect =
      EarthQuakeEffect{pServiceProvider, &mRandomGenerator, &mEventManager};
  }
}


void WorldState::synchronizeTo(
  const WorldState& other,
  IGameServiceProvider* pServiceProvider,
  data::PlayerModel* pPlayerModel,
  const data::GameSessionId sessionId)
{
  if (mBackdropSwitched != other.mBackdropSwitched)
  {
    mMapRenderer.switchBackdrops();
  }

  mBonusInfo = other.mBonusInfo;
  mLevelMusicFile = other.mLevelMusicFile;
  mActivatedCheckpoint = other.mActivatedCheckpoint;
  mScreenFlashColor = other.mScreenFlashColor;
  mBackdropFlashColor = other.mBackdropFlashColor;
  mTeleportTargetPosition = other.mTeleportTargetPosition;
  mCloakPickupPosition = other.mCloakPickupPosition;
  mBossStartingHealth = other.mBossStartingHealth;
  mReactorDestructionFramesElapsed = other.mReactorDestructionFramesElapsed;
  mScreenShakeOffsetX = other.mScreenShakeOffsetX;
  mBossDeathAnimationStartPending = other.mBossDeathAnimationStartPending;
  mBackdropSwitched = other.mBackdropSwitched;
  mLevelFinished = other.mLevelFinished;
  mPlayerDied = other.mPlayerDied;
  mIsOddFrame = other.mIsOddFrame;

  mMap = other.mMap;
  mRandomGenerator = other.mRandomGenerator;
  mCamera.synchronizeTo(other.mCamera);
  mParticles.synchronizeTo(other.mParticles);
  mMapRenderer.synchronizeTo(other.mMapRenderer);

  if (other.mEarthQuakeEffect)
  {
    mEarthQuakeEffect =
      EarthQuakeEffect{pServiceProvider, &mRandomGenerator, &mEventManager};
    mEarthQuakeEffect->synchronizeTo(*other.mEarthQuakeEffect);
  }
  else
  {
    mEarthQuakeEffect.reset();
  }

  mEntities.reset();

  entityx::Entity playerEntity;

  // clang-format off
  for (
    const auto entity :
      const_cast<entityx::EntityManager&>(other.mEntities)
        .entities_for_debugging())
  // clang-format on
  {
    auto clone = mEntities.create();

    copyAllComponents(entity, clone);

    if (entity == other.mPlayer.entity())
    {
      playerEntity = clone;
    }

    if (entity == other.mActiveBossEntity)
    {
      mActiveBossEntity = clone;
    }
  }

  {
    mPlayer = Player{
      playerEntity,
      sessionId.mDifficulty,
      pPlayerModel,
      pServiceProvider,
      mpOptions,
      &mCollisionChecker,
      &mMap,
      &mEntityFactory,
      &mEventManager,
      &mRandomGenerator};
    mPlayer.synchronizeTo(other.mPlayer, mEntities);
  }
}

} // namespace rigel::game_logic
