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

#include "base/warnings.hpp"

RIGEL_DISABLE_WARNINGS
#include <SDL_keycode.h>
RIGEL_RESTORE_WARNINGS


namespace rigel::sdl_utils {

/** Normalize keys with left/right variant to always use the left one
 *
 * For keys that usually exist on both sides of the keyboard, like Shift, Alt
 * etc., SDL provides separate key codes for the left and right variant.
 * In order to treat both versions of those keys as the same, this function
 * always returns the left variant for any of these keys. Other key codes
 * are returned unchanged.
 */
inline SDL_Keycode normalizeLeftRightVariants(const SDL_Keycode keyCode) {
  if (keyCode == SDLK_RCTRL) {
    return SDLK_LCTRL;
  } else if (keyCode == SDLK_RALT) {
    return SDLK_LALT;
  } else if (keyCode == SDLK_RSHIFT) {
    return SDLK_LSHIFT;
  } else if (keyCode == SDLK_RGUI) {
    return SDLK_LGUI;
  }

  return keyCode;
}

}
