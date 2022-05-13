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

#include "renderer/custom_quad_batch.hpp"

#include <cassert>


namespace rigel::renderer
{

using data::Image;


void Texture::render(const int x, const int y) const
{
  base::Rect<int> fullImageRect{{0, 0}, {width(), height()}};
  render(x, y, fullImageRect);
}


void Texture::render(const base::Vec2& position) const
{
  render(position.x, position.y);
}


void Texture::render(
  const base::Vec2& position,
  const base::Rect<int>& sourceRect) const
{
  render(position.x, position.y, sourceRect);
}


void Texture::render(const base::Rect<int>& sourceRect, const base::Rect<int>& destRect)
  const
{
  mpRenderer->drawTexture(
    mId, toTexCoords(sourceRect, mWidth, mHeight), destRect);
}


void Texture::render(
  const int x,
  const int y,
  const base::Rect<int>& sourceRect) const
{
  base::Rect<int> destRect{
    {x, y}, {sourceRect.size.width, sourceRect.size.height}};
  render(sourceRect, destRect);
}


Texture::Texture(renderer::Renderer* pRenderer, const Image& image)
  : Texture(
      pRenderer,
      pRenderer->createTexture(image),
      static_cast<int>(image.width()),
      static_cast<int>(image.height()))
{
}


Texture::~Texture()
{
  if (mpRenderer)
  {
    mpRenderer->destroyTexture(mId);
  }
}


void drawWithCustomShader(
  Renderer* pRenderer,
  const Texture& texture,
  const base::Vec2& position,
  const Shader& shader)
{
  drawWithCustomShader(
    pRenderer,
    texture,
    {position, {texture.width(), texture.height()}},
    shader);
}


void drawWithCustomShader(
  Renderer* pRenderer,
  const Texture& texture,
  const base::Rect<int>& destRect,
  const Shader& shader)
{
  drawWithCustomShader(
    pRenderer,
    texture,
    {{}, {texture.width(), texture.height()}},
    destRect,
    shader);
}


void drawWithCustomShader(
  Renderer* pRenderer,
  const Texture& texture,
  const base::Rect<int>& sourceRect,
  const base::Rect<int>& destRect,
  const Shader& shader)
{
  const auto textureIds = std::array{texture.data()};
  const auto vertices = createTexturedQuadVertices(
    toTexCoords(sourceRect, texture.width(), texture.height()), destRect);
  pRenderer->drawCustomQuadBatch({textureIds, vertices, &shader});
}


RenderTargetTexture::RenderTargetTexture(
  renderer::Renderer* pRenderer,
  const int width,
  const int height)
  : Texture(
      pRenderer,
      pRenderer->createRenderTargetTexture(width, height),
      width,
      height)
{
}

} // namespace rigel::renderer
