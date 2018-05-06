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

#include "game_main.hpp"
#include "game_main.ipp"

#include "data/duke_script.hpp"
#include "data/game_traits.hpp"
#include "engine/timing.hpp"
#include "loader/duke_script_loader.hpp"
#include "sdl_utils/error.hpp"

#include "game_session_mode.hpp"
#include "intro_demo_loop_mode.hpp"
#include "menu_mode.hpp"

#include <cassert>
#include <cmath>


namespace rigel {

using namespace engine;
using namespace sdl_utils;

using RenderTargetBinder = engine::RenderTargetTexture::Binder;

namespace {

struct NullGameMode : public GameMode {
  void handleEvent(const SDL_Event&) override {}
  void updateAndRender(engine::TimeDelta) override {}
};

}


void gameMain(const StartupOptions& options, SDL_Window* pWindow) {
  Game game(options.mGamePath, pWindow);
  game.run(options);
}


Game::Game(const std::string& gamePath, SDL_Window* pWindow)
  : mRenderer(pWindow)
  , mResources(gamePath)
  , mIsShareWareVersion(true)
  , mRenderTarget(
      &mRenderer,
      data::GameTraits::viewPortWidthPx,
      data::GameTraits::viewPortHeightPx)
  , mpCurrentGameMode(std::make_unique<NullGameMode>())
  , mIsRunning(true)
  , mIsMinimized(false)
  , mScriptRunner(&mResources, &mRenderer, this)
  , mTextRenderer(&mRenderer, mResources)
  , mFpsDisplay(&mTextRenderer)
{
}


void Game::run(const StartupOptions& startupOptions) {
  mRenderer.clear();
  mRenderer.swapBuffers();

  data::forEachSoundId([this](const auto id) {
    mSoundsById.emplace_back(mSoundSystem.addSound(mResources.loadSound(id)));
  });

  mMusicEnabled = startupOptions.mEnableMusic;

  // Check if running registered version
  if (
    mResources.mFilePackage.hasFile("LCR.MNI") &&
    mResources.mFilePackage.hasFile("O1.MNI")
  ) {
    mIsShareWareVersion = false;
  }

  if (startupOptions.mLevelToJumpTo)
  {
    int episode, level;
    std::tie(episode, level) = *startupOptions.mLevelToJumpTo;

    mpNextGameMode = std::make_unique<GameSessionMode>(
      episode,
      level,
      data::Difficulty::Medium,
      makeModeContext(),
      startupOptions.mPlayerPosition);
  }
  else if (startupOptions.mSkipIntro)
  {
    mpNextGameMode = std::make_unique<MenuMode>(makeModeContext());
  }
  else
  {
    if (!mIsShareWareVersion) {
      showAntiPiracyScreen();
    }
    mpNextGameMode = std::make_unique<IntroDemoLoopMode>(
      makeModeContext(),
      true);
  }

  mainLoop();
}


void Game::showAntiPiracyScreen() {
  auto antiPiracyImage = mResources.loadAntiPiracyImage();
  engine::OwningTexture imageTexture(&mRenderer, antiPiracyImage);
  imageTexture.renderScaledToScreen(&mRenderer);
  mRenderer.submitBatch();
  mRenderer.swapBuffers();

  SDL_Event event;
  while (SDL_WaitEvent(&event) && event.type != SDL_KEYDOWN);
}


void Game::mainLoop() {
  using namespace std::chrono;

  SDL_Event event;
  mLastTime = high_resolution_clock::now();

  for (;;) {
    const auto startOfFrame = high_resolution_clock::now();
    const auto elapsed =
      duration<entityx::TimeDelta>(startOfFrame - mLastTime).count();
    mLastTime = startOfFrame;

    mDebugText.clear();

    {
      RenderTargetBinder bindRenderTarget(mRenderTarget, &mRenderer);

      while (mIsMinimized && SDL_WaitEvent(&event)) {
        handleEvent(event);
      }
      while (SDL_PollEvent(&event)) {
        handleEvent(event);
      }
      if (!mIsRunning) {
        break;
      }

      if (mpNextGameMode) {
        fadeOutScreen();
        mpCurrentGameMode = std::move(mpNextGameMode);
        mpCurrentGameMode->updateAndRender(0);
        fadeInScreen();
      }

      mpCurrentGameMode->updateAndRender(elapsed);
    }

    mRenderer.clear();
    mRenderTarget.renderScaledToScreen(&mRenderer);

    if (!mDebugText.empty()) {
      mTextRenderer.drawMultiLineText(0, 2, mDebugText);
    }

    if (mShowFps) {
      const auto afterRender = high_resolution_clock::now();
      const auto innerRenderTime =
        duration<engine::TimeDelta>(afterRender - startOfFrame).count();
      mFpsDisplay.updateAndRender(elapsed, innerRenderTime);
    }

    mRenderer.swapBuffers();
  }
}


GameMode::Context Game::makeModeContext() {
  return {&mResources, &mRenderer, this, &mScriptRunner};
}


void Game::handleEvent(const SDL_Event& event) {
  switch (event.type) {
    case SDL_KEYUP:
      if (event.key.keysym.sym == SDLK_F6) {
        mShowFps = !mShowFps;
      }
      mpCurrentGameMode->handleEvent(event);
      break;

    case SDL_QUIT:
      mIsRunning = false;
      break;

    case SDL_WINDOWEVENT:
      if (event.window.event == SDL_WINDOWEVENT_MINIMIZED) {
        mIsMinimized = true;
      } else if (event.window.event == SDL_WINDOWEVENT_RESTORED) {
        mIsMinimized = false;
      }
      break;

    default:
      mpCurrentGameMode->handleEvent(event);
      break;
  }
}


void Game::performScreenFadeBlocking(const bool doFadeIn) {
  using namespace std::chrono;

  if (
    (doFadeIn && mRenderTarget.alphaMod() == 255) ||
    (!doFadeIn && mRenderTarget.alphaMod() == 0)
  ) {
    // Already faded in/out, nothing to do
    return;
  }

  engine::DefaultRenderTargetBinder bindDefaultRenderTarget(&mRenderer);

  engine::TimeDelta elapsedTime = 0.0;

  while (mIsRunning) {
    const auto now = high_resolution_clock::now();
    const auto timeDelta = duration<double>(now - mLastTime).count();
    mLastTime = now;

    elapsedTime += timeDelta;
    const auto fastTicksElapsed = engine::timeToFastTicks(elapsedTime);
    const auto fadeFactor = (fastTicksElapsed / 4.0) / 16.0;

    if (fadeFactor < 1.0) {
      const auto alpha = doFadeIn ? fadeFactor : 1.0 - fadeFactor;
      const auto mod = static_cast<int>(std::round(255.0 * alpha));
      mRenderTarget.setAlphaMod(mod);
    } else {
      mRenderTarget.setAlphaMod(doFadeIn ? 255 : 0);
    }

    mRenderer.clear();

    mRenderTarget.renderScaledToScreen(&mRenderer);
    mRenderer.swapBuffers();

    if (fadeFactor >= 1.0) {
      break;
    }
  }
}


void Game::fadeOutScreen() {
  performScreenFadeBlocking(false);

  // Clear render canvas after a fade-out
  RenderTargetBinder bindRenderTarget(mRenderTarget, &mRenderer);
  mRenderer.clear();
}


void Game::fadeInScreen() {
  performScreenFadeBlocking(true);
}


void Game::playSound(const data::SoundId id) {
  const auto index = static_cast<std::size_t>(id);
  assert(index < mSoundsById.size());

  const auto handle = mSoundsById[index];
  mSoundSystem.playSound(handle);
}


void Game::stopSound(const data::SoundId id) {
  const auto index = static_cast<std::size_t>(id);
  assert(index < mSoundsById.size());

  const auto handle = mSoundsById[index];
  mSoundSystem.stopSound(handle);
}


void Game::playMusic(const std::string& name) {
  if (!mMusicEnabled) {
    return;
  }

  mSoundSystem.playSong(mResources.loadMusic(name));
}


void Game::stopMusic() {
  mSoundSystem.stopMusic();
}


void Game::scheduleNewGameStart(
  const int episode,
  const data::Difficulty difficulty
) {
  mpNextGameMode = std::make_unique<GameSessionMode>(
    episode,
    0,
    difficulty,
    makeModeContext());
}


void Game::scheduleEnterMainMenu() {
  mpNextGameMode = std::make_unique<MenuMode>(makeModeContext());
}


void Game::scheduleGameQuit() {
  mIsRunning = false;
}


void Game::showDebugText(const std::string& text) {
  mDebugText = text;
}

}
