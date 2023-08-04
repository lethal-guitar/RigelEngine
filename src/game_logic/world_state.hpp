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

#pragma once

#include "base/spatial_types.hpp"
#include "base/warnings.hpp"
#include "data/bonus.hpp"
#include "data/map.hpp"
#include "data/player_model.hpp"
#include "engine/collision_checker.hpp"
#include "engine/entity_activation_system.hpp"
#include "engine/life_time_system.hpp"
#include "engine/map_renderer.hpp"
#include "engine/particle_system.hpp"
#include "engine/physics_system.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/sprite_rendering_system.hpp"
#include "game_logic/behavior_controller_system.hpp"
#include "game_logic/camera.hpp"
#include "game_logic/damage_infliction_system.hpp"
#include "game_logic/debugging_system.hpp"
#include "game_logic/dynamic_geometry_system.hpp"
#include "game_logic/earth_quake_effect.hpp"
#include "game_logic/effects_system.hpp"
#include "game_logic/entity_factory.hpp"
#include "game_logic/interactive/enemy_radar.hpp"
#include "game_logic/interactive/item_container.hpp"
#include "game_logic/player.hpp"
#include "game_logic/player/components.hpp"
#include "game_logic/player/damage_system.hpp"
#include "game_logic/player/interaction_system.hpp"
#include "game_logic/player/projectile_system.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <string>


namespace rigel
{
struct IGameServiceProvider;
}
namespace rigel::data
{
struct GameOptions;
}
namespace rigel::engine
{
class SpriteFactory;
}
namespace rigel::assets
{
class ResourceLoader;
}
namespace rigel::renderer
{
class Renderer;
}


namespace rigel::game_logic
{

struct BonusRelatedItemCounts
{
  int mCameraCount = 0;
  int mFireBombCount = 0;
  int mWeaponCount = 0;
  int mMerchandiseCount = 0;
  int mBonusGlobeCount = 0;
  int mLaserTurretCount = 0;
};

BonusRelatedItemCounts countBonusRelatedItems(entityx::EntityManager& es);


struct LevelBonusInfo
{
  int mInitialCameraCount = 0;
  int mInitialMerchandiseCount = 0;
  int mInitialWeaponCount = 0;
  int mInitialLaserTurretCount = 0;
  int mInitialBonusGlobeCount = 0;

  int mNumShotBonusGlobes = 0;
  bool mPlayerTookDamage = false;
};

struct CheckpointData
{
  data::PersistentPlayerState::CheckpointState mState;
  base::Vec2 mPosition;
};


struct WorldState
{
  WorldState(
    IGameServiceProvider* pServiceProvider,
    renderer::Renderer* pRenderer,
    const assets::ResourceLoader* pResources,
    data::PersistentPlayerState* pPersistentPlayerState,
    const data::GameOptions* pOptions,
    engine::SpriteFactory* pSpriteFactory,
    data::GameSessionId sessionId);
  WorldState(
    IGameServiceProvider* pServiceProvider,
    renderer::Renderer* pRenderer,
    const assets::ResourceLoader* pResources,
    data::PersistentPlayerState* pPersistentPlayerState,
    const data::GameOptions* pOptions,
    engine::SpriteFactory* pSpriteFactory,
    data::GameSessionId sessionId,
    data::map::LevelData&& loadedLevel);
  WorldState(
    IGameServiceProvider* pServiceProvider,
    renderer::Renderer* pRenderer,
    const assets::ResourceLoader* pResources,
    data::PersistentPlayerState* pPersistentPlayerState,
    const data::GameOptions* pOptions,
    engine::SpriteFactory* pSpriteFactory,
    data::GameSessionId sessionId,
    DynamicMapSectionData&& dynamicMapSections,
    data::map::LevelData&& loadedLevel);

  void synchronizeTo(
    const WorldState& other,
    IGameServiceProvider* pServiceProvider,
    data::PersistentPlayerState* pPersistentPlayerState,
    data::GameSessionId sessionId);

  data::map::Map mMap;

  entityx::EventManager mEventManager;
  entityx::EntityManager mEntities;
  engine::RandomNumberGenerator mRandomGenerator;
  EntityFactory mEntityFactory;
  RadarDishCounter mRadarDishCounter;
  engine::CollisionChecker mCollisionChecker;
  const data::GameOptions* mpOptions;

  Player mPlayer;
  Camera mCamera;
  base::Vec2 mPreviousCameraPosition;
  engine::ParticleSystem mParticles;
  engine::SpriteRenderingSystem mSpriteRenderingSystem;
  engine::MapRenderer mMapRenderer;
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
  game_logic::BehaviorControllerSystem mBehaviorControllerSystem;

  LevelBonusInfo mBonusInfo;
  std::string mLevelMusicFile;
  std::optional<CheckpointData> mActivatedCheckpoint;
  std::optional<EarthQuakeEffect> mEarthQuakeEffect;
  std::optional<base::Color> mScreenFlashColor;
  std::optional<base::Color> mBackdropFlashColor;
  std::optional<base::Vec2> mTeleportTargetPosition;
  std::optional<base::Vec2> mCloakPickupPosition;
  entityx::Entity mActiveBossEntity;
  int mBossStartingHealth = 0;
  std::optional<int> mReactorDestructionFramesElapsed;
  int mScreenShakeOffsetX = 0;
  data::map::BackdropSwitchCondition mBackdropSwitchCondition;
  int mWaterAnimStep = 0;
  bool mBossDeathAnimationStartPending = false;
  bool mBackdropSwitched = false;
  bool mLevelFinished = false;
  bool mPlayerDied = false;
  bool mIsOddFrame = true;
};

} // namespace rigel::game_logic
