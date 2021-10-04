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

#include "common/command_line_options.hpp"


namespace rigel
{

/** Initialize platform, create game, run main loop. Returns exit code
 *
 * This function is meant for environments where Rigel can own the main loop.
 * It keeps running until the user quits the game.
 *
 * Running the game via this function implements the game path browser UI for
 * choosing a Duke Nukem II installation on first launch, and also implements
 * restarting the game in case the user switches to a different game path from
 * the options menu.
 */
int gameMain(const CommandLineOptions& options);

} // namespace rigel
