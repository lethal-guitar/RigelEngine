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

#include "base/spatial_types.hpp"
#include "base/warnings.hpp"
#include "data/image.hpp"
#include "engine/renderer.hpp"
#include "engine/opengl.hpp"

#include <cstddef>


namespace rigel { namespace engine {

namespace detail {

class TextureBase {
public:
  TextureBase() = default;

  /** Render entire texture at given position */
  void render(engine::Renderer* renderer, const base::Vector& position) const;

  /** Render entire texture at given position */
  void render(engine::Renderer* renderer, int x, int y) const;

  /** Render a part of the texture at given position
   *
   * The sourceRect parameter is interpreted relative to the texture's
   * coordinate system, e.g. (0, 0, width, height) would render the entire
   * texture.
   */
  void render(
    engine::Renderer* renderer,
    const base::Vector& position,
    const base::Rect<int>& sourceRect
  ) const;

  /** Render entire texture scaled to fill the given rectangle */
  void renderScaled(
    engine::Renderer* renderer,
    const base::Rect<int>& destRect
  ) const;

  /** Render entire texture scaled to fill the entire screen */
  void renderScaledToScreen(engine::Renderer* pRenderer) const;

  void setAlphaMod(int alpha);
  int alphaMod() const;

  void setColorMod(int r, int g, int b);

  int width() const {
    return mData.mWidth;
  }

  int height() const {
    return mData.mHeight;
  }

  base::Extents extents() const {
    return {mData.mWidth, mData.mHeight};
  }

protected:
  base::Rect<int> completeSourceRect() const {
    return {{0, 0}, extents()};
  }

  void render(
    engine::Renderer* renderer,
    int x,
    int y,
    const base::Rect<int>& sourceRect) const;

  TextureBase(Renderer::TextureData data)
    : mData(data)
    , mModulation({255, 255, 255, 255})
  {
  }

  Renderer::TextureData mData;
  base::Color mModulation;
};

}

/** Wrapper class for renderable texture
 *
 * This wrapper class manages the life-time of a texture, and offers
 * a more object-oriented interface.
 *
 * The ownership semantics are the same as for a std::unique_ptr.
 */
class OwningTexture : public detail::TextureBase {
public:
  OwningTexture() = default;
  OwningTexture(engine::Renderer* renderer, const data::Image& image);
  ~OwningTexture();

  OwningTexture(OwningTexture&& other)
    : TextureBase(other.mData)
  {
    other.mData.mHandle = 0;
  }

  OwningTexture(const OwningTexture&) = delete;
  OwningTexture& operator=(const OwningTexture&) = delete;

  OwningTexture& operator=(OwningTexture&& other) {
    mData = other.mData;
    other.mData.mHandle = 0;
    return *this;
  }

  friend class NonOwningTexture;

protected:
  OwningTexture(Renderer::TextureData data)
    : TextureBase(data)
  {
  }
};


/** Non-owning version of OwningTexture
 *
 * This has exactly the same interface as OwningTexture, but it doesn't
 * manage the underlying texture's life-time.
 *
 * It behaves like a raw pointer, and clients are responsible for ensuring
 * that the corresponding OwningTexture outlives any NonOwningTexture
 * instances.
 */
class NonOwningTexture : public detail::TextureBase {
public:
  NonOwningTexture() = default;

  explicit NonOwningTexture(const OwningTexture& texture) :
    TextureBase(texture.mData)
  {
  }
};


/** Utility class for render target type textures
 *
 * It manages life-time like OwningTexture, but creates a SDL_Texture with an
 * access type of SDL_TEXTUREACCESS_TARGET.
 *
 * It also offers a RAII helper class for safe binding/unbinding of the
 * render target.
 *
 * Example use:
 *
 *   RenderTargetTexture renderTarget(pRenderer, 640, 480);
 *
 *   {
 *     RenderTargetTexture::Binder bindTarget(renderTarget);
 *
 *     // someOtherTexture will be drawn into renderTarget, not on the screen
 *     someOtherTexture.render(pRenderer, 0, 0);
 *   }
 *
 *   // Now draw the previously filled render target to the screen
 *   renderTarget.render(pRenderer, 100, 50);
 *
 *
 * Note that it's safe to nest render target bindings, e.g. you can do:
 *
 *   RenderTargetTexture::Binder bindFirstTarget(targetOne);
 *
 *   // do some rendering, it will go into targetOne
 *
 *   {
 *     RenderTargetTexture::Binder bindAnotherTarget(targetTwo);
 *
 *     // render into targetTwo
 *   }
 *
 *   // This will now end up in targetOne
 *   targetTwo.render(pRenderer, 0, 0);
 *
 * Once the outermost scope's Binder is destroyed, the default render target
 * will be active again (i.e. drawing to the screen)
 */
class RenderTargetTexture : public OwningTexture {
public:
  friend class Binder;

  class Binder {
  public:
    Binder(RenderTargetTexture& renderTarget, engine::Renderer* pRenderer);
    ~Binder();

    Binder(const Binder&) = delete;
    Binder& operator=(const Binder&) = delete;

  protected:
    Binder(const engine::Renderer::RenderTarget&, engine::Renderer* pRenderer);

  private:
    engine::Renderer::RenderTarget mRenderTarget;
    engine::Renderer::RenderTarget mPreviousRenderTarget;
    engine::Renderer* mpRenderer;
  };

  RenderTargetTexture(
    engine::Renderer* pRenderer,
    std::size_t width,
    std::size_t height);
  ~RenderTargetTexture();

private:
  RenderTargetTexture(
    const Renderer::RenderTargetHandles& handles,
    int width,
    int height);

private:
  GLuint mFboHandle;
};


class DefaultRenderTargetBinder : public RenderTargetTexture::Binder {
public:
  explicit DefaultRenderTargetBinder(engine::Renderer* pRenderer);
};

}}
