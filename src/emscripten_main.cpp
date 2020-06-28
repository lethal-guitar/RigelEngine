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

RIGEL_DISABLE_WARNINGS
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
RIGEL_RESTORE_WARNINGS

#include <iostream>
#include <stdexcept>
#include <string>
#include "game_main.hpp"
#include "game_main.ipp"

RIGEL_DISABLE_WARNINGS
#include <imfilebrowser.h>
#include <imgui.h>
#include <SDL.h>
RIGEL_RESTORE_WARNINGS
#include <emscripten.h>

using namespace rigel;
using namespace rigel::misc;

// Emscripten only takes a single argument. Therefore, all the needed arguments are combined in this struct and sent
struct em_args
{
  Game &game;
  std::vector<SDL_Event> &eventQueue;
  bool &breakOut; // for breaking out of the loop
  em_args(Game &game_ref, std::vector<SDL_Event> &eventQueue_ref, bool &breakOut_ref) : game(game_ref), eventQueue(eventQueue_ref), breakOut(breakOut_ref) {}
};

void runOneFrameWrapper(void *data)
{
  em_args *gameArgs = static_cast<em_args *>(data);
  gameArgs->game.runOneFrame(gameArgs->eventQueue, gameArgs->breakOut);
}

int main()
{
  sdl_utils::check(SDL_Init(
      SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER));
  auto sdlGuard = defer([]() { SDL_Quit(); });

  sdl_utils::check(SDL_GL_LoadLibrary(nullptr));
  setGLAttributes();

  auto userProfile = loadOrCreateUserProfile();
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

  CommandLineOptions options;
  options.mGamePath = "/duke/";  // this is the path in virtual file system (provided by emscripten, defined in cmake) where resource data of Duke Nukem II will exist
  Game game{options, &userProfile, pWindow.get()};
  game.mLastTime = std::chrono::high_resolution_clock::now();
  std::vector<SDL_Event> eventQueue;
  bool breakOut = false;

  em_args gameArgs(game, eventQueue, breakOut);
  emscripten_set_main_loop_arg(runOneFrameWrapper, &gameArgs, 0, true);

  return 0;
}