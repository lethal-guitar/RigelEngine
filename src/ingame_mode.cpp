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

#include <data/game_traits.hpp>
#include <data/map.hpp>
#include <data/sound_ids.hpp>
#include <engine/physics_system.hpp>
#include <engine/rendering_system.hpp>
#include <game_logic/player_interaction_system.hpp>
#include <game_logic/trigger_components.hpp>
#include <loader/resource_loader.hpp>
#include <ui/utils.hpp>

#include <cassert>
#include <chrono>
#include <iostream>


namespace rigel {

using namespace engine;
using namespace std;

using game_logic::PlayerControlSystem;
using game_logic::PlayerInteractionSystem;
using game_logic::MapScrollSystem;
using data::PlayerModel;


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

}


IngameMode::IngameMode(
  const int episode,
  const int levelNumber,
  const data::Difficulty difficulty,
  Context context
)
  : mpRenderer(context.mpRenderer)
  , mpServiceProvider(context.mpServiceProvider)
  , mLevelFinished(false)
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

  auto after = high_resolution_clock::now();
  std::cout << "Level load time: " <<
    duration<double>(after - before).count() * 1000.0 << " ms\n";
}


void IngameMode::handleEvent(const SDL_Event& event) {
  if (event.type != SDL_KEYDOWN && event.type != SDL_KEYUP) {
    return;
  }

  const auto keyPressed = event.type == SDL_KEYDOWN;
  switch (event.key.keysym.sym) {
    case SDLK_UP:
      mPlayerInputs.mMovingUp = keyPressed;
      break;
    case SDLK_DOWN:
      mPlayerInputs.mMovingDown = keyPressed;
      break;
    case SDLK_LEFT:
      mPlayerInputs.mMovingLeft = keyPressed;
      break;
    case SDLK_RIGHT:
      mPlayerInputs.mMovingRight = keyPressed;
      break;
    case SDLK_LCTRL:
    case SDLK_RCTRL:
      mPlayerInputs.mJumping = keyPressed;
      break;
  }
}


void IngameMode::updateAndRender(engine::TimeDelta dt) {
  // ----------
  // updating
  if (!mLevelFinished) {
    mEntities.systems.update<PlayerControlSystem>(dt);
    mEntities.systems.update<PlayerInteractionSystem>(dt);
    mEntities.systems.update<PhysicsSystem>(dt);
    mEntities.systems.update<MapScrollSystem>(dt);
    mHudRenderer.update(dt);

    checkForLevelExitReached();
  }

  // ----------
  // rendering
  if (mLevelFinished) {
    dt = 0;
  }

  {
    sdl_utils::RenderTargetTexture::Binder
      bindRenderTarget(mIngameViewPortRenderTarget, mpRenderer);

    mEntities.systems.update<RenderingSystem>(dt);
    mHudRenderer.render();
  }

  mIngameViewPortRenderTarget.render(
    mpRenderer,
    data::GameTraits::inGameViewPortOffset.x,
    data::GameTraits::inGameViewPortOffset.y);
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
  auto entityBundle = game_logic::createEntitiesForLevel(
    loadedLevel,
    mpRenderer,
    resources.mActorImagePackage,
    mEntities.entities);
  mPlayerEntity = entityBundle.mPlayerEntity;

  mSpriteTextures = std::move(entityBundle.mSpriteTextures);

  mLevelData = LevelData{
    std::move(loadedLevel.mMap),
    std::move(loadedLevel.mTileSet.mAttributes),
    std::move(loadedLevel.mActors)
  };

  mEntities.systems.add<PhysicsSystem>(
    mLevelData.mMap,
    mLevelData.mTileAttributes);
  mEntities.systems.add<game_logic::PlayerControlSystem>(
    mPlayerEntity,
    &mPlayerInputs,
    mLevelData.mMap,
    mLevelData.mTileAttributes);
  mEntities.systems.add<game_logic::MapScrollSystem>(
    &mScrollOffset,
    mPlayerEntity,
    mLevelData.mMap);
  mEntities.systems.add<RenderingSystem>(
    &mScrollOffset,
    mpRenderer,
    &mLevelData.mMap,
    &mLevelData.mTileAttributes,
    std::move(loadedLevel.mTileSet.mImage),
    std::move(loadedLevel.mBackdropImage),
    std::move(loadedLevel.mSecondaryBackdropImage),
    loadedLevel.mBackdropScrollMode);
  mEntities.systems.add<PlayerInteractionSystem>(
    mPlayerEntity,
    &mPlayerModel,
    mpServiceProvider);
  mEntities.systems.configure();

  mpServiceProvider->playMusic(loadedLevel.mMusicFile);
}


void IngameMode::checkForLevelExitReached() {
  using engine::components::WorldPosition;
  using engine::components::Physical;
  using game_logic::components::Trigger;
  using game_logic::components::TriggerType;

  mEntities.entities.each<Trigger, WorldPosition>(
    [this](
      entityx::Entity,
      const Trigger& trigger,
      const WorldPosition& triggerPosition
    ) {
      if (trigger.mType != TriggerType::LevelExit || mLevelFinished) {
        return;
      }

      const auto& playerPosition =
        *mPlayerEntity.component<WorldPosition>().get();
      const auto playerBBox = toWorldSpace(
        mPlayerEntity.component<Physical>().get()->mCollisionRect,
        playerPosition);

      const auto playerAboveOrAtTriggerHeight =
        playerBBox.bottom() <= triggerPosition.y;
      const auto touchingTriggerOnXAxis =
        triggerPosition.x >= playerBBox.left() &&
        triggerPosition.x <= (playerBBox.right() + 1);

      // TODO: Add check for trigger being visible on-screen to properly
      // replicate the original game's behavior

      mLevelFinished = playerAboveOrAtTriggerHeight && touchingTriggerOnXAxis;
    });
}

}
