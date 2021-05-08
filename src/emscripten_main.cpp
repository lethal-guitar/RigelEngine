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

// Welcome to the RigelEngine code base! If you are looking for the place in
// the code where everything starts, that would be in main.cpp. This file contains the
// main() function entry point for running RigelEngine in webassembly using emscripten.
// During all non-webassembly compilations, this file is ignored.

#include "base/warnings.hpp"

#include "base/defer.hpp"
#include "common/user_profile.hpp"
#include "frontend/game.hpp"
#include "renderer/opengl.hpp"
#include "sdl_utils/error.hpp"
#include "ui/imgui_integration.hpp"

#include "platform.hpp"

RIGEL_DISABLE_WARNINGS
#include <emscripten.h>
#include <SDL.h>
RIGEL_RESTORE_WARNINGS

#include <iostream>

// This is needed for EM_ASM
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"


using namespace rigel;

namespace {

// This is the path in the virtual file system (provided by emscripten, defined in cmake)
// where Duke Nukem II game data can be found.
// A path to valid data has to be given to cmake via the WEBASSEMBLY_GAME_PATH argument,
// and will be included in the build output.
constexpr auto WASM_GAME_PATH = "/duke/";

void runOneFrameWrapper(void *pData) {
  auto pGame = static_cast<Game*>(pData);
  if (const auto result = pGame->runOneFrame();
      result && *result == Game::StopReason::GameEnded)
  {
// clang-format off
    EM_ASM(
      document.getElementById("canvas").style.display = "none";
      document.getElementById("thankyoubox").style.display = "block";
    );
// clang-format on

    emscripten_cancel_main_loop();
  }
}

}


int main()
{
  using base::defer;

  sdl_utils::check(SDL_Init(
    SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER));
  auto sdlGuard = defer([]() { SDL_Quit(); });

  sdl_utils::check(SDL_GL_LoadLibrary(nullptr));
  platform::setGLAttributes();

  auto userProfile = loadOrCreateUserProfile();
  userProfile.mOptions.mMusicVolume = 0.5f;
  userProfile.mOptions.mSoundVolume = 0.5f;

  auto pWindow = platform::createWindow(userProfile.mOptions);
  SDL_GLContext pGlContext =
    sdl_utils::check(SDL_GL_CreateContext(pWindow.get()));
  auto glGuard = defer([pGlContext]() { SDL_GL_DeleteContext(pGlContext); });

  renderer::loadGlFunctions();

  SDL_GL_SetSwapInterval(data::ENABLE_VSYNC_DEFAULT ? 1 : 0);
  SDL_DisableScreenSaver();
  SDL_ShowCursor(SDL_DISABLE);

  ui::imgui_integration::init(
    pWindow.get(), pGlContext, createOrGetPreferencesPath());
  auto imGuiGuard = defer([]() { ui::imgui_integration::shutdown(); });

  CommandLineOptions options;
  options.mGamePath = WASM_GAME_PATH;

  Game game(options, &userProfile, pWindow.get(), false);

// clang-format off
  EM_ASM(
    var loadingBox = document.getElementById("loadingbox");
    loadingBox.parentNode.removeChild(loadingBox);

    document.getElementById("welcomebox").style.display = "block";
  );
// clang-format on

  emscripten_set_main_loop_arg(runOneFrameWrapper, &game, 0, true);

  return 0;
}

#pragma clang diagnostic pop
