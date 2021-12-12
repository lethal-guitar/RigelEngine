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

#pragma once

#include "base/spatial_types.hpp"
#include "data/game_session_data.hpp"

#include <optional>
#include <string>
#include <utility>


namespace rigel
{

/** Settings that can be changed via command-line arguments/switches
 *
 * This represents the parsed representation of all command line arguments
 * (argc/argv). The parsing happens in main.cpp
 * For documentation on the individual members, refer to the parser code over
 * there.
 *
 * If you add something here, you'll want to extend the code in main.cpp
 * as well!
 */
struct CommandLineOptions
{
  std::string mGamePath;
  std::optional<data::GameSessionId> mLevelToJumpTo;
  bool mSkipIntro = false;
  bool mDebugModeEnabled = true;
  bool mPlayDemo = false;
  std::optional<base::Vec2> mPlayerPosition;
};

} // namespace rigel
