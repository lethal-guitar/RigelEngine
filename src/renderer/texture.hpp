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

#include "base/defer.hpp"
#include "base/spatial_types.hpp"
#include "data/image.hpp"
#include "renderer/renderer.hpp"


namespace rigel::renderer
{

/** Image (bitmap) residing in GPU memory
 *
 * This is an abstraction over the low-level texture management API
 * provided by the Renderer class. It automatically manages life-time
 * and offers convenient drawing functions for various use cases.
 */
class Texture
{
public:
  Texture() = default;
  Texture(Renderer* renderer, const data::Image& image);
  ~Texture();

  Texture(Texture&& other) noexcept
    : Texture(
        std::exchange(other.mpRenderer, nullptr),
        std::exchange(other.mId, 0),
        other.mWidth,
        other.mHeight)
  {
  }

  Texture& operator=(Texture&& other) noexcept
  {
    using std::swap;

    swap(mpRenderer, other.mpRenderer);
    swap(mId, other.mId);
    mWidth = other.mWidth;
    mHeight = other.mHeight;

    return *this;
  }

  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;

  /** Render entire texture at given position */
  void render(const base::Vec2& position) const;

  /** Render entire texture at given position */
  void render(int x, int y) const;

  /** Render a part of the texture at given position
   *
   * The sourceRect parameter is interpreted relative to the texture's
   * coordinate system, e.g. (0, 0, width, height) would render the entire
   * texture. If texture repeat is enabled in the renderer, the sourceRect
   * can be larger than the texture itself, which will cause the texture
   * to be drawn multiple times (repeated).
   */
  void
    render(const base::Vec2& position, const base::Rect<int>& sourceRect) const;

  /** Render entire texture scaled to fill the given rectangle */
  void renderScaled(const base::Rect<int>& destRect) const;

  int width() const { return mWidth; }

  int height() const { return mHeight; }

  base::Extents extents() const { return {mWidth, mHeight}; }

  TextureId data() const { return mId; }

protected:
  Texture(Renderer* pRenderer, TextureId id, int width, int height)
    : mpRenderer(pRenderer)
    , mId(id)
    , mWidth(width)
    , mHeight(height)
  {
  }

  void render(int x, int y, const base::Rect<int>& sourceRect) const;

  Renderer* mpRenderer = nullptr;
  TextureId mId = 0;
  int mWidth = 0;
  int mHeight = 0;
};


/** Utility class for render target type textures
 *
 * Like the Texture class, this is an abstraction over the Renderer API.
 * It functions like a regular texture, but additionally offers a bind()
 * function to safely bind and unbind it for use as a render target.
 *
 * Example use:
 *
 *   RenderTargetTexture renderTarget(pRenderer, 640, 480);
 *
 *   {
 *     auto binding = renderTarget.bind();
 *
 *     // someOtherTexture will be drawn into renderTarget, not on the screen
 *     someOtherTexture.render(0, 0);
 *   }
 *
 *   // Now draw the previously filled render target to the screen
 *   renderTarget.render(100, 50);
 *
 *
 * Note that it's safe to nest render target bindings, e.g. you can do:
 *
 *   auto outerBinding = targetOne.bind();
 *
 *   // do some rendering, it will go into targetOne
 *
 *   {
 *     auto innerBinding = targetTwo.bind();
 *
 *     // render into targetTwo
 *   }
 *
 *   // This will now end up in targetOne
 *   targetTwo.render(0, 0);
 *
 * Once the outermost scope's binding is destroyed, the default render target
 * will be active again (i.e. drawing to the screen)
 */
class RenderTargetTexture : public Texture
{
public:
  RenderTargetTexture(Renderer* pRenderer, int width, int height);

  [[nodiscard]] auto bind()
  {
    mpRenderer->pushState();
    mpRenderer->setRenderTarget(data());
    return base::defer([this]() { mpRenderer->popState(); });
  }

  [[nodiscard]] auto bindAndReset()
  {
    mpRenderer->pushState();
    mpRenderer->resetState();
    mpRenderer->setRenderTarget(data());
    return base::defer([this]() { mpRenderer->popState(); });
  }
};

} // namespace rigel::renderer
