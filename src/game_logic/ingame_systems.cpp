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

#include "ingame_systems.hpp"

#include "data/player_model.hpp"
#include "engine/random_number_generator.hpp"
#include "game_logic/enemy_radar.hpp"
#include "game_logic/entity_factory.hpp"
#include "game_logic/interaction/force_field.hpp"

namespace rigel { namespace game_logic {

using namespace engine;


IngameSystems::IngameSystems(
  const data::GameSessionId& sessionId,
  base::Vector* pScrollOffset,
  entityx::Entity playerEntity,
  data::PlayerModel* pPlayerModel,
  data::map::Map* pMap,
  engine::MapRenderer::MapRenderData&& mapRenderData,
  IGameServiceProvider* pServiceProvider,
  EntityFactory* pEntityFactory,
  engine::RandomNumberGenerator* pRandomGenerator,
  const RadarDishCounter* pRadarDishCounter,
  engine::Renderer* pRenderer,
  entityx::EntityManager& entities,
  entityx::EventManager& eventManager,
  const loader::ResourceLoader& resources
)
  : mpScrollOffset(pScrollOffset)
  , mCollisionChecker(pMap, entities, eventManager)
  , mPlayer(
      playerEntity,
      sessionId.mDifficulty,
      pPlayerModel,
      pServiceProvider,
      &mCollisionChecker,
      pMap,
      pEntityFactory,
      &eventManager)
  , mParticles(pRandomGenerator, pRenderer)
  , mRenderingSystem(
      mpScrollOffset,
      pRenderer,
      pMap,
      std::move(mapRenderData))
  , mPhysicsSystem(&mCollisionChecker, &eventManager)
  , mDebuggingSystem(pRenderer, mpScrollOffset, pMap)
  , mMapScrollSystem(mpScrollOffset, &mPlayer, *pMap)
  , mPlayerInteractionSystem(
      sessionId,
      &mPlayer,
      pPlayerModel,
      pServiceProvider,
      pEntityFactory,
      &eventManager,
      resources)
  , mPlayerDamageSystem(&mPlayer, &eventManager)
  , mPlayerProjectileSystem(pEntityFactory, pServiceProvider, *pMap)
  , mElevatorSystem(
      playerEntity,
      pServiceProvider,
      &mCollisionChecker,
      &eventManager)
  , mRespawnCheckpointSystem(playerEntity, &eventManager)
  , mRadarComputerSystem(pRadarDishCounter)
  , mDamageInflictionSystem(pPlayerModel, pServiceProvider, &eventManager)
  , mDynamicGeometrySystem(
      pServiceProvider,
      &entities,
      pMap,
      pRandomGenerator,
      &eventManager)
  , mEffectsSystem(
      pServiceProvider,
      pRandomGenerator,
      &entities,
      pEntityFactory,
      &mParticles,
      eventManager)
  , mItemContainerSystem(&entities, eventManager)
  , mBlueGuardSystem(
      &mPlayer,
      &mCollisionChecker,
      pEntityFactory,
      pServiceProvider,
      pRandomGenerator,
      eventManager)
  , mHoverBotSystem(
      playerEntity,
      &mCollisionChecker,
      pEntityFactory)
  , mLaserTurretSystem(
      playerEntity,
      pPlayerModel,
      pEntityFactory,
      pRandomGenerator,
      pServiceProvider,
      eventManager)
  , mMessengerDroneSystem(playerEntity)
  , mPrisonerSystem(
      playerEntity,
      pEntityFactory,
      pServiceProvider,
      &mParticles,
      pRandomGenerator,
      eventManager)
  , mRedBirdSystem(playerEntity, eventManager)
  , mRocketTurretSystem(playerEntity, pEntityFactory, pServiceProvider)
  , mSimpleWalkerSystem(
      playerEntity,
      &mCollisionChecker)
  , mSlidingDoorSystem(playerEntity, pServiceProvider)
  , mSlimeBlobSystem(
      &mPlayer,
      &mCollisionChecker,
      pEntityFactory,
      pRandomGenerator,
      eventManager)
  , mSpiderSystem(
      &mPlayer,
      &mCollisionChecker,
      pRandomGenerator,
      pEntityFactory,
      eventManager)
  , mSpikeBallSystem(&mCollisionChecker, pServiceProvider, eventManager)
  , mBehaviorControllerSystem(
      GlobalDependencies{
        &mCollisionChecker,
        &mParticles,
        pRandomGenerator,
        pEntityFactory,
        pServiceProvider,
        &entities,
        &eventManager},
      &mPlayer,
      pScrollOffset,
      pMap)
  , mpRandomGenerator(pRandomGenerator)
  , mpServiceProvider(pServiceProvider)
{
}


void IngameSystems::update(
  const PlayerInput& input,
  entityx::EntityManager& es
) {
  // ----------------------------------------------------------------------
  // Animation update
  // ----------------------------------------------------------------------
  mRenderingSystem.updateAnimatedMapTiles();
  engine::updateAnimatedSprites(es);
  interaction::animateForceFields(es, *mpRandomGenerator, *mpServiceProvider);

  // ----------------------------------------------------------------------
  // Player update, camera, mark active entities
  // ----------------------------------------------------------------------
  mPlayerInteractionSystem.updatePlayerInteraction(input, es);

  mPlayer.update(input);
  mMapScrollSystem.update(input);
  engine::markActiveEntities(es, *mpScrollOffset);

  // ----------------------------------------------------------------------
  // Player related logic update
  // ----------------------------------------------------------------------
  mElevatorSystem.update(es);
  mRespawnCheckpointSystem.update(es);
  mRadarComputerSystem.update(es);

  // ----------------------------------------------------------------------
  // A.I. logic update
  // ----------------------------------------------------------------------
  mBlueGuardSystem.update(es);
  mHoverBotSystem.update(es);
  mLaserTurretSystem.update(es);
  mMessengerDroneSystem.update(es);
  mPrisonerSystem.update(es);
  mRedBirdSystem.update(es);
  mRocketTurretSystem.update(es);
  mSimpleWalkerSystem.update(es);
  mSlidingDoorSystem.update(es);
  mSlimeBlobSystem.update(es);
  mSpiderSystem.update(es);
  mSpikeBallSystem.update(es);
  mBehaviorControllerSystem.update(es, input);

  // ----------------------------------------------------------------------
  // Physics and other updates
  // ----------------------------------------------------------------------
  mPhysicsSystem.updatePhase1(es);

  // Collect items after physics, so that any collectible
  // items are in their final positions for this frame.
  mPlayerInteractionSystem.updateItemCollection(es);

  mPlayerDamageSystem.update(es);
  mDamageInflictionSystem.update(es);
  mItemContainerSystem.update(es);

  mPlayerProjectileSystem.update(es);

  mEffectsSystem.update(es);
  mLifeTimeSystem.update(es);

  // Now process any MovingBody objects that have been spawned after phase 1
  mPhysicsSystem.updatePhase2(es);

  mParticles.update();
}


void IngameSystems::render(
  entityx::EntityManager& es,
  const std::optional<base::Color>& backdropFlashColor
) {
  mRenderingSystem.update(es, backdropFlashColor);
  mParticles.render(*mpScrollOffset);
  mDebuggingSystem.update(es);
}


DebuggingSystem& IngameSystems::debuggingSystem() {
  return mDebuggingSystem;
}


void IngameSystems::switchBackdrops() {
  mRenderingSystem.switchBackdrops();
}


void IngameSystems::restartFromBeginning(entityx::Entity newPlayerEntity) {
  mPlayer.resetAfterDeath(newPlayerEntity);
}


void IngameSystems::restartFromCheckpoint(
  const base::Vector& checkpointPosition
) {
  mPlayer.position() = checkpointPosition;
  mPlayer.resetAfterRespawn();
}


void IngameSystems::centerViewOnPlayer() {
  mMapScrollSystem.centerViewOnPlayer();
}

}}
