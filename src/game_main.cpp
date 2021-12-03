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

#include "base/defer.hpp"
#include "frontend/game.hpp"
#include "renderer/opengl.hpp"
#include "sdl_utils/error.hpp"
#include "ui/game_path_browser.hpp"
#include "ui/imgui_integration.hpp"
#include "ui/utils.hpp"

#include "platform.hpp"

RIGEL_DISABLE_WARNINGS
#include <SDL.h>
#include <imgui.h>
RIGEL_RESTORE_WARNINGS

#ifdef _WIN32
  #include <windows.h>
#endif

#include <filesystem>


namespace rigel
{

using namespace sdl_utils;

namespace
{

bool isValidGamePath(const std::filesystem::path& path)
{
  namespace fs = std::filesystem;

  std::error_code ec;
  return fs::exists(path / "NUKEM2.CMP", ec) && !ec;
}


void showLoadingScreen(SDL_Window* pWindow)
{
  glClear(GL_COLOR_BUFFER_BIT);
  ui::imgui_integration::beginFrame(pWindow);

  ui::drawLoadingScreenText();

  ui::imgui_integration::endFrame();
  SDL_GL_SwapWindow(pWindow);
}


void initAndRunGame(
  SDL_Window* pWindow,
  UserProfile& userProfile,
  const CommandLineOptions& commandLineOptions)
{
  auto run = [&](const CommandLineOptions& options) {
    showLoadingScreen(pWindow);
    Game game(options, &userProfile, pWindow);

    for (;;)
    {
      auto maybeStopReason = game.runOneFrame();
      if (maybeStopReason)
      {
        return *maybeStopReason;
      }
    }
  };

  auto result = run(commandLineOptions);

  // Some game option changes (like choosing a new game path) require
  // restarting the game to make the change effective. If the first game run
  // ended with a result of RestartNeeded, launch a new game, but start from
  // the main menu and discard most command line options.
  if (result == Game::StopReason::RestartNeeded)
  {
    auto optionsForRestartedGame = CommandLineOptions{};
    optionsForRestartedGame.mSkipIntro = true;
    optionsForRestartedGame.mDebugModeEnabled =
      commandLineOptions.mDebugModeEnabled;

    while (result == Game::StopReason::RestartNeeded)
    {
      result = run(optionsForRestartedGame);
    }
  }

  // We're exiting, save the user profile
  userProfile.saveToDisk();
}

} // namespace


int gameMain(const CommandLineOptions& options)
{
  using base::defer;

#ifdef _WIN32
  SetProcessDPIAware();
#endif

  sdl_utils::check(
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER));
  auto sdlGuard = defer([]() { SDL_Quit(); });

  SDL_version version;
  SDL_GetVersion(&version);

  if (version.patch < 10)
  {
    if (const auto pMappingsFile = SDL_getenv("SDL_GAMECONTROLLERCONFIG_FILE"))
    {
      SDL_GameControllerAddMappingsFromFile(pMappingsFile);
    }
  }

  sdl_utils::check(SDL_GL_LoadLibrary(nullptr));
  platform::setGLAttributes();

  auto userProfile = loadOrCreateUserProfile();
  auto pWindow = platform::createWindow(userProfile.mOptions);
  SDL_GLContext pGlContext =
    sdl_utils::check(SDL_GL_CreateContext(pWindow.get()));
  auto glGuard = defer([pGlContext]() { SDL_GL_DeleteContext(pGlContext); });

  renderer::loadGlFunctions();

  // On some platforms, an initial swap is necessary in order for the next
  // frame (in our case, the loading screen) to show up on screen.
  SDL_GL_SetSwapInterval(data::ENABLE_VSYNC_DEFAULT ? 1 : 0);
  SDL_GL_SwapWindow(pWindow.get());

  SDL_DisableScreenSaver();
  SDL_ShowCursor(SDL_DISABLE);

  ui::imgui_integration::init(
    pWindow.get(), pGlContext, createOrGetPreferencesPath());
  auto imGuiGuard = defer([]() { ui::imgui_integration::shutdown(); });

  try
  {
    initAndRunGame(pWindow.get(), userProfile, options);
  }
  catch (const std::exception& error)
  {
    ui::showErrorMessage(pWindow.get(), error.what());
    return -2;
  }

  return 0;
}

} // namespace rigel
