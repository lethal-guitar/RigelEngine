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

#include "ingame_mode.hpp"

#include "data/game_traits.hpp"
#include "data/map.hpp"
#include "data/sound_ids.hpp"
#include "engine/collision_checker.hpp"
#include "engine/debugging_system.hpp"
#include "engine/entity_activation_system.hpp"
#include "engine/life_time_system.hpp"
#include "engine/physics_system.hpp"
#include "engine/rendering_system.hpp"
#include "game_logic/ai/blue_guard.hpp"
#include "game_logic/ai/hover_bot.hpp"
#include "game_logic/ai/laser_turret.hpp"
#include "game_logic/ai/messenger_drone.hpp"
#include "game_logic/ai/prisoner.hpp"
#include "game_logic/ai/rocket_turret.hpp"
#include "game_logic/ai/security_camera.hpp"
#include "game_logic/ai/sliding_door.hpp"
#include "game_logic/ai/slime_blob.hpp"
#include "game_logic/ai/slime_pipe.hpp"
#include "game_logic/damage_infliction_system.hpp"
#include "game_logic/interaction/elevator.hpp"
#include "game_logic/interaction/teleporter.hpp"
#include "game_logic/map_scroll_system.hpp"
#include "game_logic/player/animation_system.hpp"
#include "game_logic/player/attack_system.hpp"
#include "game_logic/player/damage_system.hpp"
#include "game_logic/player_interaction_system.hpp"
#include "game_logic/player_movement_system.hpp"
#include "game_logic/trigger_components.hpp"
#include "loader/resource_loader.hpp"
#include "ui/utils.hpp"

#include <cassert>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>


namespace rigel {

using namespace engine;
using namespace game_logic;
using namespace std;

using data::PlayerModel;
using engine::components::BoundingBox;
using engine::components::Physical;
using engine::components::WorldPosition;


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


std::string loadingScreenFileName(const int episode) {
  assert(episode >=0 && episode < 4);

  std::string fileName("LOAD");
  fileName += std::to_string(episode + 1);
  fileName += ".MNI";
  return fileName;
}


template<typename ValueT>
std::string vec2String(const base::Point<ValueT>& vec, const int width) {
  std::stringstream stream;
  stream
    << std::setw(width) << std::fixed << std::setprecision(2) << vec.x << ", "
    << std::setw(width) << std::fixed << std::setprecision(2) << vec.y;
  return stream.str();
}

}


struct IngameMode::Systems {
  template<typename FireShotFuncT>
  Systems(
    base::Vector* pScrollOffset,
    entityx::Entity playerEntity,
    data::PlayerModel* pPlayerModel,
    data::map::Map* pMap,
    IGameServiceProvider* pServiceProvider,
    EntityFactory* pEntityFactory,
    RandomNumberGenerator* pRandomGenerator,
    const data::map::Map& map,
    FireShotFuncT fireShotFunc
  )
    : mCollisionChecker(pMap)
    , mMapScrollSystem(pScrollOffset, playerEntity, map)
    , mPlayerMovementSystem(playerEntity, map)
    , mPlayerAttackSystem(
        playerEntity,
        pPlayerModel,
        pServiceProvider,
        fireShotFunc)
    , mElevatorSystem(playerEntity, pServiceProvider)
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
    , mSlimeBlobSystem(
        playerEntity,
        &mCollisionChecker,
        pEntityFactory,
        pRandomGenerator)
  {
  }

  engine::CollisionChecker mCollisionChecker;

  game_logic::MapScrollSystem mMapScrollSystem;
  game_logic::PlayerMovementSystem mPlayerMovementSystem;
  game_logic::player::AttackSystem mPlayerAttackSystem;
  game_logic::interaction::ElevatorSystem mElevatorSystem;

  game_logic::ai::BlueGuardSystem mBlueGuardSystem;
  game_logic::ai::HoverBotSystem mHoverBotSystem;
  game_logic::ai::SlimeBlobSystem mSlimeBlobSystem;
};


IngameMode::IngameMode(
  const int episode,
  const int levelNumber,
  const data::Difficulty difficulty,
  Context context,
  boost::optional<base::Vector> playerPositionOverride
)
  : mpRenderer(context.mpRenderer)
  , mpServiceProvider(context.mpServiceProvider)
  , mEntityFactory(
      context.mpRenderer,
      &mEntities.entities,
      &context.mpResources->mActorImagePackage,
      difficulty)
  , mPlayerModelAtLevelStart(mPlayerModel)
  , mLevelFinished(false)
  , mAccumulatedTime(0.0)
  , mShowDebugText(false)
  , mHudRenderer(
      &mPlayerModel,
      levelNumber + 1,
      mpRenderer,
      *context.mpResources)
  , mIngameViewPortRenderTarget(
      context.mpRenderer,
      data::GameTraits::inGameViewPortSize.width,
      data::GameTraits::inGameViewPortSize.height)
{
  showLoadingScreen(episode, *context.mpResources);

  using namespace std::chrono;
  auto before = high_resolution_clock::now();

  loadLevel(episode, levelNumber, difficulty, *context.mpResources);

  if (playerPositionOverride) {
    *mPlayerEntity.component<WorldPosition>() = *playerPositionOverride;
  }

  auto after = high_resolution_clock::now();
  std::cout << "Level load time: " <<
    duration<double>(after - before).count() * 1000.0 << " ms\n";
}

IngameMode::~IngameMode() {
}


void IngameMode::handleEvent(const SDL_Event& event) {
  if (event.type != SDL_KEYDOWN && event.type != SDL_KEYUP) {
    return;
  }

  const auto keyPressed = std::uint8_t{event.type == SDL_KEYDOWN};
  switch (event.key.keysym.sym) {
    // TODO: Refactor: This can be clearer and less repetitive.
    case SDLK_UP:
      mCombinedInputState.mMovingUp = mCombinedInputState.mMovingUp || keyPressed;
      mInputState.mMovingUp = keyPressed;
      break;
    case SDLK_DOWN:
      mCombinedInputState.mMovingDown = mCombinedInputState.mMovingDown || keyPressed;
      mInputState.mMovingDown = keyPressed;
      break;
    case SDLK_LEFT:
      mCombinedInputState.mMovingLeft = mCombinedInputState.mMovingLeft || keyPressed;
      mInputState.mMovingLeft = keyPressed;
      break;
    case SDLK_RIGHT:
      mCombinedInputState.mMovingRight = mCombinedInputState.mMovingRight || keyPressed;
      mInputState.mMovingRight = keyPressed;
      break;
    case SDLK_LCTRL:
    case SDLK_RCTRL:
      mCombinedInputState.mJumping = mCombinedInputState.mJumping || keyPressed;
      mInputState.mJumping = keyPressed;
      break;
    case SDLK_LALT:
    case SDLK_RALT:
      mCombinedInputState.mShooting = mCombinedInputState.mShooting || keyPressed;
      mInputState.mShooting = keyPressed;

      // To make shooting feel responsive even when updating the attack system
      // only at game-logic rate, we notify the system about button presses
      // immediately. The system will queue up one requested shot for the
      // next logic update.
      //
      // Without this, fire button presses can get lost since firing is
      // only allowed if the button is released between two shots. If the
      // release happens between two logic updates, the system wouldn't see it,
      // therefore thinking you're still holding the button.
      mpSystems->mPlayerAttackSystem.buttonStateChanged(mInputState);
      break;
  }

  // Debug keys
  // ----------------------------------------------------------------------
  if (keyPressed) {
    return;
  }

  auto& debuggingSystem = *mEntities.systems.system<DebuggingSystem>();
  switch (event.key.keysym.sym) {
    case SDLK_b:
      debuggingSystem.toggleBoundingBoxDisplay();
      break;

    case SDLK_c:
      debuggingSystem.toggleWorldCollisionDataDisplay();
      break;

    case SDLK_d:
      mShowDebugText = !mShowDebugText;
      break;
  }
}


void IngameMode::updateAndRender(engine::TimeDelta dt) {
  if (mLevelFinished) {
    return;
  }

  int screenShakeOffsetX = 0;

  // **********************************************************************
  // Updating
  // **********************************************************************

  constexpr auto timeForOneFrame = engine::gameFramesToTime(1);
  mAccumulatedTime += dt;
  for (;
    mAccumulatedTime >= timeForOneFrame;
    mAccumulatedTime -= timeForOneFrame
  ) {
    updateGameLogic(timeForOneFrame);
    engine::updateAnimatedSprites(mEntities.entities);
    mEntities.systems.system<RenderingSystem>()->updateAnimatedMapTiles();
    mHudRenderer.updateAnimation();

    if (mEarthQuakeEffect) {
      screenShakeOffsetX = mEarthQuakeEffect->update();
    }
  }


  // **********************************************************************
  // Rendering
  // **********************************************************************
  mpSystems->mMapScrollSystem.updateScrollOffset();

  {
    engine::RenderTargetTexture::Binder
      bindRenderTarget(mIngameViewPortRenderTarget, mpRenderer);
    mEntities.systems.update<RenderingSystem>(dt);
    mEntities.systems.update<DebuggingSystem>(dt);
    mHudRenderer.render();
  }

  mpRenderer->clear();
  mIngameViewPortRenderTarget.render(
    mpRenderer,
    data::GameTraits::inGameViewPortOffset.x + screenShakeOffsetX,
    data::GameTraits::inGameViewPortOffset.y);

  if (mShowDebugText) {
    showDebugText();
  }

  handlePlayerDeath();
  handleLevelExit();
  handleTeleporter();
}


void IngameMode::updateGameLogic(const engine::TimeDelta dt) {
  engine::markActiveEntities(mEntities.entities, mScrollOffset);

  // TODO: Use Systems struct for everything, stop using entityx::SystemManager
  // entirely

  // ----------------------------------------------------------------------
  // Player logic update
  // ----------------------------------------------------------------------
  // TODO: Move all player related systems into the player namespace
  mpSystems->mElevatorSystem.update(mEntities.entities, mCombinedInputState);
  mpSystems->mPlayerMovementSystem.update(mCombinedInputState);
  mpSystems->mPlayerAttackSystem.update();
  mEntities.systems.update<PlayerInteractionSystem>(dt);

  mCombinedInputState = mInputState;

  // ----------------------------------------------------------------------
  // A.I. logic update
  // ----------------------------------------------------------------------
  mpSystems->mBlueGuardSystem.update(mEntities.entities);
  mpSystems->mHoverBotSystem.update(mEntities.entities);
  mEntities.systems.update<ai::LaserTurretSystem>(dt);
  mEntities.systems.update<ai::MessengerDroneSystem>(dt);
  mEntities.systems.update<ai::PrisonerSystem>(dt);
  mEntities.systems.update<ai::RocketTurretSystem>(dt);
  mEntities.systems.update<ai::SecurityCameraSystem>(dt);
  mEntities.systems.update<ai::SlidingDoorSystem>(dt);
  mpSystems->mSlimeBlobSystem.update(mEntities.entities);
  mEntities.systems.update<ai::SlimePipeSystem>(dt);

  // ----------------------------------------------------------------------
  // Physics and other updates
  // ----------------------------------------------------------------------
  mEntities.systems.update<PhysicsSystem>(dt);

  mEntities.systems.update<player::DamageSystem>(dt);
  mEntities.systems.update<DamageInflictionSystem>(dt);
  mEntities.systems.update<player::AnimationSystem>(dt);

  mpSystems->mMapScrollSystem.updateManualScrolling(dt);

  mEntities.systems.update<engine::LifeTimeSystem>(dt);
}


bool IngameMode::levelFinished() const {
  return mLevelFinished;
}


void IngameMode::showLoadingScreen(
  const int episode,
  const loader::ResourceLoader& resources
) {
  mpServiceProvider->fadeOutScreen();
  mpServiceProvider->playMusic("MENUSNG2.IMF");
  {
    const auto loadingScreenTexture = ui::fullScreenImageAsTexture(
      mpRenderer,
      resources,
      loadingScreenFileName(episode));
    loadingScreenTexture.render(mpRenderer, 0, 0);
    mpRenderer->submitBatch();
  }
  mpServiceProvider->fadeInScreen();
}


void IngameMode::loadLevel(
  const int episode,
  const int levelNumber,
  const data::Difficulty difficulty,
  const loader::ResourceLoader& resources
) {
  auto loadedLevel = loader::loadLevel(
    levelFileName(episode, levelNumber), resources, difficulty);
  mPlayerEntity = mEntityFactory.createEntitiesForLevel(loadedLevel.mActors);

  mLevelData = LevelData{
    std::move(loadedLevel.mMap),
    std::move(loadedLevel.mActors),
    loadedLevel.mBackdropSwitchCondition
  };
  mMapAtLevelStart = mLevelData.mMap;

  mEntities.systems.add<PhysicsSystem>(
    &mLevelData.mMap);
  mEntities.systems.add<game_logic::player::AnimationSystem>(
    mPlayerEntity,
    mpServiceProvider,
    &mEntityFactory);
  mEntities.systems.add<game_logic::player::DamageSystem>(
    mPlayerEntity,
    &mPlayerModel,
    mpServiceProvider,
    difficulty);
  mEntities.systems.add<RenderingSystem>(
    &mScrollOffset,
    mpRenderer,
    &mLevelData.mMap,
    std::move(loadedLevel.mTileSetImage),
    std::move(loadedLevel.mBackdropImage),
    std::move(loadedLevel.mSecondaryBackdropImage),
    loadedLevel.mBackdropScrollMode);
  mEntities.systems.add<PlayerInteractionSystem>(
    mPlayerEntity,
    &mPlayerModel,
    mpServiceProvider,
    [this](const entityx::Entity& teleporter) {
      mActiveTeleporter = teleporter;
    });
  mEntities.systems.add<DamageInflictionSystem>(
    &mPlayerModel,
    &mLevelData.mMap,
    mpServiceProvider);
  mEntities.systems.add<ai::MessengerDroneSystem>(mPlayerEntity);
  mEntities.systems.add<ai::PrisonerSystem>(mPlayerEntity, &mRandomGenerator);
  mEntities.systems.add<ai::LaserTurretSystem>(
    mPlayerEntity,
    &mPlayerModel,
    &mEntityFactory,
    mpServiceProvider);
  mEntities.systems.add<ai::RocketTurretSystem>(
    mPlayerEntity,
    &mEntityFactory,
    mpServiceProvider);
  mEntities.systems.add<ai::SecurityCameraSystem>(mPlayerEntity);
  mEntities.systems.add<ai::SlidingDoorSystem>(
    mPlayerEntity,
    mpServiceProvider);
  mEntities.systems.add<ai::SlimePipeSystem>(
    &mEntityFactory,
    mpServiceProvider);
  mEntities.systems.add<engine::LifeTimeSystem>();
  mEntities.systems.add<DebuggingSystem>(
    mpRenderer,
    &mScrollOffset,
    &mLevelData.mMap);
  mEntities.systems.configure();

  mpSystems = std::make_unique<Systems>(
    &mScrollOffset,
    mPlayerEntity,
    &mPlayerModel,
    &mLevelData.mMap,
    mpServiceProvider,
    &mEntityFactory,
    &mRandomGenerator,
    mLevelData.mMap,
    [this](
      const game_logic::ProjectileType type,
      const WorldPosition& pos,
      const game_logic::ProjectileDirection direction
    ) {
      mEntityFactory.createProjectile(type, pos, direction);
    });

  mEntities.systems.system<DamageInflictionSystem>()->entityHitSignal().connect(
    [this](entityx::Entity entity) {
      mpSystems->mBlueGuardSystem.onEntityHit(entity);
      mpSystems->mSlimeBlobSystem.onEntityHit(entity);
      mEntities.systems.system<ai::LaserTurretSystem>()->onEntityHit(entity);
      mEntities.systems.system<ai::PrisonerSystem>()->onEntityHit(entity);
    });


  if (loadedLevel.mEarthquake) {
    mEarthQuakeEffect =
      engine::EarthQuakeEffect{mpServiceProvider, &mRandomGenerator};
  }

  mpServiceProvider->playMusic(loadedLevel.mMusicFile);
}


void IngameMode::handleLevelExit() {
  using engine::components::Active;
  using game_logic::components::Trigger;
  using game_logic::components::TriggerType;

  mEntities.entities.each<Trigger, WorldPosition, Active>(
    [this](
      entityx::Entity,
      const Trigger& trigger,
      const WorldPosition& triggerPosition,
      const Active&
    ) {
      if (trigger.mType != TriggerType::LevelExit || mLevelFinished) {
        return;
      }

      const auto& playerPosition = *mPlayerEntity.component<WorldPosition>();
      const auto playerBBox = toWorldSpace(
        *mPlayerEntity.component<BoundingBox>(), playerPosition);

      const auto playerAboveOrAtTriggerHeight =
        playerBBox.bottom() <= triggerPosition.y;
      const auto touchingTriggerOnXAxis =
        triggerPosition.x >= playerBBox.left() &&
        triggerPosition.x <= (playerBBox.right() + 1);

      mLevelFinished = playerAboveOrAtTriggerHeight && touchingTriggerOnXAxis;
    });
}


void IngameMode::handlePlayerDeath() {
  const auto& playerState =
    *mPlayerEntity.component<game_logic::components::PlayerControlled>();

  const auto playerDead =
    playerState.mState == player::PlayerState::Dead &&
    mPlayerModel.mHealth <= 0;
  if (playerDead) {
    restartLevel();
  }
}


void IngameMode::restartLevel() {
  mpServiceProvider->fadeOutScreen();

  mLevelData.mMap = mMapAtLevelStart;

  mEntities.entities.reset();
  mPlayerEntity = mEntityFactory.createEntitiesForLevel(
    mLevelData.mInitialActors);

  mPlayerModel = mPlayerModelAtLevelStart;

  updateAndRender(0);

  mpServiceProvider->fadeInScreen();
}


void IngameMode::handleTeleporter() {
  if (!mActiveTeleporter) {
    return;
  }

  mpServiceProvider->playSound(data::SoundId::Teleport);
  mpServiceProvider->fadeOutScreen();

  game_logic::interaction::teleportPlayer(
    mEntities.entities,
    mPlayerEntity,
    *mActiveTeleporter);
  // It's important to reset mActiveTeleporter before calling updateAndRender,
  // as there would be an infinite recursion otherwise.
  mActiveTeleporter = boost::none;

  const auto switchBackdrop =
    mLevelData.mBackdropSwitchCondition ==
      data::map::BackdropSwitchCondition::OnTeleportation;
  if (switchBackdrop) {
    mEntities.systems.system<RenderingSystem>()->switchBackdrops();
  }

  // Resetting the scroll offset to 0 will cause the scroll position update
  // to set the position as if the player started the level at the teleport
  // destination - which is exactly what we want.
  mScrollOffset = {0, 0};
  updateAndRender(0.0);
  mpServiceProvider->fadeInScreen();
}


void IngameMode::showDebugText() {
  const auto& playerPos = *mPlayerEntity.component<WorldPosition>();
  const auto& playerVel = mPlayerEntity.component<Physical>()->mVelocity;
  std::stringstream infoText;
  infoText
    << "Scroll: " << vec2String(mScrollOffset, 4) << '\n'
    << "Player: " << vec2String(playerPos, 4)
    << ", Vel.: " << vec2String(playerVel, 5) << '\n'
    << "Entities: " << mEntities.entities.size() << '\n'
    << "Sprites rendered: " <<
      mEntities.systems.system<RenderingSystem>()->spritesRendered();

  mpServiceProvider->showDebugText(infoText.str());
}

}
