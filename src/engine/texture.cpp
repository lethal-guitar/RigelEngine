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

#include <cassert>


namespace rigel { namespace engine {

using data::Image;


OwningTexture::OwningTexture(
  SDL_Renderer* renderer,
  const Image& image,
  const bool enableBlending)
  : OwningTexture(
      renderer,
      image.width(),
      image.height(),
      false,
      enableBlending)
{
  const void* pixelData = image.pixelData().data();
  SDL_UpdateTexture(texturePtr(), nullptr, pixelData, width()*4);
}


OwningTexture::OwningTexture(
  SDL_Renderer* renderer,
  const std::size_t width,
  const std::size_t height,
  const bool createRenderTarget,
  const bool blendingDesired
)
  : TextureBase(
    SDL_CreateTexture(
      renderer,
      SDL_PIXELFORMAT_ABGR8888,
      createRenderTarget ? SDL_TEXTUREACCESS_TARGET : SDL_TEXTUREACCESS_STATIC,
      static_cast<int>(width),
      static_cast<int>(height)),
      static_cast<int>(width),
      static_cast<int>(height))
{
  enableBlending(blendingDesired);
}


RenderTargetTexture::RenderTargetTexture(
  SDL_Renderer* pRenderer,
  std::size_t width,
  std::size_t height
)
  : OwningTexture(pRenderer, width, height, true, true)
{
}


RenderTargetTexture::Binder::Binder(
  RenderTargetTexture& renderTarget,
  SDL_Renderer* pRenderer
)
  : Binder(renderTarget.texturePtr(), pRenderer)
{
}


RenderTargetTexture::Binder::Binder(
  SDL_Texture* pRenderTarget,
  SDL_Renderer* pRenderer
)
  : mpRenderTarget(pRenderTarget)
  , mpPreviousRenderTarget(SDL_GetRenderTarget(pRenderer))
  , mpRenderer(pRenderer)
{
  SDL_SetRenderTarget(mpRenderer, mpRenderTarget);
}


RenderTargetTexture::Binder::~Binder() {
  SDL_SetRenderTarget(mpRenderer, mpPreviousRenderTarget);
}


DefaultRenderTargetBinder::DefaultRenderTargetBinder(SDL_Renderer* pRenderer)
  : Binder(nullptr, pRenderer)
{
}

}}
