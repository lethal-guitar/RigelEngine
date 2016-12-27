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

#include "base/spatial_types.hpp"
#include "base/warnings.hpp"

RIGEL_DISABLE_WARNINGS
#include <SDL_render.h>
RIGEL_RESTORE_WARNINGS

#include <cstdint>


namespace rigel { namespace sdl_utils {

template<typename ValueT>
SDL_Rect toSdlRect(const base::Rect<ValueT>& rect) {
  return {
    int(rect.topLeft.x), int(rect.topLeft.y),
    int(rect.size.width), int(rect.size.height)};
}


template<typename ValueT>
void drawRectangle(
  SDL_Renderer* pRenderer,
  const base::Rect<ValueT>& rect,
  const std::uint8_t red,
  const std::uint8_t green,
  const std::uint8_t blue,
  const std::uint8_t alpha = 255
) {
  const auto sdlRect = toSdlRect(rect);
  SDL_SetRenderDrawColor(pRenderer, red, green, blue, alpha);
  SDL_RenderDrawRect(pRenderer, &sdlRect);
}

}}
