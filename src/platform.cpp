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

#include "platform.hpp"

#include "sdl_utils/error.hpp"


namespace rigel::platform {

void setGLAttributes() {
#ifdef RIGEL_USE_GL_ES
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
}


sdl_utils::Ptr<SDL_Window> createWindow(const data::GameOptions& options) {
  SDL_DisplayMode displayMode;
  sdl_utils::check(SDL_GetDesktopDisplayMode(0, &displayMode));

  const auto isFullscreen = options.mWindowMode != data::WindowMode::Windowed;
  const auto windowFlags =
    flagsForWindowMode(options.mWindowMode) |
    SDL_WINDOW_RESIZABLE |
    SDL_WINDOW_OPENGL;

  auto pWindow = sdl_utils::Ptr<SDL_Window>{sdl_utils::check(SDL_CreateWindow(
    "Rigel Engine",
    options.mWindowPosX,
    options.mWindowPosY,
    isFullscreen ? displayMode.w : options.mWindowWidth,
    isFullscreen ? displayMode.h : options.mWindowHeight,
    windowFlags))};

  // Setting a display mode is necessary to make sure that exclusive
  // full-screen mode keeps using the desktop resolution. Without this,
  // switching to exclusive full-screen mode from windowed mode would result in
  // a screen resolution matching the window's last size.
  sdl_utils::check(SDL_SetWindowDisplayMode(pWindow.get(), &displayMode));

  return pWindow;
}


int flagsForWindowMode(const data::WindowMode mode) {
  using WM = data::WindowMode;

  switch (mode) {
    case WM::Fullscreen: return SDL_WINDOW_FULLSCREEN_DESKTOP;
    case WM::ExclusiveFullscreen: return SDL_WINDOW_FULLSCREEN;
    case WM::Windowed: return 0;
  }

  return 0;
}

}