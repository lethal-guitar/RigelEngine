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
#include <loader/resource_bundle.hpp>
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
  mEntities.systems.update<PlayerControlSystem>(dt);
  mEntities.systems.update<PlayerInteractionSystem>(dt);
  mEntities.systems.update<PhysicsSystem>(dt);
  mEntities.systems.update<MapScrollSystem>(dt);
  mHudRenderer.update(dt);

  // ----------
  // rendering
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


void IngameMode::showLoadingScreen(
  const int episode,
  const loader::ResourceBundle& resources
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
  const loader::ResourceBundle& resources
) {
  auto level = loader::loadLevel(
    levelFileName(episode, levelNumber), resources);
  auto entityBundle = game_logic::createEntities(
    level,
    difficulty,
    mpRenderer,
    resources.mActorImagePackage,
    mEntities.entities);
  mPlayerEntity = entityBundle.mPlayerEntity;

  mSpriteTextures = std::move(entityBundle.mSpriteTextures);

  mEntities.systems.add<PhysicsSystem>(level.mMap);
  mEntities.systems.add<game_logic::PlayerControlSystem>(
    mPlayerEntity,
    &mPlayerInputs,
    level.mMap);
  mEntities.systems.add<game_logic::MapScrollSystem>(
    &mScrollOffset,
    mPlayerEntity,
    level.mMap);
  mEntities.systems.add<RenderingSystem>(
    std::move(level),
    &mScrollOffset,
    mpRenderer);
  mEntities.systems.add<PlayerInteractionSystem>(
    mPlayerEntity,
    &mPlayerModel,
    mpServiceProvider);
  mEntities.systems.configure();

  mpServiceProvider->playMusic(level.mMusicFile);
}

}
