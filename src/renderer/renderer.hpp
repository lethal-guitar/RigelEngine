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

#include "base/color.hpp"
#include "base/spatial_types.hpp"
#include "base/warnings.hpp"
#include "data/image.hpp"

RIGEL_DISABLE_WARNINGS
#include <SDL_video.h>
RIGEL_RESTORE_WARNINGS

#include <cstdint>
#include <memory>
#include <optional>


namespace rigel::renderer {

using TextureId = std::uint32_t;

struct TexCoords {
  float left;
  float top;
  float right;
  float bottom;
};


inline TexCoords toTexCoords(
  const base::Rect<int>& sourceRect,
  const int texWidth,
  const int texHeight
) {
  const auto left = sourceRect.topLeft.x / float(texWidth);
  const auto top = sourceRect.topLeft.y / float(texHeight);
  const auto width = sourceRect.size.width / float(texWidth);
  const auto height = sourceRect.size.height / float(texHeight);
  const auto right = left + width;
  const auto bottom = top + height;

  return {left, top, right, bottom};
}


class Renderer {
public:
  class StateSaver {
  public:
    explicit StateSaver(Renderer* pRenderer)
      : mpRenderer(pRenderer)
    {
      pRenderer->pushState();
    }

    StateSaver(StateSaver&& other) noexcept
      : mpRenderer(std::exchange(other.mpRenderer, nullptr))
    {
    }

    StateSaver& operator=(StateSaver&& other) = delete;

    ~StateSaver() {
      if (mpRenderer) {
        mpRenderer->popState();
      }
    }

    StateSaver(const StateSaver&) = delete;
    StateSaver& operator=(const StateSaver&) = delete;

  private:
    Renderer* mpRenderer;
  };


  explicit Renderer(SDL_Window* pWindow);
  ~Renderer();

  // Drawing API
  ////////////////////////////////////////////////////////////////////////

  void drawTexture(
    TextureId texture,
    const TexCoords& sourceRect,
    const base::Rect<int>& destRect,
    bool repeat = false);

  void drawFilledRectangle(
    const base::Rect<int>& rect,
    const base::Color& color);

  void drawPoint(const base::Vector& position, const base::Color& color);

  void drawWaterEffect(
    const base::Rect<int>& area,
    TextureId unprocessedScreen,
    std::optional<int> surfaceAnimationStep);

  void drawRectangle(const base::Rect<int>& rect, const base::Color& color);

  void drawLine(
    const base::Vector& start,
    const base::Vector& end,
    const base::Color& color
  ) {
    drawLine(start.x, start.y, end.x, end.y, color);
  }

  void drawLine(
    int x1,
    int y1,
    int x2,
    int y2,
    const base::Color& color);

  void clear(const base::Color& clearColor = {0, 0, 0, 255});

  void swapBuffers();
  void submitBatch();

  // Resource management API
  ////////////////////////////////////////////////////////////////////////

  TextureId createTexture(const data::Image& image);
  TextureId createRenderTargetTexture(int width, int height);
  void destroyTexture(TextureId texture);

  // State management API
  ////////////////////////////////////////////////////////////////////////

  void pushState();
  void popState();
  void resetState();

  void setOverlayColor(const base::Color& color);
  void setColorModulation(const base::Color& colorModulation);

  void setGlobalTranslation(const base::Vector& translation);
  base::Vector globalTranslation() const;

  void setGlobalScale(const base::Point<float>& scale);
  base::Point<float> globalScale() const;

  void setClipRect(const std::optional<base::Rect<int>>& clipRect);
  std::optional<base::Rect<int>> clipRect() const;

  void setRenderTarget(TextureId target);

  base::Size<int> windowSize() const;
  base::Size<int> maxWindowSize() const;

private:
  struct Impl;
  std::unique_ptr<Impl> mpImpl;
};

}
