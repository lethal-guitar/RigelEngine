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


namespace rigel::renderer {

using data::Image;
using detail::TextureBase;


void TextureBase::render(
  renderer::Renderer* renderer,
  const int x,
  const int y
) const {
  base::Rect<int> fullImageRect{{0, 0}, {width(), height()}};
  render(renderer, x, y, fullImageRect);
}


void TextureBase::render(
  renderer::Renderer* renderer,
  const base::Vector& position
) const {
  render(renderer, position.x, position.y);
}


void TextureBase::render(
  renderer::Renderer* renderer,
  const base::Vector& position,
  const base::Rect<int>& sourceRect
) const {
  render(
    renderer,
    position.x,
    position.y,
    sourceRect);
}


void TextureBase::renderScaled(
  renderer::Renderer* pRenderer,
  const base::Rect<int>& destRect
) const {
  pRenderer->drawTexture(mData, completeSourceRect(), destRect);
}


void TextureBase::renderScaledToScreen(renderer::Renderer* pRenderer) const {
  renderScaled(pRenderer, pRenderer->fullScreenRect());
}


void TextureBase::render(
  renderer::Renderer* pRenderer,
  const int x,
  const int y,
  const base::Rect<int>& sourceRect
) const {
  base::Rect<int> destRect{
    {x, y},
    {sourceRect.size.width, sourceRect.size.height}
  };
  pRenderer->drawTexture(mData, sourceRect, destRect);
}


OwningTexture::OwningTexture(renderer::Renderer* pRenderer, const Image& image)
  : TextureBase(pRenderer->createTexture(image))
{
}


OwningTexture::~OwningTexture() {
  glDeleteTextures(1, &mData.mHandle);
}


RenderTargetTexture::RenderTargetTexture(
  renderer::Renderer* pRenderer,
  const std::size_t width,
  const std::size_t height
)
  : RenderTargetTexture(
      pRenderer->createRenderTargetTexture(int(width), int(height)),
      static_cast<int>(width),
      static_cast<int>(height))
{
}


RenderTargetTexture::RenderTargetTexture(
  const Renderer::RenderTargetHandles& handles,
  const int width,
  const int height
)
  : OwningTexture({width, height, handles.texture})
  , mFboHandle(handles.fbo)
{
}


RenderTargetTexture::~RenderTargetTexture() {
  glDeleteFramebuffers(1, &mFboHandle);
}


RenderTargetTexture::Binder::Binder(
  RenderTargetTexture& renderTarget,
  renderer::Renderer* pRenderer
)
  : Binder(
      {
        base::Size<int>{renderTarget.mData.mWidth, renderTarget.mData.mHeight},
        renderTarget.mFboHandle
      },
      pRenderer)
{
}


RenderTargetTexture::Binder::Binder(
  const renderer::Renderer::RenderTarget& target,
  renderer::Renderer* pRenderer
)
  : mStateSaver(pRenderer)
{
  pRenderer->setRenderTarget(target);
  pRenderer->setGlobalTranslation({});
  pRenderer->setGlobalScale({1.0f, 1.0f});
  pRenderer->setClipRect({});
}


DefaultRenderTargetBinder::DefaultRenderTargetBinder(
  renderer::Renderer* pRenderer
)
  : Binder({{0, 0}, 0}, pRenderer)
{
}

}
