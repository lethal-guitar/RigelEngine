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
using detail::TextureBase;


void TextureBase::setAlphaMod(const int alpha) {
  mModulation.a = std::uint8_t(alpha);
}


int TextureBase::alphaMod() const {
  return mModulation.a;
}


void TextureBase::setColorMod(
  const int red,
  const int green,
  const int blue
) {
  mModulation.r = std::uint8_t(red);
  mModulation.g = std::uint8_t(green);
  mModulation.b = std::uint8_t(blue);
}


void TextureBase::render(
  engine::Renderer* renderer,
  const int x,
  const int y
) const {
  base::Rect<int> fullImageRect{{0, 0}, {width(), height()}};
  render(renderer, x, y, fullImageRect);
}


void TextureBase::render(
  engine::Renderer* renderer,
  const base::Vector& position
) const {
  render(renderer, position.x, position.y);
}


void TextureBase::render(
  engine::Renderer* renderer,
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
  engine::Renderer* pRenderer,
  const base::Rect<int>& destRect
) const {
  pRenderer->drawTexture(mData, completeSourceRect(), destRect, mModulation);
}


void TextureBase::renderScaledToScreen(engine::Renderer* pRenderer) const {
  renderScaled(pRenderer, pRenderer->fullScreenRect());
}


void TextureBase::render(
  engine::Renderer* pRenderer,
  const int x,
  const int y,
  const base::Rect<int>& sourceRect
) const {
  base::Rect<int> destRect{
    {x, y},
    {sourceRect.size.width, sourceRect.size.height}
  };
  pRenderer->drawTexture(
    mData,
    sourceRect,
    destRect,
    mModulation);
}


OwningTexture::OwningTexture(engine::Renderer* pRenderer, const Image& image)
  : TextureBase(pRenderer->createTexture(image))
{
}


OwningTexture::~OwningTexture() {
  glDeleteTextures(1, &mData.mHandle);
}


RenderTargetTexture::RenderTargetTexture(
  engine::Renderer* pRenderer,
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
  engine::Renderer* pRenderer
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
  const engine::Renderer::RenderTarget& target,
  engine::Renderer* pRenderer
)
  : mRenderTarget(target)
  , mPreviousRenderTarget(pRenderer->currentRenderTarget())
  , mpRenderer(pRenderer)
{
  mpRenderer->setRenderTarget(mRenderTarget);
}


RenderTargetTexture::Binder::~Binder() {
  mpRenderer->setRenderTarget(mPreviousRenderTarget);
}


DefaultRenderTargetBinder::DefaultRenderTargetBinder(
  engine::Renderer* pRenderer
)
  : Binder({{0, 0}, 0}, pRenderer)
{
}

}}
