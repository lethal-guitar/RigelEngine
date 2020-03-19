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

#include "base/math_tools.hpp"
#include "data/duke_script.hpp"
#include "data/game_traits.hpp"
#include "engine/timing.hpp"
#include "loader/duke_script_loader.hpp"
#include "renderer/opengl.hpp"
#include "renderer/upscaling_utils.hpp"
#include "sdl_utils/error.hpp"
#include "ui/imgui_integration.hpp"

#include "anti_piracy_screen_mode.hpp"
#include "game_session_mode.hpp"
#include "intro_demo_loop_mode.hpp"
#include "menu_mode.hpp"

RIGEL_DISABLE_WARNINGS
#include <imgui.h>
#include <SDL.h>
RIGEL_RESTORE_WARNINGS

#ifdef _WIN32
#include <Windows.h>
#endif

#include <cassert>


namespace rigel {

using namespace engine;
using namespace sdl_utils;

using RenderTargetBinder = renderer::RenderTargetTexture::Binder;

namespace {

template <typename Callback>
class CallOnDestruction {
public:
  explicit CallOnDestruction(Callback&& callback)
    : mCallback(std::forward<Callback>(callback))
  {
  }

  ~CallOnDestruction() {
    mCallback();
  }

  CallOnDestruction(const CallOnDestruction&) = delete;
  CallOnDestruction& operator=(const CallOnDestruction&) = delete;

private:
  Callback mCallback;
};


template <typename Callback>
[[nodiscard]] auto defer(Callback&& callback) {
  return CallOnDestruction{std::forward<Callback>(callback)};
}


void setGLAttributes() {
#ifdef RIGEL_USE_GL_ES
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
}


int flagsForWindowMode(const data::WindowMode mode) {
  using WM = data::WindowMode;

  switch (mode) {
    case WM::Fullscreen: return SDL_WINDOW_FULLSCREEN_DESKTOP;
    case WM::ExclusiveFullscreen: return SDL_WINDOW_FULLSCREEN;
    case WM::Windowed: return 0;
  }

  return 0;
}


auto createWindow(const data::GameOptions& options) {
  SDL_DisplayMode displayMode;
  sdl_utils::check(SDL_GetDesktopDisplayMode(0, &displayMode));

  const auto isFullscreen = options.mWindowMode != data::WindowMode::Windowed;
  const auto windowFlags =
    flagsForWindowMode(options.mWindowMode) |
    SDL_WINDOW_RESIZABLE |
    SDL_WINDOW_OPENGL;

  auto pWindow = sdl_utils::Ptr<SDL_Window>{sdl_utils::check(SDL_CreateWindow(
    "Rigel Engine",
    options.mWindowPosX,
    options.mWindowPosY,
    isFullscreen ? displayMode.w : options.mWindowWidth,
    isFullscreen ? displayMode.h : options.mWindowHeight,
    windowFlags))};

  // Setting a display mode is necessary to make sure that exclusive
  // full-screen mode keeps using the desktop resolution. Without this,
  // switching to exclusive full-screen mode from windowed mode would result in
  // a screen resolution matching the window's last size.
  sdl_utils::check(SDL_SetWindowDisplayMode(pWindow.get(), &displayMode));

  return pWindow;
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
  const StartupOptions& startupOptions,
  const bool isShareWareVersion)
{
  if (startupOptions.mLevelToJumpTo)
  {
    auto [episode, level] = *startupOptions.mLevelToJumpTo;

    return std::make_unique<GameSessionMode>(
      data::GameSessionId{episode, level, data::Difficulty::Medium},
      context,
      startupOptions.mPlayerPosition);
  }
  else if (startupOptions.mSkipIntro)
  {
    return std::make_unique<MenuMode>(context);
  }

  if (!isShareWareVersion) {
    return std::make_unique<AntiPiracyScreenMode>(context);
  }

  return std::make_unique<IntroDemoLoopMode>(context, true);
}


std::optional<FpsLimiter> createLimiter(const data::GameOptions& options) {
  if (options.mEnableFpsLimit && !options.mEnableVsync) {
    return FpsLimiter{options.mMaxFps};
  } else {
    return std::nullopt;
  }
}


std::string effectiveGamePath(
  const StartupOptions& options,
  const UserProfile& profile
) {
  using namespace std::string_literals;

  if (!options.mGamePath.empty()) {
    return options.mGamePath;
  }

  return profile.mGamePath ? profile.mGamePath->u8string() + "/"s : ""s;
}

}


void gameMain(const StartupOptions& options) {
#ifdef _WIN32
  SDL_setenv("SDL_AUDIODRIVER", "directsound", true);
  SetProcessDPIAware();
#endif

  sdl_utils::check(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER));
  auto sdlGuard = defer([]() { SDL_Quit(); });

  sdl_utils::check(SDL_GL_LoadLibrary(nullptr));
  setGLAttributes();

  auto userProfile = loadOrCreateUserProfile(options.mGamePath);
  if (!options.mGamePath.empty())
  {
    userProfile.mGamePath = options.mGamePath;
  }

  auto pWindow = createWindow(userProfile.mOptions);
  SDL_GLContext pGlContext =
    sdl_utils::check(SDL_GL_CreateContext(pWindow.get()));
  auto glGuard = defer([pGlContext]() { SDL_GL_DeleteContext(pGlContext); });

  renderer::loadGlFunctions();

  SDL_DisableScreenSaver();
  SDL_ShowCursor(SDL_DISABLE);

  ui::imgui_integration::init(
    pWindow.get(), pGlContext, createOrGetPreferencesPath());
  auto imGuiGuard = defer([]() { ui::imgui_integration::shutdown(); });

  Game game(options, &userProfile, pWindow.get());
  game.run(options);
}


FpsLimiter::FpsLimiter(const int targetFps)
  : mLastTime(std::chrono::high_resolution_clock::now())
  , mTargetFrameTime(1.0 / targetFps)
{
}


void FpsLimiter::updateAndWait() {
  using namespace std::chrono;

  const auto now = high_resolution_clock::now();
  const auto delta = duration<double>(now - mLastTime).count();
  mLastTime = now;

  mError += mTargetFrameTime - delta;

  const auto timeToWaitFor = mTargetFrameTime + mError;
  if (timeToWaitFor > 0.0) {
    // We use SDL_Delay instead of std::this_thread::sleep_for, because the
    // former is more accurate on some platforms.
    SDL_Delay(static_cast<Uint32>(timeToWaitFor * 1000.0));
  }
}


Game::Game(
  const StartupOptions& startupOptions,
  UserProfile* pUserProfile,
  SDL_Window* pWindow
)
  : mpWindow(pWindow)
  , mRenderer(pWindow)
  , mResources(effectiveGamePath(startupOptions, *pUserProfile))
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
  , mpUserProfile(pUserProfile)
  , mScriptRunner(&mResources, &mRenderer, &mpUserProfile->mSaveSlots, this)
  , mAllScripts(loadScripts(mResources))
  , mUiSpriteSheet(
      renderer::OwningTexture{
        &mRenderer, mResources.loadTiledFullscreenImage("STATUS.MNI")},
      &mRenderer)
  , mTextRenderer(&mUiSpriteSheet, &mRenderer, mResources)
{
}


void Game::run(const StartupOptions& startupOptions) {
  mRenderer.clear();
  mRenderer.swapBuffers();

  data::forEachSoundId([this](const auto id) {
    mSoundsById.emplace_back(mSoundSystem.addSound(mResources.loadSound(id)));
  });

  applyChangedOptions();

  mpCurrentGameMode = wrapWithInitialFadeIn(createInitialGameMode(
    makeModeContext(), startupOptions, mIsShareWareVersion));

  //TODO : support multiple controllers
  for (std::uint8_t i = 0; i < SDL_NumJoysticks(); ++i) {
    if (SDL_IsGameController(i)) {
      mpGameController =
        sdl_utils::Ptr<SDL_GameController>{SDL_GameControllerOpen(i)};
      break;
    }
  }

  mainLoop();

  // exiting the game
  mpUserProfile->saveToDisk();
}


void Game::mainLoop() {
  using namespace std::chrono;

  std::vector<SDL_Event> eventQueue;
  mLastTime = high_resolution_clock::now();

  for (;;) {
    const auto startOfFrame = high_resolution_clock::now();
    const auto elapsed =
      duration<entityx::TimeDelta>(startOfFrame - mLastTime).count();
    mLastTime = startOfFrame;

    pumpEvents(eventQueue);
    if (!mIsRunning) {
      break;
    }

    ui::imgui_integration::beginFrame(mpWindow);
    ImGui::SetMouseCursor(ImGuiMouseCursor_None);

    {
      RenderTargetBinder bindRenderTarget(mRenderTarget, &mRenderer);
      mRenderer.clear();

      auto saved = setupSimpleUpscaling(&mRenderer);

      auto pMaybeNextMode =
        mpCurrentGameMode->updateAndRender(elapsed, eventQueue);
      eventQueue.clear();

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
        const auto afterRender = high_resolution_clock::now();
        const auto innerRenderTime =
          duration<engine::TimeDelta>(afterRender - startOfFrame).count();
        mFpsDisplay.updateAndRender(elapsed, innerRenderTime);
      }
    }

    ui::imgui_integration::endFrame();
    swapBuffers();

    applyChangedOptions();
  }
}


void Game::pumpEvents(std::vector<SDL_Event>& eventQueue) {
  SDL_Event event;
  while (mIsMinimized && SDL_WaitEvent(&event)) {
    if (!handleEvent(event)) {
      eventQueue.push_back(event);
    }
  }

  while (SDL_PollEvent(&event)) {
    if (!handleEvent(event)) {
      eventQueue.push_back(event);
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

  auto startTime = high_resolution_clock::now();

  while (mIsRunning) {
    const auto now = high_resolution_clock::now();
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
  mLastTime = high_resolution_clock::now();
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
    SDL_SetWindowFullscreen(
      mpWindow, flagsForWindowMode(currentOptions.mWindowMode));

    if (currentOptions.mWindowMode == data::WindowMode::Windowed) {
      SDL_SetWindowSize(
        mpWindow, currentOptions.mWindowWidth, currentOptions.mWindowHeight);
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

  if (
    currentOptions.mMusicVolume != mPreviousOptions.mMusicVolume ||
    currentOptions.mMusicOn != mPreviousOptions.mMusicOn
  ) {
    const auto newVolume = currentOptions.mMusicOn
      ? currentOptions.mMusicVolume
      : 0.0f;
    mSoundSystem.setMusicVolume(newVolume);
  }

  if (
    currentOptions.mSoundVolume != mPreviousOptions.mSoundVolume ||
    currentOptions.mSoundOn != mPreviousOptions.mSoundOn
  ) {
    const auto newVolume = currentOptions.mSoundOn
      ? currentOptions.mSoundVolume
      : 0.0f;
    mSoundSystem.setSoundVolume(newVolume);
  }

  mPreviousOptions = mpUserProfile->mOptions;
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
  mSoundSystem.playSong(mResources.loadMusic(name));
}


void Game::stopMusic() {
  mSoundSystem.stopMusic();
}


void Game::scheduleGameQuit() {
  mIsRunning = false;
}

}
