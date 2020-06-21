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

#include "opengl.hpp"

#include "sdl_utils/error.hpp"

RIGEL_DISABLE_WARNINGS
#include <SDL_video.h>
RIGEL_RESTORE_WARNINGS

#include <stdexcept>


void rigel::renderer::loadGlFunctions() {
  int result = 0;
#ifdef RIGEL_USE_GL_ES
  result = gladLoadGLES2Loader([](const char* proc) {
    return sdl_utils::check(SDL_GL_GetProcAddress(proc));
  });
#else
  result = gladLoadGLLoader([](const char* proc) {
    return sdl_utils::check(SDL_GL_GetProcAddress(proc));
  });
#endif

  if (!result) {
    throw std::runtime_error("Failed to load OpenGL function pointers");
  }
}
