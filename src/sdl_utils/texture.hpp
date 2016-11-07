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

#include "ptr.hpp"

#include <base/spatial_types.hpp>
#include <data/image.hpp>

#include <cstddef>
#include <SDL.h>


namespace rigel { namespace sdl_utils {

namespace detail {

template<typename PtrT>
inline SDL_Texture* getTexturePtr(const PtrT& ptr) {
  return ptr;
}

template<>
inline SDL_Texture* getTexturePtr(const Ptr<SDL_Texture>& ptr) {
  return ptr.get();
}

template<typename PtrT>
class TextureBase {
public:
  TextureBase();

  /** Render entire texture at given position */
  void render(SDL_Renderer* renderer, const base::Vector& position) const;

  /** Render entire texture at given position */
  void render(SDL_Renderer* renderer, int x, int y) const;

  /** Render a part of the texture at given position
   *
   * The sourceRect parameter is interpreted relative to the texture's
   * coordinate system, e.g. (0, 0, width, height) would render the entire
   * texture.
   */
  void render(
    SDL_Renderer* renderer,
    const base::Vector& position,
    const base::Rect<int>& sourceRect
  ) const;

  /** Render entire texture scaled to fill the given rectangle */
  void renderScaled(
    SDL_Renderer* renderer,
    const base::Rect<int>& destRect
  ) const;

  /** Render entire texture scaled to fill the entire screen */
  void renderScaledToScreen(SDL_Renderer* pRenderer) const;

  /** Sets the blendmode to BLEND or NONE */
  void enableBlending(bool enable);

  void setAlphaMod(int alpha);
  int alphaMod() const;

  void setColorMod(int r, int g, int b);

  int width() const {
    return mWidth;
  }

  int height() const {
    return mHeight;
  }

  base::Extents extents() const {
    return {mWidth, mHeight};
  }

protected:
  void render(
    SDL_Renderer* renderer,
    int x,
    int y,
    const SDL_Rect& sourceRect) const;

  template<typename PtrInitT>
  TextureBase(PtrInitT&& ptr, const int width, const int height)
    : mpTexture(std::forward<PtrInitT>(ptr))
    , mWidth(width)
    , mHeight(height)
  {
  }

  SDL_Texture* texturePtr() const {
    return getTexturePtr(mpTexture);
  }

  PtrT mpTexture;

private:
  int mWidth;
  int mHeight;
};

}

/** Wrapper class for SDL_Texture
 *
 * This wrapper class manages the life-time of an SDL_Texture, and offers
 * a more object-oriented interface.
 *
 * The ownership semantics are the same as for a std::unique_ptr.
 */
class OwningTexture : public detail::TextureBase<Ptr<SDL_Texture>> {
public:
  OwningTexture() = default;
  OwningTexture(
    SDL_Renderer* renderer,
    const data::Image& image,
    bool enableBlending = true);

  friend class NonOwningTexture;

protected:
  OwningTexture(
    SDL_Renderer* renderer,
    std::size_t width,
    std::size_t height,
    bool createRenderTarget,
    bool enableBlending);
};


/** Non-owning version of OwningTexture
 *
 * This has exactly the same interface as OwningTexture, but it doesn't
 * manage the underlying SDL_Texture's life-time.
 *
 * It behaves like a raw pointer, and clients are responsible for ensuring
 * that the corresponding OwningTexture outlives any NonOwningTexture
 * instances.
 */
class NonOwningTexture : public detail::TextureBase<SDL_Texture*> {
public:
  NonOwningTexture() = default;

  explicit NonOwningTexture(const OwningTexture& texture) :
    TextureBase(texture.texturePtr(), texture.width(), texture.height())
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
    Binder(RenderTargetTexture& renderTarget, SDL_Renderer* pRenderer);
    ~Binder();

    Binder(const Binder&) = delete;
    Binder& operator=(const Binder&) = delete;

  protected:
    Binder(SDL_Texture* pRenderTarget, SDL_Renderer* pRenderer);

  private:
    SDL_Texture* mpRenderTarget;
    SDL_Texture* mpPreviousRenderTarget;
    SDL_Renderer* mpRenderer;
  };

  RenderTargetTexture(
    SDL_Renderer* pRenderer,
    std::size_t width,
    std::size_t height);
};


class DefaultRenderTargetBinder : public RenderTargetTexture::Binder {
public:
  explicit DefaultRenderTargetBinder(SDL_Renderer* pRenderer);
};

}}

#include "texture.ipp"
