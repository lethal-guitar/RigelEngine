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

RIGEL_DISABLE_WARNINGS
#include <SDL.h>
#include <SDL_mixer.h>
RIGEL_RESTORE_WARNINGS

#include <memory>


namespace rigel { namespace sdl_utils {

namespace detail {

template<typename SDLType>
using PtrBase = std::unique_ptr<SDLType, void(*)(SDLType*)>;


template<typename SDLType>
struct DeleterFor {};

template<>
struct DeleterFor<SDL_Window> {
  static auto deleter() { return &SDL_DestroyWindow; }
};

template<>
struct DeleterFor<Mix_Chunk> {
  static auto deleter() { return &Mix_FreeChunk; }
};

template<typename SDLType>
auto deleterFor() {
  return DeleterFor<SDLType>::deleter();
}

}

template<typename SDLType>
class Ptr : public detail::PtrBase<SDLType> {
public:
  Ptr()
    : Ptr(nullptr)
  {
  }

  template<typename P>
  explicit Ptr(P&& p)
    : detail::PtrBase<SDLType>(
        std::forward<P>(p),
        detail::deleterFor<SDLType>())
  {
  }
};

}}
