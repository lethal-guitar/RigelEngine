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

#pragma once

#include "base/warnings.hpp"
#include "common/command_line_options.hpp"

RIGEL_DISABLE_WARNINGS
#include <SDL_video.h>
RIGEL_RESTORE_WARNINGS

#include <memory>


namespace rigel {

class Game;
class UserProfile;

// Regular game main


/** Initialize platform, create game, run main loop
 *
 * This function is meant for environments where Rigel can own the main loop.
 * It keeps running until the user quits the game.
 *
 * Running the game via this function implements the game path browser UI for
 * choosing a Duke Nukem II installation on first launch, and also implements
 * restarting the game in case the user switches to a different game path from
 * the options menu.
 */
void gameMain(const CommandLineOptions& options);


// Lower-level API for integration into callback-based frameworks

struct GameDeleter {
  void operator()(Game* pGame);
};

/** Create a Game
 *
 * This creates a Game for use with runOneFrame. It requires the user profile
 * to have a valid game path set, otherwise it will fail with an exception.
 * It also needs a valid OpenGL window to already be open, and ImGui must have
 * been initialized.
 *
 * This is meant for environments where Rigel cannot own the main loop, like
 * when running in a browser via Webassembly.
 *
 * TODO: Move the platform initialization code to a place where it can be
 * reused more easily, independently of gameMain().
 */
std::unique_ptr<Game, GameDeleter> createGame(
  SDL_Window* pWindow,
  UserProfile* pUserProfile,
  const CommandLineOptions& commandLineOptions);

/** Run one frame of the game
 *
 * Use this with the Game object returned from createGame. Should be called
 * once every frame. This is meant for callback-based environments, where the
 * main loop is outside of Rigel's control, like Webassembly.
 *
 * At the moment, this does not allow for game restarts as required when
 * choosing a different game path. But since createGame() doesn't implement
 * the game path browser and requires a valid game path to be pre-configured,
 * this seems like an ok limitation.
 */
void runOneFrame(Game* pGame);

}
