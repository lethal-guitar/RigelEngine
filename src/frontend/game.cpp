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

#include "game.hpp"

#include "base/defer.hpp"
#include "base/math_tools.hpp"
#include "data/duke_script.hpp"
#include "data/game_traits.hpp"
#include "engine/timing.hpp"
#include "game_logic/demo_player.hpp"
#include "loader/duke_script_loader.hpp"
#include "renderer/upscaling_utils.hpp"
#include "ui/imgui_integration.hpp"

#include "anti_piracy_screen_mode.hpp"
#include "game_session_mode.hpp"
#include "intro_demo_loop_mode.hpp"
#include "menu_mode.hpp"
#include "platform.hpp"

RIGEL_DISABLE_WARNINGS
#include <imgui.h>
RIGEL_RESTORE_WARNINGS

namespace rigel {

using namespace engine;
using RenderTargetBinder = renderer::RenderTargetTexture::Binder;

namespace {

/** Returns game path to be used for loading resources
 *
 * A game path specified on the command line takes priority over the path
 * stored in the user profile. This function implements that priority by
 * returning the right path based on the given command line options and user
 * profile.
 */
std::string effectiveGamePath(
  const CommandLineOptions& options,
  const UserProfile& profile
) {
  using namespace std::string_literals;

  if (!options.mGamePath.empty()) {
    return options.mGamePath;
  }

  return profile.mGamePath ? profile.mGamePath->u8string() + "/"s : ""s;
}


auto wrapWithInitialFadeIn(std::unique_ptr<GameMode> mode) {
  class InitialFadeInWrapper : public GameMode {
  public:
    explicit InitialFadeInWrapper(std::unique_ptr<GameMode> pModeToSwitchTo)
      : mpModeToSwitchTo(std::move(pModeToSwitchTo))
    {
    }

    std::unique_ptr<GameMode> updateAndRender(
      engine::TimeDelta,
      const std::vector<SDL_Event>&
    ) override {
      return std::move(mpModeToSwitchTo);
    }

  private:
    std::unique_ptr<GameMode> mpModeToSwitchTo;
  };

  return std::make_unique<InitialFadeInWrapper>(std::move(mode));
}


auto loadScripts(const loader::ResourceLoader& resources) {
  auto allScripts = resources.loadScriptBundle("TEXT.MNI");
  const auto optionsScripts = resources.loadScriptBundle("OPTIONS.MNI");
  const auto orderInfoScripts = resources.loadScriptBundle("ORDERTXT.MNI");

  allScripts.insert(std::begin(optionsScripts), std::end(optionsScripts));
  allScripts.insert(std::begin(orderInfoScripts), std::end(orderInfoScripts));

  return allScripts;
}


[[nodiscard]] auto setupSimpleUpscaling(renderer::Renderer* pRenderer) {
  auto saved = renderer::Renderer::StateSaver{pRenderer};

  const auto [offset, size, scale] =
    renderer::determineViewPort(pRenderer);
  pRenderer->setGlobalScale(scale);
  pRenderer->setGlobalTranslation(offset);
  pRenderer->setClipRect(base::Rect<int>{offset, size});

  return saved;
}


std::unique_ptr<GameMode> createInitialGameMode(
  GameMode::Context context,
  const CommandLineOptions& commandLineOptions,
  const bool isSharewareVersion,
  const bool isFirstLaunch)
{
  class DemoTestMode : public GameMode {
  public:
    explicit DemoTestMode(Context context)
      : mDemoPlayer(context)
    {
    }

    std::unique_ptr<GameMode> updateAndRender(
      engine::TimeDelta dt,
      const std::vector<SDL_Event>&
    ) override {
      mDemoPlayer.updateAndRender(dt);
      return nullptr;
    }

  private:
    game_logic::DemoPlayer mDemoPlayer;
  };

  if (commandLineOptions.mLevelToJumpTo)
  {
    return std::make_unique<GameSessionMode>(
      *commandLineOptions.mLevelToJumpTo,
      context,
      commandLineOptions.mPlayerPosition);
  }
  else if (commandLineOptions.mSkipIntro)
  {
    return std::make_unique<MenuMode>(context);
  }
  else if (commandLineOptions.mPlayDemo) {
    return std::make_unique<DemoTestMode>(context);
  }

  if (!isSharewareVersion) {
    return std::make_unique<AntiPiracyScreenMode>(context, isFirstLaunch);
  }

  return std::make_unique<IntroDemoLoopMode>(
    context,
    isFirstLaunch
      ? IntroDemoLoopMode::Type::AtFirstLaunch
      : IntroDemoLoopMode::Type::DuringGameStart);
}


std::optional<renderer::FpsLimiter> createLimiter(
  const data::GameOptions& options
) {
  if (options.mEnableFpsLimit && !options.mEnableVsync) {
    return renderer::FpsLimiter{options.mMaxFps};
  } else {
    return std::nullopt;
  }
}

}


Game::Game(
  const CommandLineOptions& commandLineOptions,
  UserProfile* pUserProfile,
  SDL_Window* pWindow,
  const bool isFirstLaunch
)
  : mpWindow(pWindow)
  , mRenderer(pWindow)
  , mResources(effectiveGamePath(commandLineOptions, *pUserProfile))
  , mpSoundSystem([this]() {
      std::unique_ptr<engine::SoundSystem> pResult;
      try {
        pResult = std::make_unique<engine::SoundSystem>(mResources);
      } catch (const std::exception& ex) {
        std::cerr << "WARNING: Failed to initialize audio: " << ex.what() << '\n';
      }

      return pResult;
    }())
  , mIsShareWareVersion([this]() {
      // The registered version has 24 additional level files, and a
      // "anti-piracy" image (LCR.MNI). But we don't check for the presence of
      // all of these files, as that would be fairly tedious. Instead, we just
      // check for the presence of one of the registered version's levels, and
      // the anti-piracy screen, and assume that we're dealing with a
      // registered version data set if these two are present.
      const auto hasRegisteredVersionFiles =
        mResources.hasFile("LCR.MNI") && mResources.hasFile("O1.MNI");
      return !hasRegisteredVersionFiles;
    }())
  , mFpsLimiter(createLimiter(pUserProfile->mOptions))
  , mRenderTarget(
      &mRenderer,
      mRenderer.maxWindowSize().width,
      mRenderer.maxWindowSize().height)
  , mIsRunning(true)
  , mIsMinimized(false)
  , mCommandLineOptions(commandLineOptions)
  , mpUserProfile(pUserProfile)
  , mScriptRunner(&mResources, &mRenderer, &mpUserProfile->mSaveSlots, this)
  , mAllScripts(loadScripts(mResources))
  , mUiSpriteSheet(
      renderer::OwningTexture{
        &mRenderer, mResources.loadTiledFullscreenImage("STATUS.MNI")},
      &mRenderer)
  , mTextRenderer(&mUiSpriteSheet, &mRenderer, mResources)
{
  applyChangedOptions();

  mpCurrentGameMode = wrapWithInitialFadeIn(createInitialGameMode(
    makeModeContext(), mCommandLineOptions, mIsShareWareVersion, isFirstLaunch));

  enumerateGameControllers();

  mLastTime = base::Clock::now();
}


auto Game::runOneFrame() -> std::optional<StopReason> {
  using namespace std::chrono;
  using base::defer;

  const auto startOfFrame = base::Clock::now();
  const auto elapsed =
    duration<entityx::TimeDelta>(startOfFrame - mLastTime).count();
  mLastTime = startOfFrame;

  pumpEvents();
  if (!mIsRunning) {
    return StopReason::GameEnded;
  }

  {
    ui::imgui_integration::beginFrame(mpWindow);
    auto imGuiFrameGuard = defer([]() { ui::imgui_integration::endFrame(); });
    ImGui::SetMouseCursor(ImGuiMouseCursor_None);

    updateAndRender(elapsed);
    mEventQueue.clear();
  }

  swapBuffers();

  applyChangedOptions();

  if (!mGamePathToSwitchTo.empty()) {
    mpUserProfile->mGamePath = mGamePathToSwitchTo;
    mpUserProfile->saveToDisk();
    return StopReason::RestartNeeded;
  }

  return {};
}


void Game::pumpEvents() {
  SDL_Event event;
  while (mIsMinimized && SDL_WaitEvent(&event)) {
    if (!handleEvent(event)) {
      mEventQueue.push_back(event);
    }
  }

  while (SDL_PollEvent(&event)) {
    if (!handleEvent(event)) {
      mEventQueue.push_back(event);
    }
  }
}


void Game::updateAndRender(const entityx::TimeDelta elapsed) {
  {
    RenderTargetBinder bindRenderTarget(mRenderTarget, &mRenderer);
    mRenderer.clear();

    auto saved = setupSimpleUpscaling(&mRenderer);

    auto pMaybeNextMode =
      mpCurrentGameMode->updateAndRender(elapsed, mEventQueue);

    if (pMaybeNextMode) {
      fadeOutScreen();
      mpCurrentGameMode = std::move(pMaybeNextMode);
      mpCurrentGameMode->updateAndRender(0, {});
      fadeInScreen();
    }
  }

  if (mAlphaMod != 0) {
    mRenderer.clear();
    mRenderTarget.render(&mRenderer, 0, 0);
    mRenderer.submitBatch();

    if (mpUserProfile->mOptions.mShowFpsCounter) {
      mFpsDisplay.updateAndRender(elapsed);
    }
  }
}


GameMode::Context Game::makeModeContext() {
  return {
    &mResources,
    &mRenderer,
    this,
    &mScriptRunner,
    &mAllScripts,
    &mTextRenderer,
    &mUiSpriteSheet,
    mpUserProfile};
}


bool Game::handleEvent(const SDL_Event& event) {
  if (ui::imgui_integration::handleEvent(event)) {
    return true;
  }

  auto& options = mpUserProfile->mOptions;
  switch (event.type) {
    case SDL_KEYUP:
      if (event.key.keysym.sym == SDLK_F6) {
        options.mShowFpsCounter = !options.mShowFpsCounter;
      }
      return false;

    case SDL_QUIT:
      mIsRunning = false;
      break;

    case SDL_WINDOWEVENT:
      switch (event.window.event) {
        case SDL_WINDOWEVENT_MINIMIZED:
          mIsMinimized = true;
          break;

        case SDL_WINDOWEVENT_RESTORED:
          mIsMinimized = false;
          break;

        case SDL_WINDOWEVENT_SIZE_CHANGED:
          if (options.mWindowMode == data::WindowMode::Windowed) {
            options.mWindowWidth = event.window.data1;
            options.mWindowHeight = event.window.data2;
          }
          break;

        case SDL_WINDOWEVENT_MOVED:
          if (options.mWindowMode == data::WindowMode::Windowed) {
            options.mWindowPosX = event.window.data1;
            options.mWindowPosY = event.window.data2;
          }
          break;
      }
      break;

    case SDL_CONTROLLERDEVICEADDED:
    case SDL_CONTROLLERDEVICEREMOVED:
      enumerateGameControllers();
      break;

    default:
      return false;
  }

  return true;
}


void Game::performScreenFadeBlocking(const FadeType type) {
  using namespace std::chrono;

  if (
    (type == FadeType::In && mAlphaMod == 255) ||
    (type == FadeType::Out && mAlphaMod == 0)
  ) {
    // Already faded in/out, nothing to do
    return;
  }

  renderer::DefaultRenderTargetBinder bindDefaultRenderTarget(&mRenderer);
  auto saved = renderer::setupDefaultState(&mRenderer);

  auto startTime = base::Clock::now();

  while (mIsRunning) {
    const auto now = base::Clock::now();
    const auto elapsedTime = duration<double>(now - startTime).count();
    const auto fastTicksElapsed = engine::timeToFastTicks(elapsedTime);
    const auto fadeFactor =
      std::clamp((fastTicksElapsed / 4.0) / 16.0, 0.0, 1.0);
    const auto alpha = type == FadeType::In ? fadeFactor : 1.0 - fadeFactor;
    mAlphaMod =
      base::roundTo<std::uint8_t>(255.0 * alpha);

    mRenderer.clear();

    mRenderer.setColorModulation({255, 255, 255, mAlphaMod});
    mRenderTarget.render(&mRenderer, 0, 0);
    swapBuffers();

    if (fadeFactor >= 1.0) {
      break;
    }
  }

  mRenderer.setColorModulation({255, 255, 255, 255});

  // Pretend that the fade didn't take any time
  mLastTime = base::Clock::now();
}


void Game::swapBuffers() {
  mRenderer.swapBuffers();

  if (mFpsLimiter) {
    mFpsLimiter->updateAndWait();
  }
}


void Game::applyChangedOptions() {
  const auto& currentOptions = mpUserProfile->mOptions;

  if (currentOptions.mWindowMode != mPreviousOptions.mWindowMode) {
    const auto result = SDL_SetWindowFullscreen(
      mpWindow, platform::flagsForWindowMode(currentOptions.mWindowMode));

    if (result != 0) {
      std::cerr <<
        "WARNING: Failed to set window mode: " << SDL_GetError() << '\n';
      mpUserProfile->mOptions.mWindowMode = mPreviousOptions.mWindowMode;
    } else {
      if (currentOptions.mWindowMode == data::WindowMode::Windowed) {
        SDL_SetWindowSize(
          mpWindow, currentOptions.mWindowWidth, currentOptions.mWindowHeight);
      }
    }
  }

  if (currentOptions.mEnableVsync != mPreviousOptions.mEnableVsync) {
    SDL_GL_SetSwapInterval(mpUserProfile->mOptions.mEnableVsync ? 1 : 0);
  }

  if (
    currentOptions.mEnableVsync != mPreviousOptions.mEnableVsync ||
    currentOptions.mEnableFpsLimit != mPreviousOptions.mEnableFpsLimit ||
    currentOptions.mMaxFps != mPreviousOptions.mMaxFps
  ) {
    mFpsLimiter = createLimiter(currentOptions);
  }

  if (mpSoundSystem) {
    if (
      currentOptions.mMusicVolume != mPreviousOptions.mMusicVolume ||
      currentOptions.mMusicOn != mPreviousOptions.mMusicOn
    ) {
      const auto newVolume = currentOptions.mMusicOn
        ? currentOptions.mMusicVolume
        : 0.0f;
      mpSoundSystem->setMusicVolume(newVolume);
    }

    if (
      currentOptions.mSoundVolume != mPreviousOptions.mSoundVolume ||
      currentOptions.mSoundOn != mPreviousOptions.mSoundOn
    ) {
      const auto newVolume = currentOptions.mSoundOn
        ? currentOptions.mSoundVolume
        : 0.0f;
      mpSoundSystem->setSoundVolume(newVolume);
    }
  }

  mPreviousOptions = mpUserProfile->mOptions;
}


void Game::enumerateGameControllers() {
  // TODO : support multiple controllers.
  // At the moment, this opens only the first available controller.

  mpGameController.reset();

  for (std::uint8_t i = 0; i < SDL_NumJoysticks(); ++i) {
    if (SDL_IsGameController(i)) {
      mpGameController =
        sdl_utils::Ptr<SDL_GameController>{SDL_GameControllerOpen(i)};
      break;
    }
  }
}


void Game::fadeOutScreen() {
  performScreenFadeBlocking(FadeType::Out);

  // Clear render canvas after a fade-out
  RenderTargetBinder bindRenderTarget(mRenderTarget, &mRenderer);
  auto saved = renderer::setupDefaultState(&mRenderer);

  mRenderer.clear();
}


void Game::fadeInScreen() {
  performScreenFadeBlocking(FadeType::In);
}


void Game::playSound(const data::SoundId id) {
  if (mpSoundSystem) {
    mpSoundSystem->playSound(id);
  }
}


void Game::stopSound(const data::SoundId id) {
  if (mpSoundSystem) {
    mpSoundSystem->stopSound(id);
  }
}


void Game::playMusic(const std::string& name) {
  if (mpSoundSystem) {
    mpSoundSystem->playSong(mResources.loadMusic(name));
  }
}


void Game::stopMusic() {
  if (mpSoundSystem) {
    mpSoundSystem->stopMusic();
  }
}


void Game::scheduleGameQuit() {
  mIsRunning = false;
}


void Game::switchGamePath(const std::filesystem::path& newGamePath) {
  if (newGamePath != mpUserProfile->mGamePath) {
    mGamePathToSwitchTo = newGamePath;
  }
}

}
