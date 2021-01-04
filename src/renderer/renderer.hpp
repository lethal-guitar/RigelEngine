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
#include "renderer/opengl.hpp"
#include "renderer/shader.hpp"

RIGEL_DISABLE_WARNINGS
#include <glm/mat4x4.hpp>
#include <SDL_video.h>
RIGEL_RESTORE_WARNINGS

#include <cstdint>
#include <optional>
#include <tuple>


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
      , mClipRect(pRenderer->clipRect())
      , mGlobalTranslation(pRenderer->globalTranslation())
      , mGlobalScale(pRenderer->globalScale())
      , mRenderTarget(pRenderer->currentRenderTarget())
    {
    }

    StateSaver(StateSaver&& other) noexcept
      : mpRenderer(other.mpRenderer)
      , mClipRect(other.mClipRect)
      , mGlobalTranslation(other.mGlobalTranslation)
      , mGlobalScale(other.mGlobalScale)
      , mRenderTarget(other.mRenderTarget)
    {
      // Mark the moved-from object as such (see ~StateSaver)
      other.mpRenderer = nullptr;
    }

    StateSaver& operator=(StateSaver&& other) noexcept {
      if (&other != this) {
        *this = std::move(other);
      }

      return *this;
    }

    ~StateSaver() {
      // A nullptr renderer indicates a moved-from state, where we shouldn't
      // reset anything
      if (mpRenderer) {
        mpRenderer->setRenderTarget(mRenderTarget);
        mpRenderer->setClipRect(mClipRect);
        mpRenderer->setGlobalTranslation(mGlobalTranslation);
        mpRenderer->setGlobalScale(mGlobalScale);
      }
    }

    StateSaver(const StateSaver&) = delete;
    StateSaver& operator=(const StateSaver&) = delete;

  private:
    Renderer* mpRenderer;
    std::optional<base::Rect<int>> mClipRect;
    base::Vector mGlobalTranslation;
    base::Point<float> mGlobalScale;
    TextureId mRenderTarget;
  };


  explicit Renderer(SDL_Window* pWindow);
  ~Renderer();

  base::Size<int> windowSize() const {
    return mWindowSize;
  }
  base::Size<int> maxWindowSize() const {
    return mMaxWindowSize;
  }

  void setOverlayColor(const base::Color& color);
  void setColorModulation(const base::Color& colorModulation);

  void drawTexture(
    TextureId texture,
    const TexCoords& sourceRect,
    const base::Rect<int>& destRect,
    bool repeat = false);

  void drawFilledRectangle(
    const base::Rect<int>& rect,
    const base::Color& color);

  void drawRectangle(
    const base::Rect<int>& rect,
    const base::Color& color);

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


  void drawPoint(const base::Vector& position, const base::Color& color);

  void drawWaterEffect(
    const base::Rect<int>& area,
    TextureId unprocessedScreen,
    std::optional<int> surfaceAnimationStep);

  void setGlobalTranslation(const base::Vector& translation);
  base::Vector globalTranslation() const;

  void setGlobalScale(const base::Point<float>& scale);
  base::Point<float> globalScale() const;

  void setClipRect(const std::optional<base::Rect<int>>& clipRect);
  std::optional<base::Rect<int>> clipRect() const;

  TextureId currentRenderTarget() const;
  void setRenderTarget(TextureId target);

  void clear(const base::Color& clearColor = {0, 0, 0, 255});
  void swapBuffers();

  void submitBatch();

  TextureId createTexture(const data::Image& image);
  TextureId createRenderTargetTexture(int width, int height);
  void destroyTexture(TextureId texture);

private:
  class DummyVao {
#ifndef RIGEL_USE_GL_ES
  public:
    DummyVao() {
      glGenVertexArrays(1, &mVao);
      glBindVertexArray(mVao);
    }

    ~DummyVao() {
      glDeleteVertexArrays(1, &mVao);
    }

    DummyVao(const DummyVao&) = delete;
    DummyVao& operator=(const DummyVao&) = delete;

  private:
    GLuint mVao;
#endif
  };

  enum class RenderMode {
    SpriteBatch,
    NonTexturedRender,
    Points,
    WaterEffect
  };

  template <typename VertexIter>
  void batchQuadVertices(
    VertexIter&& dataBegin,
    VertexIter&& dataEnd,
    const std::size_t attributesPerVertex);

  void useShaderIfChanged(Shader& shader);
  void setRenderModeIfChanged(RenderMode mode);
  void updateShaders();
  void onRenderTargetChanged();
  void updateProjectionMatrix();

  GLuint createGlTexture(
    GLsizei width,
    GLsizei height,
    const GLvoid* const pData);

private:
  struct RenderTarget {
    base::Extents mSize;
    GLuint mFbo;
  };

  SDL_Window* mpWindow;

  DummyVao mDummyVao;
  GLuint mStreamVbo;
  GLuint mStreamEbo;

  Shader mTexturedQuadShader;
  Shader mSimpleTexturedQuadShader;
  Shader mSolidColorShader;
  Shader mWaterEffectShader;

  GLuint mLastUsedShader;
  GLuint mLastUsedTexture;
  base::Color mLastColorModulation;
  base::Color mLastOverlayColor;
  bool mTextureRepeatOn = false;

  RenderMode mRenderMode;

  std::vector<GLfloat> mBatchData;
  std::vector<GLushort> mBatchIndices;
  std::unordered_map<TextureId, RenderTarget> mRenderTargetDict;

  TextureId mWaterSurfaceAnimTexture;
  TextureId mWaterEffectColorMapTexture;

  TextureId mCurrentRenderTargetTexture;
  GLuint mCurrentFbo = 0;
  base::Size<int> mWindowSize;
  base::Size<int> mMaxWindowSize;
  base::Size<int> mCurrentFramebufferSize;

  glm::mat4 mProjectionMatrix;
  std::optional<base::Rect<int>> mClipRect;
  glm::vec2 mGlobalTranslation;
  glm::vec2 mGlobalScale;
};

}
