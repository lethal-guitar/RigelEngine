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

#include "texture.hpp"


namespace rigel { namespace sdl_utils {

using data::Image;

namespace detail {

template<typename PtrT>
TextureBase<PtrT>::TextureBase()
  : mpTexture(nullptr)
  , mWidth(0)
  , mHeight(0)
{
}


template<typename PtrT>
void TextureBase<PtrT>::enableBlending(const bool enable) {
  SDL_SetTextureBlendMode(
    texturePtr(),
    enable ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
}


template<typename PtrT>
void TextureBase<PtrT>::setAlphaMod(const int alpha) {
  SDL_SetTextureAlphaMod(texturePtr(), static_cast<Uint8>(alpha));
}


template<typename PtrT>
int TextureBase<PtrT>::alphaMod() const {
  Uint8 alphaModValue = 0;
  SDL_GetTextureAlphaMod(texturePtr(), &alphaModValue);
  return alphaModValue;
}


template<typename PtrT>
void TextureBase<PtrT>::setColorMod(
  const int red,
  const int green,
  const int blue
) {
  SDL_SetTextureColorMod(
    texturePtr(),
    static_cast<Uint8>(red),
    static_cast<Uint8>(green),
    static_cast<Uint8>(blue));
}



template<typename PtrT>
void TextureBase<PtrT>::render(
  SDL_Renderer* renderer,
  const int x,
  const int y
) const {
  SDL_Rect fullImageRect{0, 0, mWidth, mHeight};
  render(renderer, x, y, fullImageRect);
}

template<typename PtrT>
void TextureBase<PtrT>::render(
  SDL_Renderer* renderer,
  const base::Vector& position
) const {
  render(renderer, position.x, position.y);
}

template<typename PtrT>
void TextureBase<PtrT>::render(
  SDL_Renderer* renderer,
  const base::Vector& position,
  const base::Rect<int>& sourceRect
) const {
  render(
    renderer,
    position.x,
    position.y,
    SDL_Rect{
      sourceRect.topLeft.x, sourceRect.topLeft.y,
      sourceRect.size.width, sourceRect.size.height});
}


template<typename PtrT>
void TextureBase<PtrT>::renderScaled(
  SDL_Renderer* renderer,
  const base::Rect<int>& destRect
) const {
  SDL_Rect sdlDestRect{
    destRect.topLeft.x, destRect.topLeft.y,
    destRect.size.width, destRect.size.height};
  SDL_RenderCopy(renderer, texturePtr(), nullptr, &sdlDestRect);
}


template<typename PtrT>
void TextureBase<PtrT>::renderScaledToScreen(SDL_Renderer* renderer) const {
  SDL_RenderCopy(renderer, texturePtr(), nullptr, nullptr);
}


template<typename PtrT>
void TextureBase<PtrT>::render(
    SDL_Renderer* renderer,
    const int x,
    const int y,
    const SDL_Rect& sourceRect
) const {
  SDL_Rect destRect{x, y, sourceRect.w, sourceRect.h};
  SDL_RenderCopy(renderer, texturePtr(), &sourceRect, &destRect);
}

}


}}
