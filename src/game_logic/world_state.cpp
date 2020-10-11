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

#include "common/game_service_provider.hpp"
#include "engine/sprite_factory.hpp"
#include "game_logic/actor_tag.hpp"
#include "loader/resource_loader.hpp"
#include "renderer/renderer.hpp"


namespace rigel::game_logic {

namespace {

char EPISODE_PREFIXES[] = {'L', 'M', 'N', 'O'};


std::string levelFileName(const int episode, const int level) {
  assert(episode >=0 && episode < 4);
  assert(level >=0 && level < 8);

  std::string fileName;
  fileName += EPISODE_PREFIXES[episode];
  fileName += std::to_string(level + 1);
  fileName += ".MNI";
  return fileName;
}

}


BonusRelatedItemCounts countBonusRelatedItems(entityx::EntityManager& es) {
  using game_logic::components::ActorTag;
  using AT = ActorTag::Type;

  BonusRelatedItemCounts counts;

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

  return counts;
}


WorldState::WorldState(
  IGameServiceProvider* pServiceProvider,
  renderer::Renderer* pRenderer,
  const loader::ResourceLoader* pResources,
  data::PlayerModel* pPlayerModel,
  entityx::EventManager& eventManager,
  engine::SpriteFactory* pSpriteFactory,
  const data::GameSessionId sessionId
)
  : WorldState(
      pServiceProvider,
      pRenderer,
      pResources,
      pPlayerModel,
      eventManager,
      pSpriteFactory,
      sessionId,
      loader::loadLevel(
        levelFileName(sessionId.mEpisode, sessionId.mLevel),
        *pResources,
        sessionId.mDifficulty))
{
}


WorldState::WorldState(
  IGameServiceProvider* pServiceProvider,
  renderer::Renderer* pRenderer,
  const loader::ResourceLoader* pResources,
  data::PlayerModel* pPlayerModel,
  entityx::EventManager& eventManager,
  engine::SpriteFactory* pSpriteFactory,
  const data::GameSessionId sessionId,
  data::map::LevelData&& loadedLevel
)
  : mEntities(eventManager)
  , mEntityFactory(
    pSpriteFactory,
    &mEntities,
    &mRandomGenerator,
    sessionId.mDifficulty)
  , mRadarDishCounter(mEntities, eventManager)
  , mMap(std::move(loadedLevel.mMap))
  , mLevelMusicFile(loadedLevel.mMusicFile)
  , mCollisionChecker(&mMap, mEntities, eventManager)
  , mPlayer(
      [&]() {
        using engine::components::Orientation;
        auto playerEntity = mEntityFactory.createActor(
          data::ActorID::Duke_LEFT, loadedLevel.mPlayerSpawnPosition);
        assignPlayerComponents(
          playerEntity,
          loadedLevel.mPlayerFacingLeft ? Orientation::Left : Orientation::Right);
        return playerEntity;
      }(),
      sessionId.mDifficulty,
      pPlayerModel,
      pServiceProvider,
      &mCollisionChecker,
      &mMap,
      &mEntityFactory,
      &eventManager,
      &mRandomGenerator)
  , mCamera(&mPlayer, mMap, eventManager)
  , mParticles(&mRandomGenerator, pRenderer)
  , mRenderingSystem(
      &mCamera.position(),
      pRenderer,
      &mMap,
      engine::MapRenderer::MapRenderData{
        std::move(loadedLevel.mTileSetImage),
        std::move(loadedLevel.mBackdropImage),
        std::move(loadedLevel.mSecondaryBackdropImage),
        loadedLevel.mBackdropScrollMode})
  , mPhysicsSystem(&mCollisionChecker, &mMap, &eventManager)
  , mDebuggingSystem(pRenderer, &mCamera.position(), &mMap)
  , mPlayerInteractionSystem(
      sessionId,
      &mPlayer,
      pPlayerModel,
      pServiceProvider,
      &mEntityFactory,
      &eventManager,
      *pResources)
  , mPlayerDamageSystem(&mPlayer)
  , mPlayerProjectileSystem(
      &mEntityFactory,
      pServiceProvider,
      &mCollisionChecker,
      &mMap)
  , mDamageInflictionSystem(pPlayerModel, pServiceProvider, &eventManager)
  , mDynamicGeometrySystem(
      pServiceProvider,
      &mEntities,
      &mMap,
      &mRandomGenerator,
      &eventManager)
  , mEffectsSystem(
      pServiceProvider,
      &mRandomGenerator,
      &mEntities,
      &mEntityFactory,
      &mParticles,
      eventManager)
  , mItemContainerSystem(&mEntities, &mCollisionChecker, eventManager)
  , mBehaviorControllerSystem(
      GlobalDependencies{
        &mCollisionChecker,
        &mParticles,
        &mRandomGenerator,
        &mEntityFactory,
        pServiceProvider,
        &mEntities,
        &eventManager},
      &mRadarDishCounter,
      &mPlayer,
      &mCamera.position(),
      &mMap)
  , mBackdropSwitchCondition(loadedLevel.mBackdropSwitchCondition)
{
  mEntityFactory.createEntitiesForLevel(loadedLevel.mActors);

  const auto counts = countBonusRelatedItems(mEntities);
  mBonusInfo.mInitialCameraCount = counts.mCameraCount;
  mBonusInfo.mInitialMerchandiseCount = counts.mMerchandiseCount;
  mBonusInfo.mInitialWeaponCount = counts.mWeaponCount;
  mBonusInfo.mInitialLaserTurretCount = counts.mLaserTurretCount;
  mBonusInfo.mInitialBonusGlobeCount = counts.mBonusGlobeCount;

  if (loadedLevel.mEarthquake) {
    mEarthQuakeEffect = EarthQuakeEffect{
      pServiceProvider, &mRandomGenerator, &eventManager};
  }
}

}
