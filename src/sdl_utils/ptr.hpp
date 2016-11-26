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

}

template<typename SDLType>
class Ptr : public detail::PtrBase<SDLType> {
public:
  template<typename P, typename D>
  Ptr(P&& p, D&& d)
    : detail::PtrBase<SDLType>(std::forward<P>(p), std::forward<D>(d))
  {
  }
};

template<>
class Ptr<SDL_Renderer> : public detail::PtrBase<SDL_Renderer> {
public:
  explicit Ptr(SDL_Renderer* r)
    : detail::PtrBase<SDL_Renderer>(r, SDL_DestroyRenderer)
  {
  }
};


template<>
class Ptr<SDL_Window> : public detail::PtrBase<SDL_Window> {
public:
  explicit Ptr(SDL_Window* w)
    : detail::PtrBase<SDL_Window>(w, SDL_DestroyWindow)
  {
  }
};


template<>
class Ptr<SDL_Texture> : public detail::PtrBase<SDL_Texture> {
public:
  explicit Ptr(SDL_Texture* t)
    : detail::PtrBase<SDL_Texture>(t, SDL_DestroyTexture)
  {
  }
};


template<>
class Ptr<Mix_Chunk> : public detail::PtrBase<Mix_Chunk> {
public:
  explicit Ptr(Mix_Chunk* c)
    : detail::PtrBase<Mix_Chunk>(c, Mix_FreeChunk)
  {
  }
};


}}
