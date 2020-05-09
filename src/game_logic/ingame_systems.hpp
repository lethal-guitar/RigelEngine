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

#pragma once

#include "engine/entity_activation_system.hpp"
#include "engine/life_time_system.hpp"
#include "engine/particle_system.hpp"
#include "engine/physics_system.hpp"
#include "engine/rendering_system.hpp"
#include "engine/rendering_system.hpp"
#include "game_logic/behavior_controller_system.hpp"
#include "game_logic/camera.hpp"
#include "game_logic/damage_infliction_system.hpp"
#include "game_logic/debugging_system.hpp"
#include "game_logic/dynamic_geometry_system.hpp"
#include "game_logic/effects_system.hpp"
#include "game_logic/enemies/simple_walker.hpp"
#include "game_logic/enemies/spider.hpp"
#include "game_logic/enemies/spike_ball.hpp"
#include "game_logic/interactive/item_container.hpp"
#include "game_logic/player.hpp"
#include "game_logic/player/damage_system.hpp"
#include "game_logic/player/interaction_system.hpp"
#include "game_logic/player/projectile_system.hpp"

#include <iosfwd>

namespace rigel {

namespace data { class PlayerModel; }
namespace engine { class RandomNumberGenerator; }
namespace game_logic {
  class EntityFactory;
  class RadarDishCounter;
}
namespace loader { class ResourceLoader; }

}


namespace rigel::game_logic {

class Player;

class IngameSystems {
public:
  IngameSystems(
    const data::GameSessionId& sessionId,
    entityx::Entity playerEntity,
    data::PlayerModel* pPlayerModel,
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
    const loader::ResourceLoader& resources);

  void update(
    const PlayerInput& inputState,
    entityx::EntityManager& es,
    const base::Extents& viewPortSize);
  void render(
    entityx::EntityManager& es,
    const std::optional<base::Color>& backdropFlashColor,
    const base::Extents& viewPortSize);
  void updateBackdropAutoScrolling(const engine::TimeDelta dt) {
    mRenderingSystem.updateBackdropAutoScrolling(dt);
  }

  DebuggingSystem& debuggingSystem();

  void switchBackdrops();

  void restartFromCheckpoint(const base::Vector& checkpointPosition);

  void centerViewOnPlayer();

  Player& player() {
    return mPlayer;
  }

  void printDebugText(std::ostream& stream) const;

private:
  Player mPlayer;
  Camera mCamera;

  engine::ParticleSystem mParticles;

  engine::RenderingSystem mRenderingSystem;
  engine::PhysicsSystem mPhysicsSystem;
  engine::LifeTimeSystem mLifeTimeSystem;

  game_logic::DebuggingSystem mDebuggingSystem;

  game_logic::PlayerInteractionSystem mPlayerInteractionSystem;
  game_logic::player::DamageSystem mPlayerDamageSystem;
  game_logic::player::ProjectileSystem mPlayerProjectileSystem;

  game_logic::DamageInflictionSystem mDamageInflictionSystem;
  game_logic::DynamicGeometrySystem mDynamicGeometrySystem;
  game_logic::EffectsSystem mEffectsSystem;
  game_logic::ItemContainerSystem mItemContainerSystem;

  game_logic::ai::SimpleWalkerSystem mSimpleWalkerSystem;
  game_logic::ai::SpiderSystem mSpiderSystem;
  game_logic::ai::SpikeBallSystem mSpikeBallSystem;

  game_logic::BehaviorControllerSystem mBehaviorControllerSystem;

  renderer::Renderer* mpRenderer;
  renderer::RenderTargetTexture mLowResLayer;
};

}
