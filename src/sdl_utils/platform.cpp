/* Copyright (C) 2021, Nikolai Wuttke. All rights reserved.
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

#include "platform.hpp"

#include <SDL_video.h>

#include <string_view>


namespace rigel::sdl_utils
{

bool isRunningInDesktopEnvironment()
{
  const auto sdlVideoDriver = std::string_view(SDL_GetCurrentVideoDriver());

  // clang-format off
  return
    sdlVideoDriver == "cocoa" ||
    sdlVideoDriver == "wayland" ||
    sdlVideoDriver == "windows" ||
    sdlVideoDriver == "x11";
  // clang-format on
}

} // namespace rigel::sdl_utils
