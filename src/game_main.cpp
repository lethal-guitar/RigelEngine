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

#include "base/defer.hpp"
#include "base/math_tools.hpp"
#include "data/duke_script.hpp"
#include "data/game_traits.hpp"
#include "engine/timing.hpp"
#include "loader/duke_script_loader.hpp"
#include "renderer/opengl.hpp"
#include "renderer/upscaling_utils.hpp"
#include "sdl_utils/error.hpp"
#include "ui/game_path_browser.hpp"
#include "ui/imgui_integration.hpp"

#include "anti_piracy_screen_mode.hpp"
#include "game_session_mode.hpp"
#include "intro_demo_loop_mode.hpp"
#include "menu_mode.hpp"
#include "platform.hpp"

RIGEL_DISABLE_WARNINGS
#include <imgui.h>
#include <SDL.h>
RIGEL_RESTORE_WARNINGS

#ifdef _WIN32
#include <Windows.h>
#endif

#include <cassert>
#include <filesystem>


namespace rigel {

using namespace engine;
using namespace sdl_utils;

using RenderTargetBinder = renderer::RenderTargetTexture::Binder;

namespace {

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
  const bool isShareWareVersion,
  const bool isFirstLaunch)
{
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

  if (!isShareWareVersion) {
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


void setupForFirstLaunch(
  SDL_Window* pWindow,
  UserProfile& userProfile,
  const std::string& commandLineGamePath
) {
  namespace fs = std::filesystem;

  auto gamePath = fs::path{};

  // Case 1: A path is given on the command line on first launch. Use that.
  if (!commandLineGamePath.empty()) {
    gamePath = fs::u8path(commandLineGamePath);
  }

  const auto dataFilePath = fs::u8path("NUKEM2.CMP");

  // Case 2: The current working directory is set to a Duke Nukem II
  // installation, most likely because the RigelEngine executable has been
  // copied there. Use the current working directory as game path.
  if (gamePath.empty()) {
    const auto currentWorkingDir = fs::current_path();
    if (
      commandLineGamePath.empty() &&
      fs::exists(currentWorkingDir / dataFilePath)
    ) {
      gamePath = currentWorkingDir;
    }
  }

  // Case 3: Neither case 1 nor case 2 apply. Show a folder browser to let
  // the user select their Duke Nukem II installation.
  if (gamePath.empty()) {
    gamePath = ui::runFolderBrowser(pWindow);
  }

  // If we still don't have a game path, stop here.
  if (gamePath.empty()) {
    throw std::runtime_error(
R"(No game path given. RigelEngine needs the original Duke Nukem II data files in order to function.
You can download the Shareware version for free, see
https://github.com/lethal-guitar/RigelEngine/blob/master/README.md#acquiring-the-game-data
for more info.)");
  }

  // Make sure there is a data file at the game path.
  if (!fs::exists(gamePath / dataFilePath)) {
    throw std::runtime_error("No game data (NUKEM2.CMP file) found in game path");
  }

  // Import original game's profile data, if our profile is still 'empty'
  if (!userProfile.hasProgressData()) {
    importOriginalGameProfileData(userProfile, gamePath.u8string() + "/");
  }

  // Finally, persist the chosen game path in the user profile for the next
  // launch.
  userProfile.mGamePath = fs::canonical(gamePath);
  userProfile.saveToDisk();
}


void initAndRunGame(
  SDL_Window* pWindow,
  UserProfile& userProfile,
  const CommandLineOptions& commandLineOptions
) {
  auto run = [&](const CommandLineOptions& options, const bool isFirstLaunch) {
    Game game(options, &userProfile, pWindow, isFirstLaunch);

    for (;;) {
      auto maybeStopReason = game.runOneFrame();
      if (maybeStopReason) {
        return *maybeStopReason;
      }
    }
  };

  const auto needsProfileSetup = !userProfile.mGamePath.has_value();
  if (needsProfileSetup) {
    setupForFirstLaunch(pWindow, userProfile, commandLineOptions.mGamePath);
  }

  auto result = run(
    commandLineOptions,
    needsProfileSetup && !userProfile.hasProgressData());

  // Some game option changes (like choosing a new game path) require
  // restarting the game to make the change effective. If the first game run
  // ended with a result of RestartNeeded, launch a new game, but start from
  // the main menu and discard most command line options.
  if (result == Game::StopReason::RestartNeeded) {
    auto optionsForRestartedGame = CommandLineOptions{};
    optionsForRestartedGame.mSkipIntro = true;
    optionsForRestartedGame.mDebugModeEnabled =
      commandLineOptions.mDebugModeEnabled;

    while (result == Game::StopReason::RestartNeeded) {
      result = run(optionsForRestartedGame, false);
    }
  }

  // We're exiting, save the user profile
  userProfile.saveToDisk();
}

}


void gameMain(const CommandLineOptions& options) {
  using base::defer;

#ifdef _WIN32
  SDL_setenv("SDL_AUDIODRIVER", "directsound", true);
  SetProcessDPIAware();
#endif

  sdl_utils::check(SDL_Init(
    SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER));
  auto sdlGuard = defer([]() { SDL_Quit(); });

  sdl_utils::check(SDL_GL_LoadLibrary(nullptr));
  platform::setGLAttributes();

  auto userProfile = loadOrCreateUserProfile();
  auto pWindow = platform::createWindow(userProfile.mOptions);
  SDL_GLContext pGlContext =
    sdl_utils::check(SDL_GL_CreateContext(pWindow.get()));
  auto glGuard = defer([pGlContext]() { SDL_GL_DeleteContext(pGlContext); });

  renderer::loadGlFunctions();

  SDL_DisableScreenSaver();
  SDL_ShowCursor(SDL_DISABLE);

  ui::imgui_integration::init(
    pWindow.get(), pGlContext, createOrGetPreferencesPath());
  auto imGuiGuard = defer([]() { ui::imgui_integration::shutdown(); });

  try {
    initAndRunGame(pWindow.get(), userProfile, options);
  } catch (const std::exception& error) {
    ui::showErrorMessage(pWindow.get(), error.what());
  }
}


void GameDeleter::operator()(Game* pGame) {
  delete pGame;
}


std::unique_ptr<Game, GameDeleter> createGame(
  SDL_Window* pWindow,
  UserProfile* pUserProfile,
  const CommandLineOptions& commandLineOptions
) {
  return std::unique_ptr<Game, GameDeleter>(
    new Game{commandLineOptions, pUserProfile, pWindow, false});
}


void runOneFrame(Game* pGame) {
  pGame->runOneFrame();
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

  mLastTime = std::chrono::high_resolution_clock::now();
}


auto Game::runOneFrame() -> std::optional<StopReason> {
  using namespace std::chrono;
  using base::defer;

  const auto startOfFrame = high_resolution_clock::now();
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
