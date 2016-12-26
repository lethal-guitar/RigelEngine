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

#include "game.hpp"

#include "data/duke_script.hpp"
#include "data/game_traits.hpp"
#include "engine/timing.hpp"
#include "loader/duke_script_loader.hpp"
#include "sdl_utils/error.hpp"

#include "game_session_mode.hpp"
#include "ingame_mode.hpp"
#include "intro_demo_loop_mode.hpp"
#include "menu_mode.hpp"

#include <cassert>
#include <cmath>


namespace rigel {

using namespace sdl_utils;

using RenderTargetBinder = sdl_utils::RenderTargetTexture::Binder;

namespace {

// The game's original 320x200 resolution would give us a 16:10 aspect ratio
// when using square pixels, but monitors of the time had a 4:3 aspect ratio,
// and that's what the game's graphics were designed for (very noticeable e.g.
// with the earth in the Apogee logo). It worked out fine back then because
// CRTs can show non-square pixels, but that's not possible with today's
// screens anymore. Therefore, we need to stretch the image slightly before
// actually rendering it. We do that by rendering the game into a 320x200
// render target, and then stretching that onto our logical display which has a
// slightly bigger vertical resolution in order to get a 4:3 aspect ratio.
const auto ASPECT_RATIO_CORRECTED_VIEW_PORT_HEIGHT = 240;

// By making the logical display bigger than the aspect-ratio corrected
// original resolution, we can show text with debug info (e.g. FPS) without it
// taking up too much space or being hard to read.
const auto SCALE_FACTOR = 2;

const auto LOGICAL_DISPLAY_WIDTH =
  data::GameTraits::viewPortWidthPx * SCALE_FACTOR;
const auto LOGICAL_DISPLAY_HEIGHT =
  ASPECT_RATIO_CORRECTED_VIEW_PORT_HEIGHT * SCALE_FACTOR;

}


Game::Game(const std::string& gamePath, SDL_Renderer* pRenderer)
  : mpRenderer(pRenderer)
  , mResources(gamePath)
  , mIsShareWareVersion(true)
  , mRenderTarget(
      mpRenderer,
      data::GameTraits::viewPortWidthPx,
      data::GameTraits::viewPortHeightPx)
  , mIsRunning(true)
  , mIsMinimized(false)
  , mTextRenderer(pRenderer, mResources)
  , mFpsDisplay(&mTextRenderer)
{
  clearScreen();
  SDL_RenderPresent(mpRenderer);

  throwIfFailed([this]() {
    return SDL_RenderSetLogicalSize(
      mpRenderer,
      LOGICAL_DISPLAY_WIDTH,
      LOGICAL_DISPLAY_HEIGHT);
  });
}


void Game::run(const Options& options) {
  data::forEachSoundId([this](const auto id) {
    mSoundsById.emplace_back(mSoundSystem.addSound(mResources.loadSound(id)));
  });

  const auto preLoadSong = [this](const auto songFile) {
    mLoadedSongs.emplace(
      songFile,
      mSoundSystem.addSong(mResources.loadMusic(songFile)));
  };

  preLoadSong("DUKEIIA.IMF");
  preLoadSong("FANFAREA.IMF");
  preLoadSong("MENUSNG2.IMF");
  preLoadSong("OPNGATEA.IMF");
  preLoadSong("RANGEA.IMF");

  mSoundSystem.reportMemoryUsage();


  mMusicEnabled = options.mEnableMusic;

  // Check if running registered version
  if (
    mResources.mFilePackage.hasFile("LCR.MNI") &&
    mResources.mFilePackage.hasFile("O1.MNI")
  ) {
    mIsShareWareVersion = false;
  }

  if (options.mLevelToJumpTo)
  {
    int episode, level;
    std::tie(episode, level) = *options.mLevelToJumpTo;

    mpCurrentGameMode = std::make_unique<GameSessionMode>(
      episode,
      level,
      data::Difficulty::Medium,
      makeModeContext());
  }
  else if (options.mSkipIntro)
  {
    mpCurrentGameMode = std::make_unique<MenuMode>(makeModeContext());
  }
  else
  {
    mpCurrentGameMode = std::make_unique<IntroDemoLoopMode>(
      makeModeContext(),
      true);
  }

  mainLoop();
}

void Game::mainLoop() {
  assert(mpCurrentGameMode);

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
      RenderTargetBinder bindRenderTarget(mRenderTarget, mpRenderer);

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

    mRenderTarget.renderScaledToScreen(mpRenderer);

    if (!mDebugText.empty()) {
      mTextRenderer.drawMultiLineText(0, 2, mDebugText);
    }

    const auto afterRender = high_resolution_clock::now();
    const auto innerRenderTime =
      duration<engine::TimeDelta>(afterRender - startOfFrame).count();
    mFpsDisplay.updateAndRender(elapsed, innerRenderTime);

    SDL_RenderPresent(mpRenderer);
  }
}


GameMode::Context Game::makeModeContext() {
  return {&mResources, mpRenderer, &mSoundSystem, this};
}


void Game::handleEvent(const SDL_Event& event) {
  switch (event.type) {
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

  sdl_utils::DefaultRenderTargetBinder bindDefaultRenderTarget(mpRenderer);

  // We use the previous frame's mLastTime here as initial value
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

    clearScreen();

    mRenderTarget.renderScaledToScreen(mpRenderer);
    SDL_RenderPresent(mpRenderer);

    if (fadeFactor >= 1.0) {
      break;
    }
  }
}


void Game::fadeOutScreen() {
  performScreenFadeBlocking(false);

  // Clear render canvas after a fade-out
  RenderTargetBinder bindRenderTarget(mRenderTarget, mpRenderer);
  clearScreen();
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


void Game::playMusic(const std::string& name) {
  if (!mMusicEnabled) {
    return;
  }

  auto loadedSongIter = mLoadedSongs.find(name);
  if (loadedSongIter == mLoadedSongs.end()) {
    const auto handle = mSoundSystem.addSong(mResources.loadMusic(name));
    loadedSongIter = mLoadedSongs.emplace(name, handle).first;
  }

  mSoundSystem.playSong(loadedSongIter->second);
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


void Game::clearScreen() {
  SDL_SetRenderDrawColor(mpRenderer, 0, 0, 0, 255);
  SDL_RenderClear(mpRenderer);
}

}
