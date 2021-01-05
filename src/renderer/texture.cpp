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


void Texture::render(const int x, const int y) const {
  base::Rect<int> fullImageRect{{0, 0}, {width(), height()}};
  render(x, y, fullImageRect);
}


void Texture::render(const base::Vector& position) const {
  render(position.x, position.y);
}


void Texture::render(
  const base::Vector& position,
  const base::Rect<int>& sourceRect
) const {
  render(position.x, position.y, sourceRect);
}


void Texture::renderScaled(const base::Rect<int>& destRect) const {
  mpRenderer->drawTexture(mId, {0.0f, 0.0f, 1.0f, 1.0f}, destRect);
}


void Texture::render(
  const int x,
  const int y,
  const base::Rect<int>& sourceRect
) const {
  base::Rect<int> destRect{
    {x, y},
    {sourceRect.size.width, sourceRect.size.height}
  };
  mpRenderer->drawTexture(
    mId, toTexCoords(sourceRect, mWidth, mHeight), destRect);
}


Texture::Texture(renderer::Renderer* pRenderer, const Image& image)
  : Texture(
      pRenderer,
      pRenderer->createTexture(image),
      static_cast<int>(image.width()),
      static_cast<int>(image.height()))
{
}


Texture::~Texture() {
  if (mpRenderer) {
    mpRenderer->destroyTexture(mId);
  }
}


RenderTargetTexture::RenderTargetTexture(
  renderer::Renderer* pRenderer,
  const int width,
  const int height
)
  : Texture(
      pRenderer,
      pRenderer->createRenderTargetTexture(width, height),
      width,
      height)
{
}


RenderTargetTexture::Binder::Binder(
  RenderTargetTexture& renderTarget,
  renderer::Renderer* pRenderer
)
  : Binder(renderTarget.data(), pRenderer)
{
}


RenderTargetTexture::Binder::Binder(
  const TextureId target,
  renderer::Renderer* pRenderer
)
  : mpRenderer(pRenderer)
{
  pRenderer->pushState();
  pRenderer->setRenderTarget(target);
}


RenderTargetTexture::Binder::~Binder() {
  if (mpRenderer) {
    mpRenderer->popState();
  }
}


RenderTargetTexture::Binder::Binder(Binder&& other)
  : mpRenderer(std::exchange(other.mpRenderer, nullptr))
{
}


auto RenderTargetTexture::Binder::operator=(
  RenderTargetTexture::Binder&& other
) -> Binder& {
  mpRenderer = std::exchange(other.mpRenderer, nullptr);
  return *this;
}


DefaultRenderTargetBinder::DefaultRenderTargetBinder(Renderer* pRenderer)
  : Binder(0, pRenderer)
{
}

}
