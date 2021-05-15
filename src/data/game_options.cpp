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

#include "game_options.hpp"

#include <algorithm>


namespace rigel::data
{

bool canBeUsedForKeyBinding(const SDL_Keycode keyCode)
{
  using std::begin;
  using std::end;
  using std::find;

  static constexpr auto DISALLOWED_KEYS = std::array<SDL_Keycode, 8>{
    // The following keys are used by the in-game menu system, to enter the
    // menu.
    // We don't want to allow these keys for use in key bindings.
    // We could make it possible to rebind those menu keys as well, but for now,
    // we just disallow their use.
    SDLK_F1,
    SDLK_F2,
    SDLK_F3,
    SDLK_h,
    SDLK_p,

    // The following keys could in theory be used for bindings, but are unlikely
    // to work as expected in practice.
    SDLK_LGUI,
    SDLK_RGUI,
    SDLK_CAPSLOCK};

  return find(begin(DISALLOWED_KEYS), end(DISALLOWED_KEYS), keyCode) ==
    end(DISALLOWED_KEYS);
}

} // namespace rigel::data
