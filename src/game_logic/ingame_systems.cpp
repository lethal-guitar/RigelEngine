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

#include "data/game_traits.hpp"
#include "data/player_model.hpp"
#include "engine/random_number_generator.hpp"
#include "game_logic/entity_factory.hpp"
#include "game_logic/interactive/enemy_radar.hpp"
#include "renderer/upscaling_utils.hpp"

namespace rigel::game_logic {

using namespace engine;


IngameSystems::IngameSystems(
  const data::GameSessionId& sessionId,
  data::PlayerModel* pPlayerModel,
  Player* pPlayer,
  Camera* pCamera,
  data::map::Map* pMap,
  engine::MapRenderer::MapRenderData&& mapRenderData,
  IGameServiceProvider* pServiceProvider,
  EntityFactory* pEntityFactory,
  engine::RandomNumberGenerator* pRandomGenerator,
  const RadarDishCounter* pRadarDishCounter,
  const engine::CollisionChecker* pCollisionChecker,
  renderer::Renderer* pRenderer,
  entityx::EntityManager& entities,
  entityx::EventManager& eventManager,
  const loader::ResourceLoader& resources
)
  : mpPlayer(pPlayer)
  , mpCamera(pCamera)
  , mParticles(pRandomGenerator, pRenderer)
  , mRenderingSystem(
      &mpCamera->position(),
      pRenderer,
      pMap,
      std::move(mapRenderData))
  , mPhysicsSystem(pCollisionChecker, pMap, &eventManager)
  , mDebuggingSystem(pRenderer, &mpCamera->position(), pMap)
  , mPlayerInteractionSystem(
      sessionId,
      pPlayer,
      pPlayerModel,
      pServiceProvider,
      pEntityFactory,
      &eventManager,
      resources)
  , mPlayerDamageSystem(pPlayer)
  , mPlayerProjectileSystem(
      pEntityFactory,
      pServiceProvider,
      pCollisionChecker,
      pMap)
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
  , mItemContainerSystem(&entities, pCollisionChecker, eventManager)
  , mBehaviorControllerSystem(
      GlobalDependencies{
        pCollisionChecker,
        &mParticles,
        pRandomGenerator,
        pEntityFactory,
        pServiceProvider,
        &entities,
        &eventManager},
      pRadarDishCounter,
      pPlayer,
      &mpCamera->position(),
      pMap)
  , mpRenderer(pRenderer)
  , mLowResLayer(
      pRenderer,
      renderer::determineWidescreenViewPort(pRenderer).mWidthPx,
      data::GameTraits::viewPortHeightPx)
{
}


void IngameSystems::update(
  const PlayerInput& input,
  entityx::EntityManager& es,
  const base::Extents& viewPortSize
) {
  // ----------------------------------------------------------------------
  // Animation update
  // ----------------------------------------------------------------------
  mRenderingSystem.updateAnimatedMapTiles();
  engine::updateAnimatedSprites(es);

  // ----------------------------------------------------------------------
  // Player update, camera, mark active entities
  // ----------------------------------------------------------------------
  mPlayerInteractionSystem.updatePlayerInteraction(input, es);

  mpPlayer->update(input);
  mpCamera->update(input, viewPortSize);
  engine::markActiveEntities(es, mpCamera->position(), viewPortSize);

  // ----------------------------------------------------------------------
  // A.I. logic update
  // ----------------------------------------------------------------------
  mBehaviorControllerSystem.update(es, input, viewPortSize);

  // ----------------------------------------------------------------------
  // Physics and other updates
  // ----------------------------------------------------------------------
  mPhysicsSystem.updatePhase1(es);
  mItemContainerSystem.updateItemBounce(es);

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
  const std::optional<base::Color>& backdropFlashColor,
  const base::Extents& viewPortSize
) {
  mRenderingSystem.update(es, backdropFlashColor, viewPortSize);

  {
    const auto binder =
      renderer::RenderTargetTexture::Binder(mLowResLayer, mpRenderer);
    const auto saved = renderer::setupDefaultState(mpRenderer);

    mpRenderer->clear({0, 0, 0, 0});
    mParticles.render(mpCamera->position());
    mDebuggingSystem.update(es, viewPortSize);
  }

  mLowResLayer.render(mpRenderer, 0, 0);
}


DebuggingSystem& IngameSystems::debuggingSystem() {
  return mDebuggingSystem;
}


void IngameSystems::switchBackdrops() {
  mRenderingSystem.switchBackdrops();
}

}
