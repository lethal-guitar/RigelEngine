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

#include "base/warnings.hpp"
#include "sdl_utils/error.hpp"
#include "sdl_utils/ptr.hpp"

#include "game.hpp"

RIGEL_DISABLE_WARNINGS
#include <boost/program_options.hpp>
#include <SDL.h>
#include <SDL_mixer.h>
RIGEL_RESTORE_WARNINGS

#include <iostream>
#include <stdexcept>
#include <string>


using namespace rigel;
using namespace rigel::sdl_utils;
using namespace std;

namespace po = boost::program_options;


namespace {

#ifdef __APPLE__
  const auto WINDOW_FLAGS = SDL_WINDOW_FULLSCREEN_DESKTOP;
#else
  const auto WINDOW_FLAGS = SDL_WINDOW_BORDERLESS;
#endif

const auto SOME_DUMMY_SIZE = 32;


struct SdlInitializer {
  SdlInitializer() {
    throwIfFailed([]() {
      return SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    });
  }
  ~SdlInitializer() {
    SDL_Quit();
  }

  SdlInitializer(const SdlInitializer&) = delete;
  SdlInitializer& operator=(const SdlInitializer&) = delete;
};


SDL_Window* createWindow() {
  SDL_DisplayMode displayMode;
  throwIfFailed([&displayMode]() {
    return SDL_GetDesktopDisplayMode(0, &displayMode);
  });

  return throwIfCreationFailed([&displayMode]() {
    return SDL_CreateWindow(
      "Rigel Engine",
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      displayMode.w,
      displayMode.h,
      WINDOW_FLAGS);
  });
}


SDL_Renderer* createRenderer(SDL_Window* pWindow) {
  return throwIfCreationFailed([pWindow]() {
    return SDL_CreateRenderer(
      pWindow,
      -1,
      SDL_RENDERER_ACCELERATED
      | SDL_RENDERER_PRESENTVSYNC
      | SDL_RENDERER_TARGETTEXTURE
    );
  });
}


void verifyRequiredRendererCapabilities(SDL_Renderer* pRenderer) {
  SDL_RendererInfo rendererInfo;
  throwIfFailed([&rendererInfo, pRenderer]() {
    return SDL_GetRendererInfo(pRenderer, &rendererInfo);
  });

  // Verify that render targets are supported
  if ((rendererInfo.flags & SDL_RENDERER_TARGETTEXTURE) == 0) {
    throw runtime_error("Renderer doesn't support render targets");
  }

  // Verify that textures can be created with the desired pixel format
  Ptr<SDL_Texture> pTestTexture(throwIfCreationFailed([pRenderer]() {
    return SDL_CreateTexture(
      pRenderer,
      SDL_PIXELFORMAT_ABGR8888,
      SDL_TEXTUREACCESS_STATIC,
      SOME_DUMMY_SIZE,
      SOME_DUMMY_SIZE);
  }));

  // Verify that textures support alpha blending
  throwIfFailed([&pTestTexture, pRenderer]() {
    return SDL_SetTextureBlendMode(pTestTexture.get(), SDL_BLENDMODE_BLEND);
  });

  // Verify that textures support alpha modulation
  throwIfFailed([&pTestTexture, pRenderer]() {
    return SDL_SetTextureAlphaMod(pTestTexture.get(), 125);
  });

  // Verify that textures support color modulation
  throwIfFailed([&pTestTexture, pRenderer]() {
    return SDL_SetTextureColorMod(pTestTexture.get(), 0, 255, 0);
  });
}


void showBanner() {
  cout <<
    "================================================================================\n"
    "                            Welcome to RIGEL ENGINE!\n"
    "\n"
    "  A modern reimplementation of the game Duke Nukem II, originally released in\n"
    "  1993 for MS-DOS by Apogee Software.\n"
    "\n"
    "You need the original game's data files in order to play, e.g. the freely\n"
    "available shareware version.\n"
    "\n"
    "Rigel Engine Copyright (C) 2016, Nikolai Wuttke.\n"
    "Rigel Engine comes with ABSOLUTELY NO WARRANTY. This is free software, and you\n"
    "are welcome to redistribute it under certain conditions.\n"
    "For details, see https://www.gnu.org/licenses/gpl-2.0.html\n"
    "================================================================================\n"
    "\n";
}


void initAndRunGame(const string& gamePath, const Game::Options& gameOptions) {
  SdlInitializer initializeSDL;
  Ptr<SDL_Window> pWindow(createWindow());
  Ptr<SDL_Renderer> pRenderer(createRenderer(pWindow.get()));

  verifyRequiredRendererCapabilities(pRenderer.get());

  // We don't care if screen saver disabling failed, it's not that important.
  // So no return value checking.
  SDL_DisableScreenSaver();

  Game game(gamePath, pRenderer.get());
  game.run(gameOptions);
}

}


int main(int argc, char** argv) {
  showBanner();

  string gamePath;
  Game::Options gameOptions;
  bool disableMusic = false;

  po::options_description optionsDescription("Options");
  optionsDescription.add_options()
    ("help,h", "Show command line help message")
    ("skip-intro,s",
     po::bool_switch(&gameOptions.mSkipIntro),
     "Skip intro movies/Apogee logo, go straight to main menu")
    ("play-level,l",
     po::value<string>(),
     "Directly jump to given map, skipping intro/menu etc.")
    ("no-music",
     po::bool_switch(&disableMusic),
     "Disable music playback")
    ("game-path",
     po::value<string>(&gamePath),
     "Path to original game's installation. Can also be given as positional "
     "argument.");

  po::positional_options_description positionalArgsDescription;
  positionalArgsDescription.add("game-path", -1);

  try
  {
    po::variables_map options;
    po::store(
      po::command_line_parser(argc, argv)
        .options(optionsDescription)
        .positional(positionalArgsDescription)
        .run(),
      options);
    po::notify(options);

    if (options.count("help")) {
      cout << optionsDescription << '\n';
      return 0;
    }

    if (disableMusic) {
      gameOptions.mEnableMusic = false;
    }

    if (options.count("play-level")) {
      const auto levelToPlay = options["play-level"].as<string>();
      if (levelToPlay.size() != 2) {
        throw invalid_argument("Invalid level name");
      }

      const auto episode = static_cast<int>(levelToPlay[0] - 'L');
      const auto level = static_cast<int>(levelToPlay[1] - '0') - 1;

      if (episode < 0 || episode >= 4 || level < 0 || level >= 8) {
        throw invalid_argument(string("Invalid level name: ") + levelToPlay);
      }

      gameOptions.mLevelToJumpTo = std::make_pair(episode, level);
    }

    if (!gamePath.empty() && gamePath.back() != '/') {
      gamePath += "/";
    }

    initAndRunGame(gamePath, gameOptions);
  }
  catch (const po::error& err)
  {
    cerr << "ERROR: " << err.what() << "\n\n";
    cerr << optionsDescription << '\n';
    return -1;
  }
  catch (const std::exception& ex)
  {
    cerr << "ERROR: " << ex.what() << '\n';
    return -2;
  }
  catch (...)
  {
    cerr << "UNKNOWN ERROR\n";
    return -3;
  }

  return 0;
}
