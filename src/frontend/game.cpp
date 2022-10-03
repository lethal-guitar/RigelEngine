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

#include "assets/duke_script_loader.hpp"
#include "assets/png_image.hpp"
#include "base/defer.hpp"
#include "base/math_utils.hpp"
#include "data/duke_script.hpp"
#include "data/game_traits.hpp"
#include "engine/timing.hpp"
#include "game_logic/demo_player.hpp"
#include "renderer/upscaling.hpp"
#include "ui/imgui_integration.hpp"

#include "anti_piracy_screen_mode.hpp"
#include "game_session_mode.hpp"
#include "intro_demo_loop_mode.hpp"
#include "menu_mode.hpp"
#include "platform.hpp"

RIGEL_DISABLE_WARNINGS
#include <imgui.h>
#include <loguru.hpp>
RIGEL_RESTORE_WARNINGS

#include <ctime>


namespace rigel
{

using namespace engine;

namespace
{

auto wrapWithInitialFadeIn(std::unique_ptr<GameMode> mode)
{
  class InitialFadeInWrapper : public GameMode
  {
  public:
    explicit InitialFadeInWrapper(std::unique_ptr<GameMode> pModeToSwitchTo)
      : mpModeToSwitchTo(std::move(pModeToSwitchTo))
    {
    }

    std::unique_ptr<GameMode>
      updateAndRender(engine::TimeDelta, const std::vector<SDL_Event>&) override
    {
      return std::move(mpModeToSwitchTo);
    }

  private:
    std::unique_ptr<GameMode> mpModeToSwitchTo;
  };

  return std::make_unique<InitialFadeInWrapper>(std::move(mode));
}


auto loadScripts(const assets::ResourceLoader& resources)
{
  auto allScripts = resources.loadScriptBundle("TEXT.MNI");
  const auto optionsScripts = resources.loadScriptBundle("OPTIONS.MNI");
  const auto orderInfoScripts = resources.loadScriptBundle("ORDERTXT.MNI");

  allScripts.insert(std::begin(optionsScripts), std::end(optionsScripts));
  allScripts.insert(std::begin(orderInfoScripts), std::end(orderInfoScripts));

  return allScripts;
}


std::unique_ptr<GameMode> createInitialGameMode(
  GameMode::Context context,
  const CommandLineOptions& commandLineOptions,
  const bool isSharewareVersion,
  const bool isFirstLaunch)
{
  class DemoTestMode : public GameMode
  {
  public:
    explicit DemoTestMode(Context context)
      : mDemoPlayer(context)
      , mpServiceProvider(context.mpServiceProvider)
    {
    }

    std::unique_ptr<GameMode> updateAndRender(
      engine::TimeDelta dt,
      const std::vector<SDL_Event>&) override
    {
      mDemoPlayer.updateAndRender(dt);

      if (mDemoPlayer.isFinished())
      {
        mpServiceProvider->scheduleGameQuit();
      }

      return nullptr;
    }

  private:
    game_logic::DemoPlayer mDemoPlayer;
    IGameServiceProvider* mpServiceProvider;
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
  else if (commandLineOptions.mPlayDemo)
  {
    return std::make_unique<DemoTestMode>(context);
  }

  if (!isSharewareVersion)
  {
    return std::make_unique<AntiPiracyScreenMode>(context, isFirstLaunch);
  }

  return std::make_unique<IntroDemoLoopMode>(
    context,
    isFirstLaunch ? IntroDemoLoopMode::Type::AtFirstLaunch
                  : IntroDemoLoopMode::Type::DuringGameStart);
}


std::optional<renderer::FpsLimiter>
  createLimiter(const data::GameOptions& options)
{
  if (options.mEnableFpsLimit && !options.mEnableVsync)
  {
    return renderer::FpsLimiter{options.mMaxFps};
  }
  else
  {
    return std::nullopt;
  }
}


std::string makeScreenshotFilename()
{
  using namespace std::literals;

  const auto time =
    std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  const auto pLocalTime = std::localtime(&time);

  std::array<char, 32> dateTimeBuffer;
  dateTimeBuffer.fill(0);

  std::strftime(
    dateTimeBuffer.data(),
    dateTimeBuffer.size(),
    "%Y-%m-%d_%H%M%S",
    pLocalTime);

  return "RigelEngine_"s + dateTimeBuffer.data() + ".png";
}

} // namespace


std::filesystem::path effectiveGamePath(
  const CommandLineOptions& options,
  const UserProfile& profile)
{
  using namespace std::string_literals;

  if (!options.mGamePath.empty())
  {
    return std::filesystem::u8path(options.mGamePath);
  }

  return profile.mGamePath ? *profile.mGamePath : std::filesystem::path{};
}


Game::Game(
  const CommandLineOptions& commandLineOptions,
  UserProfile* pUserProfile,
  SDL_Window* pWindow,
  const bool isFirstLaunch)
  : mpWindow(pWindow)
  , mRenderer(pWindow)
  , mResources(
      effectiveGamePath(commandLineOptions, *pUserProfile),
      pUserProfile->mOptions.mEnableTopLevelMods,
      pUserProfile->mModLibrary.enabledModPaths())
  , mpSoundSystem([&]() -> std::unique_ptr<audio::SoundSystem> {
    if (commandLineOptions.mDisableAudio)
    {
      return nullptr;
    }

    std::unique_ptr<audio::SoundSystem> pResult;
    try
    {
      pResult = std::make_unique<audio::SoundSystem>(
        &mResources,
        pUserProfile->mOptions.mSoundStyle,
        pUserProfile->mOptions.mAdlibPlaybackType);
    }
    catch (const std::exception& ex)
    {
      LOG_F(WARNING, "Failed to initialize audio: %s", ex.what());
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
  , mUpscalingBuffer(&mRenderer, pUserProfile->mOptions)
  , mIsRunning(true)
  , mIsMinimized(false)
  , mCommandLineOptions(commandLineOptions)
  , mpUserProfile(pUserProfile)
  , mPreviousWindowSize(mRenderer.windowSize())
  , mWidescreenModeWasActive(
      pUserProfile->mOptions.mWidescreenModeOn &&
      renderer::canUseWidescreenMode(&mRenderer))
  , mScriptRunner(&mResources, &mRenderer, &mpUserProfile->mSaveSlots, this)
  , mAllScripts(loadScripts(mResources))
  , mUiSpriteSheet(
      // Explicitly specify the palette here to avoid loading any replacement
      // status.png file (since that is meant only for in-game, for now)
      renderer::Texture{
        &mRenderer,
        mResources.loadUiSpriteSheet(data::GameTraits::INGAME_PALETTE)},
      &mRenderer)
  , mSpriteFactory(&mRenderer, &mResources)
  , mTextRenderer(&mUiSpriteSheet, &mRenderer, mResources)
{
  LOG_F(INFO, "Successfully loaded all resources");
  LOG_F(
    INFO,
    "Running %s version at %s",
    mIsShareWareVersion ? "Shareware" : "Registered",
    effectiveGamePath(commandLineOptions, *pUserProfile).u8string().c_str());

  mCommandLineOptions.mSkipIntro |= mpUserProfile->mOptions.mSkipIntro;

  applyChangedOptions();

  mpCurrentGameMode = wrapWithInitialFadeIn(createInitialGameMode(
    makeModeContext(),
    mCommandLineOptions,
    mIsShareWareVersion,
    isFirstLaunch));

  LOG_F(INFO, "Game started");

  mLastTime = base::Clock::now();
}


auto Game::runOneFrame() -> std::optional<StopReason>
{
  using namespace std::chrono;
  using base::defer;

  const auto startOfFrame = base::Clock::now();
  const auto elapsed =
    duration<entityx::TimeDelta>(startOfFrame - mLastTime).count();
  mLastTime = startOfFrame;

  pumpEvents();
  if (!mIsRunning)
  {
    stopMusic();
    return StopReason::GameEnded;
  }

  {
    ui::imgui_integration::beginFrame(mpWindow);
    auto imGuiFrameGuard = defer([]() { ui::imgui_integration::endFrame(); });
    ImGui::SetMouseCursor(ImGuiMouseCursor_None);

    updateAndRender(elapsed);
    mEventQueue.clear();
  }

  if (mScreenshotRequested)
  {
    takeScreenshot();
    mScreenshotRequested = false;
  }

  swapBuffers();

  const auto changedOptionsRequireRestart = applyChangedOptions();

  if (!mGamePathToSwitchTo.empty())
  {
    mpUserProfile->mGamePath = mGamePathToSwitchTo;
    mpUserProfile->saveToDisk();
    return StopReason::RestartNeeded;
  }

  if (
    changedOptionsRequireRestart ||
    mpUserProfile->mModLibrary.fetchAndClearSelectionChangedFlag())
  {
    return StopReason::RestartNeeded;
  }

  return {};
}


void Game::pumpEvents()
{
  SDL_Event event;
  while (mIsMinimized && SDL_WaitEvent(&event))
  {
    if (!handleEvent(event))
    {
      mEventQueue.push_back(event);
    }
  }

  while (SDL_PollEvent(&event))
  {
    if (!handleEvent(event))
    {
      mEventQueue.push_back(event);
    }
  }
}


void Game::updateAndRender(const entityx::TimeDelta elapsed)
{
  mCurrentFrameIsWidescreen = false;

  auto pMaybeNextMode = std::invoke([&]() {
    auto saved = mUpscalingBuffer.bindAndClear(
      mpUserProfile->mOptions.mPerElementUpscalingEnabled);
    return mpCurrentGameMode->updateAndRender(elapsed, mEventQueue);
  });

  if (pMaybeNextMode)
  {
    fadeOutScreen();

    setPerElementUpscalingEnabled(pMaybeNextMode->needsPerElementUpscaling());
    mpCurrentGameMode = std::move(pMaybeNextMode);

    {
      auto saved = mUpscalingBuffer.bindAndClear(
        mpUserProfile->mOptions.mPerElementUpscalingEnabled);
      mpCurrentGameMode->updateAndRender(0, {});
    }

    fadeInScreen();
  }

  mUpscalingBuffer.present(
    mCurrentFrameIsWidescreen,
    mpUserProfile->mOptions.mPerElementUpscalingEnabled);

  if (mpUserProfile->mOptions.mShowFpsCounter)
  {
    mFpsDisplay.updateAndRender(elapsed);
  }
}


GameMode::Context Game::makeModeContext()
{
  return {
    &mResources,
    &mRenderer,
    this,
    &mScriptRunner,
    &mAllScripts,
    &mTextRenderer,
    &mUiSpriteSheet,
    &mSpriteFactory,
    mpUserProfile};
}


bool Game::handleEvent(const SDL_Event& event)
{
  if (ui::imgui_integration::handleEvent(event) && event.type != SDL_KEYUP)
  {
    return true;
  }

  auto& options = mpUserProfile->mOptions;
  switch (event.type)
  {
    case SDL_KEYUP:
      if (event.key.keysym.sym == SDLK_F6)
      {
        options.mShowFpsCounter = !options.mShowFpsCounter;
      }
      else if (event.key.keysym.sym == SDLK_F12)
      {
        mScreenshotRequested = true;
      }
      return false;

    case SDL_QUIT:
      mIsRunning = false;
      break;

    case SDL_WINDOWEVENT:
      switch (event.window.event)
      {
        case SDL_WINDOWEVENT_MINIMIZED:
          LOG_F(INFO, "Window minimized, pausing");
          mIsMinimized = true;
          break;

        case SDL_WINDOWEVENT_ENTER:
        case SDL_WINDOWEVENT_FOCUS_GAINED:
        case SDL_WINDOWEVENT_MAXIMIZED:
        case SDL_WINDOWEVENT_RESTORED:
          LOG_IF_F(INFO, mIsMinimized, "Window restored, unpausing");
          mIsMinimized = false;
          break;

        case SDL_WINDOWEVENT_SIZE_CHANGED:
          if (options.effectiveWindowMode() == data::WindowMode::Windowed)
          {
            options.mWindowWidth = event.window.data1;
            options.mWindowHeight = event.window.data2;
          }
          break;

        case SDL_WINDOWEVENT_MOVED:
          if (options.effectiveWindowMode() == data::WindowMode::Windowed)
          {
            options.mWindowPosX = event.window.data1;
            options.mWindowPosY = event.window.data2;
          }
          break;
      }
      break;

    case SDL_JOYDEVICEADDED:
    case SDL_JOYDEVICEREMOVED:
      enumerateGameControllers();
      break;

    default:
      return false;
  }

  return true;
}


void Game::performScreenFadeBlocking(const FadeType type)
{
#ifdef __EMSCRIPTEN__
  // TODO: Implement screen fades for the Emscripten version.
  // This is not so easy because we can't simply do a loop that renders
  // multiple frames when running in the browser, as it just blocks the
  // browser's main thread and the intermediate (faded) frames are not
  // shown until the current requestAnimationFrame() callback returns.
  // So we'd need to either find a way to suspend and then resume C++ code
  // execution during the fade, or rewrite all client code to be stateful
  // instead of relying on a blocking fade() function.
  mUpscalingBuffer.setAlphaMod(type == FadeType::In ? 255 : 0);
#else
  using namespace std::chrono;

  auto saved = renderer::saveState(&mRenderer);
  mRenderer.resetState();

  auto startTime = base::Clock::now();

  while (mIsRunning)
  {
    const auto now = base::Clock::now();
    const auto elapsedTime = duration<double>(now - startTime).count();
    const auto fastTicksElapsed = engine::timeToFastTicks(elapsedTime);
    const auto fadeFactor =
      std::clamp((fastTicksElapsed / 4.0) / 16.0, 0.0, 1.0);
    const auto alpha = type == FadeType::In ? fadeFactor : 1.0 - fadeFactor;
    const auto alphaMod = base::roundTo<std::uint8_t>(255.0 * alpha);

    mUpscalingBuffer.setAlphaMod(alphaMod);
    mUpscalingBuffer.present(
      mCurrentFrameIsWidescreen,
      mpUserProfile->mOptions.mPerElementUpscalingEnabled);
    swapBuffers();

    if (fadeFactor >= 1.0)
    {
      break;
    }
  }

  // Pretend that the fade didn't take any time
  mLastTime = base::Clock::now();
#endif
}


void Game::swapBuffers()
{
  mRenderer.swapBuffers();

  if (mFpsLimiter)
  {
    mFpsLimiter->updateAndWait();
  }
}


bool Game::applyChangedOptions()
{
  const auto& currentOptions = mpUserProfile->mOptions;

  if (
    currentOptions.effectiveWindowMode() !=
    mPreviousOptions.effectiveWindowMode())
  {
    LOG_F(
      INFO,
      "Changing window mode to %s",
      data::windowModeName(currentOptions.effectiveWindowMode()));
    const auto result = SDL_SetWindowFullscreen(
      mpWindow,
      platform::flagsForWindowMode(currentOptions.effectiveWindowMode()));

    if (result != 0)
    {
      LOG_F(WARNING, "Failed to set window mode: %s", SDL_GetError());
      mpUserProfile->mOptions.mWindowMode = mPreviousOptions.mWindowMode;
    }
    else
    {
      if (currentOptions.effectiveWindowMode() == data::WindowMode::Windowed)
      {
        if (currentOptions.mWindowCoordsValid)
        {
          SDL_SetWindowSize(
            mpWindow,
            currentOptions.mWindowWidth,
            currentOptions.mWindowHeight);
        }
        else
        {
          int width = 0;
          int height = 0;
          SDL_GetWindowSize(mpWindow, &width, &height);

          SDL_SetWindowSize(
            mpWindow, base::round(width * 0.8f), base::round(height * 0.8f));
          SDL_SetWindowPosition(
            mpWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
          mpUserProfile->mOptions.mWindowCoordsValid = true;
        }
      }
    }
  }

  if (currentOptions.mEnableVsync != mPreviousOptions.mEnableVsync)
  {
    SDL_GL_SetSwapInterval(mpUserProfile->mOptions.mEnableVsync ? 1 : 0);
  }

  if (
    currentOptions.mEnableVsync != mPreviousOptions.mEnableVsync ||
    currentOptions.mEnableFpsLimit != mPreviousOptions.mEnableFpsLimit ||
    currentOptions.mMaxFps != mPreviousOptions.mMaxFps)
  {
    mFpsLimiter = createLimiter(currentOptions);
  }

  if (mpSoundSystem)
  {
    if (currentOptions.mSoundStyle != mPreviousOptions.mSoundStyle)
    {
      mpSoundSystem->setSoundStyle(currentOptions.mSoundStyle);
    }

    if (
      currentOptions.mAdlibPlaybackType != mPreviousOptions.mAdlibPlaybackType)
    {
      mpSoundSystem->setAdlibPlaybackType(currentOptions.mAdlibPlaybackType);
    }

    if (
      currentOptions.mMusicVolume != mPreviousOptions.mMusicVolume ||
      currentOptions.mMusicOn != mPreviousOptions.mMusicOn)
    {
      const auto newVolume =
        currentOptions.mMusicOn ? currentOptions.mMusicVolume : 0.0f;
      mpSoundSystem->setMusicVolume(newVolume);
    }

    if (
      currentOptions.mSoundVolume != mPreviousOptions.mSoundVolume ||
      currentOptions.mSoundOn != mPreviousOptions.mSoundOn)
    {
      const auto newVolume =
        currentOptions.mSoundOn ? currentOptions.mSoundVolume : 0.0f;
      mpSoundSystem->setSoundVolume(newVolume);
    }
  }

  const auto widescreenModeActive = currentOptions.mWidescreenModeOn &&
    renderer::canUseWidescreenMode(&mRenderer);
  if (
    widescreenModeActive != mWidescreenModeWasActive ||
    mPreviousWindowSize != mRenderer.windowSize() ||
    currentOptions.mUpscalingFilter != mPreviousOptions.mUpscalingFilter ||
    currentOptions.mAspectRatioCorrectionEnabled !=
      mPreviousOptions.mAspectRatioCorrectionEnabled)
  {
    mUpscalingBuffer.updateConfiguration(currentOptions);
  }

  const auto restartNeeded =
    currentOptions.mEnableTopLevelMods != mPreviousOptions.mEnableTopLevelMods;

  mPreviousOptions = mpUserProfile->mOptions;
  mWidescreenModeWasActive = widescreenModeActive;
  mPreviousWindowSize = mRenderer.windowSize();

  return restartNeeded;
}


void Game::enumerateGameControllers()
{
  LOG_SCOPE_FUNCTION(INFO);

  mGameControllerInfo.mGameControllers.clear();
  mGameControllerInfo.mUnrecognizedControllers.clear();

  auto addUnrecognized = [this](const int index) {
    const auto sdlGuid = SDL_JoystickGetDeviceGUID(index);

    std::string guid;
    guid.resize(33);
    SDL_JoystickGetGUIDString(sdlGuid, &guid[0], int(guid.size()));

    mGameControllerInfo.mUnrecognizedControllers.emplace_back(
      SDL_JoystickNameForIndex(index), guid);
    LOG_F(
      INFO,
      "Found game controller without mappings: %s with GUID %s",
      SDL_JoystickNameForIndex(index),
      guid.c_str());
  };

  for (std::uint8_t i = 0; i < SDL_NumJoysticks(); ++i)
  {
    if (SDL_IsGameController(i))
    {
      auto pController =
        sdl_utils::Ptr<SDL_GameController>{SDL_GameControllerOpen(i)};
      if (pController)
      {
        LOG_F(
          INFO,
          "Found game controller: %s",
          SDL_GameControllerName(pController.get()));
        mGameControllerInfo.mGameControllers.push_back(std::move(pController));
      }
      else
      {
        LOG_F(ERROR, "Failed to open game controller: %s", SDL_GetError());
      }
    }
    else
    {
      addUnrecognized(i);
    }
  }
}


void Game::takeScreenshot()
{
  namespace fs = std::filesystem;

  constexpr auto SCREENSHOTS_SUBDIR = "screenshots";

  const auto shot = mRenderer.grabCurrentFramebuffer();
  const auto filename = makeScreenshotFilename();

  auto saveShot = [&](const fs::path& path) {
    std::error_code ec;

    if (!fs::exists(path, ec) && !ec)
    {
      fs::create_directory(path, ec);
    }

    return assets::savePng((path / filename).u8string(), shot);
  };

  const auto gameDirScreenshotPath =
    effectiveGamePath(mCommandLineOptions, *mpUserProfile) / SCREENSHOTS_SUBDIR;

  // First, try the game dir.
  if (saveShot(gameDirScreenshotPath))
  {
    return;
  }

  // If the game dir is not writable, try the user profile dir.
  if (const auto maybePrefsDir = createOrGetPreferencesPath(); maybePrefsDir)
  {
    saveShot(*maybePrefsDir / SCREENSHOTS_SUBDIR);
  }
}


void Game::setPerElementUpscalingEnabled(bool enabled)
{
  if (enabled != mpUserProfile->mOptions.mPerElementUpscalingEnabled)
  {
    mpUserProfile->mOptions.mPerElementUpscalingEnabled = enabled;
    mUpscalingBuffer.updateConfiguration(mpUserProfile->mOptions);
  }
}

void Game::fadeOutScreen()
{
  if (mUpscalingBuffer.alphaMod() == 0)
  {
    // Already faded out
    return;
  }

  performScreenFadeBlocking(FadeType::Out);

  // Clear render canvas after a fade-out
  mUpscalingBuffer.clear();

  mCurrentFrameIsWidescreen = false;
}


void Game::fadeInScreen()
{
  if (mUpscalingBuffer.alphaMod() == 255)
  {
    // Already faded in
    return;
  }

  performScreenFadeBlocking(FadeType::In);
}


void Game::playSound(const data::SoundId id)
{
  if (mpSoundSystem)
  {
    mpSoundSystem->playSound(id);
  }
}


void Game::stopSound(const data::SoundId id)
{
  if (mpSoundSystem)
  {
    mpSoundSystem->stopSound(id);
  }
}


void Game::stopAllSounds()
{
  if (mpSoundSystem)
  {
    mpSoundSystem->stopAllSounds();
  }
}


void Game::playMusic(const std::string& name)
{
  if (mpSoundSystem)
  {
    mpSoundSystem->playSong(name);
  }
}


void Game::stopMusic()
{
  if (mpSoundSystem)
  {
    mpSoundSystem->stopMusic();
  }
}


void Game::scheduleGameQuit()
{
  mIsRunning = false;
}


void Game::switchGamePath(const std::filesystem::path& newGamePath)
{
  if (newGamePath != mpUserProfile->mGamePath)
  {
    mGamePathToSwitchTo = newGamePath;
  }
}

} // namespace rigel
