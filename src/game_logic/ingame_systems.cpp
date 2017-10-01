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
  entityx::EventManager& eventManager,
  entityx::SystemManager& systems
)
  : mpScrollOffset(pScrollOffset)
  , mCollisionChecker(pMap, entities, eventManager)
  , mPhysicsSystem(&mCollisionChecker)
  , mMapScrollSystem(mpScrollOffset, playerEntity, *pMap)
  , mPlayerMovementSystem(playerEntity, *pMap)
  , mPlayerAttackSystem(
      playerEntity,
      pPlayerModel,
      pServiceProvider,
      pEntityFactory)
  , mElevatorSystem(playerEntity, pServiceProvider)
  , mNapalmBombSystem(pServiceProvider, pEntityFactory, &mCollisionChecker)
  , mBlueGuardSystem(
      playerEntity,
      &mCollisionChecker,
      pEntityFactory,
      pServiceProvider,
      pRandomGenerator)
  , mHoverBotSystem(
      playerEntity,
      &mCollisionChecker,
      pEntityFactory)
  , mSimpleWalkerSystem(
      playerEntity,
      &mCollisionChecker)
  , mSlimeBlobSystem(
      playerEntity,
      &mCollisionChecker,
      pEntityFactory,
      pRandomGenerator)
  , mpPlayerModel(pPlayerModel)
  , mSystems(systems)
{
  systems.add<game_logic::player::AnimationSystem>(
    playerEntity,
    pServiceProvider,
    pEntityFactory);
  systems.add<game_logic::player::DamageSystem>(
    playerEntity,
    pPlayerModel,
    pServiceProvider,
    difficulty);
  systems.add<RenderingSystem>(
    mpScrollOffset,
    pRenderer,
    pMap,
    std::move(mapRenderData));
  systems.add<PlayerInteractionSystem>(
    playerEntity,
    pPlayerModel,
    pServiceProvider,
    [this](const entityx::Entity& teleporter) {
      mActiveTeleporter = teleporter;
    });
  systems.add<DamageInflictionSystem>(
    pPlayerModel,
    pMap,
    pServiceProvider);
  systems.add<ai::MessengerDroneSystem>(playerEntity);
  systems.add<ai::PrisonerSystem>(playerEntity, pRandomGenerator);
  systems.add<ai::LaserTurretSystem>(
    playerEntity,
    pPlayerModel,
    pEntityFactory,
    pServiceProvider);
  systems.add<ai::RocketTurretSystem>(
    playerEntity,
    pEntityFactory,
    pServiceProvider);
  systems.add<ai::SecurityCameraSystem>(playerEntity);
  systems.add<ai::SlidingDoorSystem>(
    playerEntity,
    pServiceProvider);
  systems.add<ai::SlimePipeSystem>(
    pEntityFactory,
    pServiceProvider);
  systems.add<engine::LifeTimeSystem>();
  systems.add<DebuggingSystem>(
    pRenderer,
    mpScrollOffset,
    pMap);
  systems.configure();

  systems.system<DamageInflictionSystem>()->entityHitSignal().connect(
    [this, &entities](entityx::Entity entity) {
      mBlueGuardSystem.onEntityHit(entity);
      item_containers::onEntityHit(entity, entities);
      mNapalmBombSystem.onEntityHit(entity);
      mSlimeBlobSystem.onEntityHit(entity);
      mSystems.system<ai::LaserTurretSystem>()->onEntityHit(entity);
      mSystems.system<ai::PrisonerSystem>()->onEntityHit(entity);
    });
}


void IngameSystems::update(
  const PlayerInputState& inputState,
  entityx::EntityManager& entities
) {
  // ----------------------------------------------------------------------
  // Animation update
  // ----------------------------------------------------------------------
  mSystems.system<RenderingSystem>()->updateAnimatedMapTiles();
  engine::updateAnimatedSprites(entities);

  // ----------------------------------------------------------------------
  // Mark active entities
  // ----------------------------------------------------------------------
  engine::markActiveEntities(entities, *mpScrollOffset);

  // TODO: Use mSystems struct for everything, stop using entityx::SystemManager
  // entirely

  // ----------------------------------------------------------------------
  // Player logic update
  // ----------------------------------------------------------------------
  // TODO: Move all player related mSystems into the player namespace
  mpPlayerModel->updateTemporaryItemExpiry();
  mElevatorSystem.update(entities, inputState);
  mPlayerMovementSystem.update(inputState);
  mSystems.update<PlayerInteractionSystem>(0.0);

  // ----------------------------------------------------------------------
  // A.I. logic update
  // ----------------------------------------------------------------------
  mBlueGuardSystem.update(entities);
  mHoverBotSystem.update(entities);
  mSystems.update<ai::LaserTurretSystem>(0.0);
  mSystems.update<ai::MessengerDroneSystem>(0.0);
  mNapalmBombSystem.update(entities);
  mSystems.update<ai::PrisonerSystem>(0.0);
  mSystems.update<ai::RocketTurretSystem>(0.0);
  mSystems.update<ai::SecurityCameraSystem>(0.0);
  mSimpleWalkerSystem.update(entities);
  mSystems.update<ai::SlidingDoorSystem>(0.0);
  mSlimeBlobSystem.update(entities);
  mSystems.update<ai::SlimePipeSystem>(0.0);

  // ----------------------------------------------------------------------
  // Physics and other updates
  // ----------------------------------------------------------------------
  mPhysicsSystem.update(entities);

  // Player attacks have to be processed after physics, because:
  //  1. Player projectiles should spawn at the player's location
  //  2. Player projectiles mustn't move on the frame they were spawned on
  mPlayerAttackSystem.update();

  mSystems.update<player::DamageSystem>(0.0);
  mSystems.update<DamageInflictionSystem>(0.0);
  mSystems.update<player::AnimationSystem>(0.0);

  mMapScrollSystem.updateManualScrolling(0.0);

  mSystems.update<engine::LifeTimeSystem>(0.0);
}


void IngameSystems::render() {
  mMapScrollSystem.updateScrollOffset();
  mSystems.update<RenderingSystem>(0.0);
  mSystems.update<DebuggingSystem>(0.0);
}


void IngameSystems::buttonStateChanged(const PlayerInputState& inputState)
{
  mPlayerAttackSystem.buttonStateChanged(inputState);
}


DebuggingSystem& IngameSystems::debuggingSystem() {
  return *mSystems.system<DebuggingSystem>();
}


void IngameSystems::switchBackdrops() {
  mSystems.system<RenderingSystem>()->switchBackdrops();
}


entityx::Entity IngameSystems::getAndResetActiveTeleporter() {
  auto activeTeleporter = entityx::Entity{};
  std::swap(activeTeleporter, mActiveTeleporter);
  return activeTeleporter;
}

}}
