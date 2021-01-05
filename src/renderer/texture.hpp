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
#include "renderer/renderer.hpp"


namespace rigel::renderer {

/** Wrapper class for renderable texture
 *
 * This wrapper class manages the life-time of a texture, and offers
 * a more object-oriented interface.
 *
 * The ownership semantics are the same as for a std::unique_ptr.
 */
class Texture {
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

  Texture& operator=(Texture&& other) noexcept {
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
  void render(const base::Vector& position) const;

  /** Render entire texture at given position */
  void render(int x, int y) const;

  /** Render a part of the texture at given position
   *
   * The sourceRect parameter is interpreted relative to the texture's
   * coordinate system, e.g. (0, 0, width, height) would render the entire
   * texture.
   */
  void render(
    const base::Vector& position, const base::Rect<int>& sourceRect) const;

  /** Render entire texture scaled to fill the given rectangle */
  void renderScaled(const base::Rect<int>& destRect) const;

  int width() const {
    return mWidth;
  }

  int height() const {
    return mHeight;
  }

  base::Extents extents() const {
    return {mWidth, mHeight};
  }

  TextureId data() const {
    return mId;
  }

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
 * It manages life-time like Texture, but creates a SDL_Texture with an
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
class RenderTargetTexture : public Texture {
public:
  friend class Binder;

  class Binder {
  public:
    Binder(RenderTargetTexture& renderTarget, Renderer* pRenderer);
    ~Binder();

    Binder(const Binder&) = delete;
    Binder& operator=(const Binder&) = delete;

    Binder(Binder&&);
    Binder& operator=(Binder&&);

  protected:
    Binder(TextureId, Renderer* pRenderer);

  private:
    TextureId mPreviousRenderTarget;
    Renderer* mpRenderer;
  };

  RenderTargetTexture(Renderer* pRenderer, int width, int height);
};


class DefaultRenderTargetBinder : public RenderTargetTexture::Binder {
public:
  explicit DefaultRenderTargetBinder(renderer::Renderer* pRenderer);
};


[[nodiscard]] inline auto setupDefaultState(Renderer* pRenderer) {
  auto saved = Renderer::StateSaver{pRenderer};
  pRenderer->setGlobalTranslation({});
  pRenderer->setGlobalScale({1.0f, 1.0f});
  pRenderer->setClipRect({});
  return saved;
}

}
