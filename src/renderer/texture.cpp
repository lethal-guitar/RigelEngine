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
  pRenderer->drawTexture(mId, {0.0f, 0.0f, 1.0f, 1.0f}, destRect);
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
  pRenderer->drawTexture(
    mId, toTexCoords(sourceRect, mWidth, mHeight), destRect);
}


OwningTexture::OwningTexture(renderer::Renderer* pRenderer, const Image& image)
  : TextureBase(
      pRenderer->createTexture(image),
      static_cast<int>(image.width()),
      static_cast<int>(image.height()))
{
}


OwningTexture::~OwningTexture() {
  glDeleteTextures(1, &mId);
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
  : OwningTexture(handles.texture, width, height)
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
        base::Size<int>{renderTarget.mWidth, renderTarget.mHeight},
        renderTarget.mFboHandle
      },
      pRenderer)
{
}


RenderTargetTexture::Binder::Binder(
  const renderer::Renderer::RenderTarget& target,
  renderer::Renderer* pRenderer
)
  : mPreviousRenderTarget(pRenderer->currentRenderTarget())
  , mpRenderer(pRenderer)
{
  pRenderer->setRenderTarget(target);
}


RenderTargetTexture::Binder::~Binder() {
  if (mpRenderer) {
    mpRenderer->setRenderTarget(mPreviousRenderTarget);
  }
}


RenderTargetTexture::Binder::Binder(Binder&& other)
  : mPreviousRenderTarget(other.mPreviousRenderTarget)
  , mpRenderer(other.mpRenderer)
{
  // Mark other as "moved from"
  other.mpRenderer = nullptr;
}


auto RenderTargetTexture::Binder::operator=(
  RenderTargetTexture::Binder&& other
) -> Binder& {
  mPreviousRenderTarget = other.mPreviousRenderTarget;
  mpRenderer = other.mpRenderer;

  // Mark other as "moved from"
  other.mpRenderer = nullptr;
  return *this;
}


DefaultRenderTargetBinder::DefaultRenderTargetBinder(
  renderer::Renderer* pRenderer
)
  : Binder({{0, 0}, 0}, pRenderer)
{
}

}
