/* Copyright (C) 2019, Nikolai Wuttke. All rights reserved.
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

#include "base/color.hpp"
#include "base/spatial_types.hpp"
#include "base/warnings.hpp"
#include "common/game_mode.hpp"
#include "common/global.hpp"
#include "data/bonus.hpp"
#include "data/player_model.hpp"
#include "data/tutorial_messages.hpp"
#include "engine/collision_checker.hpp"
#include "engine/entity_activation_system.hpp"
#include "engine/life_time_system.hpp"
#include "engine/particle_system.hpp"
#include "engine/physics_system.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/rendering_system.hpp"
#include "engine/sprite_factory.hpp"
#include "game_logic/behavior_controller_system.hpp"
#include "game_logic/camera.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/damage_infliction_system.hpp"
#include "game_logic/debugging_system.hpp"
#include "game_logic/dynamic_geometry_system.hpp"
#include "game_logic/earth_quake_effect.hpp"
#include "game_logic/effects_system.hpp"
#include "game_logic/entity_factory.hpp"
#include "game_logic/input.hpp"
#include "game_logic/interactive/enemy_radar.hpp"
#include "game_logic/interactive/item_container.hpp"
#include "game_logic/player.hpp"
#include "game_logic/player/components.hpp"
#include "game_logic/player/damage_system.hpp"
#include "game_logic/player/interaction_system.hpp"
#include "game_logic/player/projectile_system.hpp"
#include "ui/hud_renderer.hpp"
#include "ui/ingame_message_display.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <iosfwd>
#include <optional>
#include <vector>

namespace rigel { class GameRunner; }
namespace rigel::data { struct GameOptions; }
namespace rigel::data::map { struct LevelData; }


namespace rigel::game_logic {


class GameWorld : public entityx::Receiver<GameWorld> {
public:
  GameWorld(
    data::PlayerModel* pPlayerModel,
    const data::GameSessionId& sessionId,
    GameMode::Context context,
    std::optional<base::Vector> playerPositionOverride = std::nullopt,
    bool showWelcomeMessage = false);
  ~GameWorld(); // NOLINT

  bool levelFinished() const;
  std::set<data::Bonus> achievedBonuses() const;

  void receive(const rigel::events::CheckPointActivated& event);
  void receive(const rigel::events::ExitReached& event);
  void receive(const rigel::events::PlayerDied& event);
  void receive(const rigel::events::PlayerTookDamage& event);
  void receive(const rigel::events::PlayerMessage& event);
  void receive(const rigel::events::PlayerTeleported& event);
  void receive(const rigel::events::ScreenFlash& event);
  void receive(const rigel::events::ScreenShake& event);
  void receive(const rigel::events::TutorialMessage& event);
  void receive(const events::ShootableKilled& event);
  void receive(const rigel::events::BossActivated& event);
  void receive(const rigel::events::BossDestroyed& event);

  void updateGameLogic(const PlayerInput& input);
  void render();
  void processEndOfFrameActions();

  friend class rigel::GameRunner;

private:
  void loadLevel();

  void onReactorDestroyed(const base::Vector& position);
  void updateReactorDestructionEvent();

  void handleLevelExit();
  void handlePlayerDeath();
  void restartLevel();
  void restartFromCheckpoint();
  void handleTeleporter();
  void updateTemporaryItemExpiration();
  void showTutorialMessage(const data::TutorialMessageId id);

  void printDebugText(std::ostream& stream) const;

private:
  struct LevelBonusInfo {
    int mInitialCameraCount = 0;
    int mInitialMerchandiseCount = 0;
    int mInitialWeaponCount = 0;
    int mInitialLaserTurretCount = 0;
    int mInitialBonusGlobeCount = 0;

    int mNumShotBonusGlobes = 0;
    bool mPlayerTookDamage = false;
  };

  struct CheckpointData {
    data::PlayerModel::CheckpointState mState;
    base::Vector mPosition;
  };

  struct WorldState {
    WorldState(
      IGameServiceProvider* pServiceProvider,
      renderer::Renderer* pRenderer,
      const loader::ResourceLoader* pResources,
      data::PlayerModel* pPlayerModel,
      entityx::EventManager& eventManager,
      engine::SpriteFactory* pSpriteFactory,
      data::GameSessionId sessionId);
    WorldState(
      IGameServiceProvider* pServiceProvider,
      renderer::Renderer* pRenderer,
      const loader::ResourceLoader* pResources,
      data::PlayerModel* pPlayerModel,
      entityx::EventManager& eventManager,
      engine::SpriteFactory* pSpriteFactory,
      data::GameSessionId sessionId,
      data::map::LevelData&& loadedLevel);

    entityx::EntityManager mEntities;
    engine::RandomNumberGenerator mRandomGenerator;
    EntityFactory mEntityFactory;
    RadarDishCounter mRadarDishCounter;

    data::map::Map mMap;
    LevelBonusInfo mBonusInfo;
    std::string mLevelMusicFile;

    engine::CollisionChecker mCollisionChecker;
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
    game_logic::BehaviorControllerSystem mBehaviorControllerSystem;

    std::optional<CheckpointData> mActivatedCheckpoint;
    std::optional<EarthQuakeEffect> mEarthQuakeEffect;
    std::optional<base::Color> mScreenFlashColor;
    std::optional<base::Color> mBackdropFlashColor;
    std::optional<base::Vector> mTeleportTargetPosition;
    entityx::Entity mActiveBossEntity;
    std::optional<int> mReactorDestructionFramesElapsed;
    int mScreenShakeOffsetX = 0;
    data::map::BackdropSwitchCondition mBackdropSwitchCondition;
    bool mBossDeathAnimationStartPending = false;
    bool mBackdropSwitched = false;
    bool mLevelFinished = false;
    bool mPlayerDied = false;
  };

  renderer::Renderer* mpRenderer;
  IGameServiceProvider* mpServiceProvider;
  engine::TiledTexture* mpUiSpriteSheet;
  ui::MenuElementRenderer* mpTextRenderer;
  data::PlayerModel* mpPlayerModel;
  const data::GameOptions* mpOptions;
  const loader::ResourceLoader* mpResources;
  data::GameSessionId mSessionId;

  entityx::EventManager mEventManager;
  engine::SpriteFactory mSpriteFactory;
  data::PlayerModel mPlayerModelAtLevelStart;
  ui::HudRenderer mHudRenderer;
  ui::IngameMessageDisplay mMessageDisplay;
  renderer::RenderTargetTexture mLowResLayer;

  std::unique_ptr<WorldState> mpState;
};

}
