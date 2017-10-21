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

#include "data/player_data.hpp"
#include "engine/random_number_generator.hpp"
#include "game_logic/entity_factory.hpp"


namespace rigel { namespace game_logic {

using namespace engine;


IngameSystems::IngameSystems(
  const data::Difficulty difficulty,
  base::Vector* pScrollOffset,
  entityx::Entity playerEntity,
  data::PlayerModel* pPlayerModel,
  data::map::Map* pMap,
  engine::MapRenderer::MapRenderData&& mapRenderData,
  IGameServiceProvider* pServiceProvider,
  EntityFactory* pEntityFactory,
  engine::RandomNumberGenerator* pRandomGenerator,
  engine::Renderer* pRenderer,
  entityx::EntityManager& entities,
  entityx::EventManager& eventManager
)
  : mpScrollOffset(pScrollOffset)
  , mCollisionChecker(pMap, entities, eventManager)
  , mParticles(pRandomGenerator, pRenderer)
  , mRenderingSystem(
      mpScrollOffset,
      pRenderer,
      pMap,
      std::move(mapRenderData))
  , mPhysicsSystem(&mCollisionChecker, &eventManager)
  , mDebuggingSystem(pRenderer, mpScrollOffset, pMap)
  , mMapScrollSystem(mpScrollOffset, playerEntity, *pMap)
  , mPlayerMovementSystem(playerEntity, *pMap)
  , mPlayerInteractionSystem(
      playerEntity,
      pPlayerModel,
      pServiceProvider,
      [this](const entityx::Entity& teleporter) {
        mActiveTeleporter = teleporter;
      })
  , mPlayerAttackSystem(
      playerEntity,
      pPlayerModel,
      pServiceProvider,
      pEntityFactory)
  , mPlayerAnimationSystem(
      playerEntity,
      pServiceProvider,
      pEntityFactory)
  , mPlayerDamageSystem(
      playerEntity,
      pPlayerModel,
      pServiceProvider,
      difficulty)
  , mPlayerProjectileSystem(pEntityFactory, pServiceProvider, *pMap)
  , mElevatorSystem(playerEntity, pServiceProvider)
  , mDamageInflictionSystem(pPlayerModel, pServiceProvider, &eventManager)
  , mDynamicGeometrySystem(
      pServiceProvider,
      &entities,
      pMap,
      pRandomGenerator,
      eventManager)
  , mEffectsSystem(
      pServiceProvider,
      pRandomGenerator,
      &entities,
      pEntityFactory,
      &mParticles,
      eventManager)
  , mItemContainerSystem(&entities, eventManager)
  , mNapalmBombSystem(
      pServiceProvider,
      pEntityFactory,
      &mCollisionChecker,
      eventManager)
  , mBlueGuardSystem(
      playerEntity,
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
  , mSecurityCameraSystem(playerEntity)
  , mSimpleWalkerSystem(
      playerEntity,
      &mCollisionChecker)
  , mSlidingDoorSystem(playerEntity, pServiceProvider)
  , mSlimeBlobSystem(
      playerEntity,
      &mCollisionChecker,
      pEntityFactory,
      pRandomGenerator,
      eventManager)
  , mSlimePipeSystem(pEntityFactory, pServiceProvider)
  , mSpikeBallSystem(&mCollisionChecker, pServiceProvider, eventManager)
  , mpPlayerModel(pPlayerModel)
{
}


void IngameSystems::update(
  const PlayerInputState& inputState,
  entityx::EntityManager& es
) {
  // ----------------------------------------------------------------------
  // Animation update
  // ----------------------------------------------------------------------
  mRenderingSystem.updateAnimatedMapTiles();
  engine::updateAnimatedSprites(es);

  // ----------------------------------------------------------------------
  // Mark active entities
  // ----------------------------------------------------------------------
  engine::markActiveEntities(es, *mpScrollOffset);

  // ----------------------------------------------------------------------
  // Player logic update
  // ----------------------------------------------------------------------
  // TODO: Move all player related systems into the player namespace
  mpPlayerModel->updateTemporaryItemExpiry();
  mElevatorSystem.update(es, inputState);
  mPlayerMovementSystem.update(inputState);
  mPlayerInteractionSystem.update(es);

  // ----------------------------------------------------------------------
  // A.I. logic update
  // ----------------------------------------------------------------------
  mBlueGuardSystem.update(es);
  mHoverBotSystem.update(es);
  mLaserTurretSystem.update(es);
  mMessengerDroneSystem.update(es);
  mNapalmBombSystem.update(es);
  mPrisonerSystem.update(es);
  mRedBirdSystem.update(es);
  mRocketTurretSystem.update(es);
  mSecurityCameraSystem.update(es);
  mSimpleWalkerSystem.update(es);
  mSlidingDoorSystem.update(es);
  mSlimeBlobSystem.update(es);
  mSlimePipeSystem.update(es);
  mSpikeBallSystem.update(es);

  // ----------------------------------------------------------------------
  // Physics and other updates
  // ----------------------------------------------------------------------
  mPhysicsSystem.update(es);

  // Player attacks have to be processed after physics, because:
  //  1. Player projectiles should spawn at the player's location
  //  2. Player projectiles mustn't move on the frame they were spawned on
  mPlayerAttackSystem.update();

  mPlayerDamageSystem.update(es);
  mDamageInflictionSystem.update(es);
  mEffectsSystem.update(es);
  mPlayerAnimationSystem.update(es);

  mPlayerProjectileSystem.update(es);

  mMapScrollSystem.update();
  mLifeTimeSystem.update(es);

  mParticles.update();
}


void IngameSystems::render(entityx::EntityManager& es) {
  mRenderingSystem.update(es);
  mParticles.render(*mpScrollOffset);
  mDebuggingSystem.update(es);
}


void IngameSystems::buttonStateChanged(const PlayerInputState& inputState)
{
  mPlayerAttackSystem.buttonStateChanged(inputState);
}


DebuggingSystem& IngameSystems::debuggingSystem() {
  return mDebuggingSystem;
}


void IngameSystems::switchBackdrops() {
  mRenderingSystem.switchBackdrops();
}


entityx::Entity IngameSystems::getAndResetActiveTeleporter() {
  auto activeTeleporter = entityx::Entity{};
  std::swap(activeTeleporter, mActiveTeleporter);
  return activeTeleporter;
}


void IngameSystems::centerViewOnPlayer() {
  mMapScrollSystem.centerViewOnPlayer();
}

}}
