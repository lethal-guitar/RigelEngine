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


namespace rigel {

struct CommandLineOptions {
  std::string mGamePath;
  std::optional<data::GameSessionId> mLevelToJumpTo;
  bool mSkipIntro = false;
  bool mDebugModeEnabled = false;
  std::optional<base::Vector> mPlayerPosition;
};

}
