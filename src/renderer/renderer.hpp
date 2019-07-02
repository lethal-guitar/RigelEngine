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

class Renderer {
public:
  // TODO: Re-evaluate how render targets work
  struct RenderTarget {
    RenderTarget() = default;
    RenderTarget(const base::Size<int> size, const GLuint fbo)
      : mSize(size)
      , mFbo(fbo)
    {
    }

    bool isDefault() const {
      return mFbo == 0;
    }

    bool operator==(const RenderTarget& other) const {
      return std::tie(mSize, mFbo) == std::tie(other.mSize, other.mFbo);
    }

    bool operator!=(const RenderTarget& other) const {
      return !(*this == other);
    }

    base::Size<int> mSize;
    GLuint mFbo = 0;
  };

  struct TextureData {
    TextureData() = default;
    TextureData(const int width, const int height, const GLuint handle)
      : mWidth(width)
      , mHeight(height)
      , mHandle(handle)
    {
    }

    int mWidth = 0;
    int mHeight = 0;
    GLuint mHandle = 0;
  };

  struct RenderTargetHandles {
    GLuint texture;
    GLuint fbo;
  };


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
    RenderTarget mRenderTarget;
  };


  explicit Renderer(SDL_Window* pWindow);
  ~Renderer();

  base::Rect<int> fullScreenRect() const;
  base::Size<int> windowSize() const {
    return mWindowSize;
  }

  void setOverlayColor(const base::Color& color);
  void setColorModulation(const base::Color& colorModulation);

  void drawTexture(
    const TextureData& textureData,
    const base::Rect<int>& pSourceRect,
    const base::Rect<int>& pDestRect);

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
    TextureData unprocessedScreen,
    std::optional<int> surfaceAnimationStep);

  void setGlobalTranslation(const base::Vector& translation);
  base::Vector globalTranslation() const;

  void setGlobalScale(const base::Point<float>& scale);
  base::Point<float> globalScale() const;

  void setClipRect(const std::optional<base::Rect<int>>& clipRect);
  std::optional<base::Rect<int>> clipRect() const;

  RenderTarget currentRenderTarget() const;
  void setRenderTarget(const RenderTarget& target);

  void clear(const base::Color& clearColor = {0, 0, 0, 255});
  void swapBuffers();

  void submitBatch();

  TextureData createTexture(const data::Image& image);

  // TODO: Revisit the render target API and its use in RenderTargetTexture,
  // there should be a nicer way to do this.
  RenderTargetHandles createRenderTargetTexture(
    int width,
    int height);

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

  bool isVisible(const base::Rect<int>& rect) const;

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
  SDL_Window* mpWindow;

  DummyVao mDummyVao;
  GLuint mStreamVbo;
  GLuint mStreamEbo;

  Shader mTexturedQuadShader;
  Shader mSolidColorShader;
  Shader mWaterEffectShader;

  GLuint mLastUsedShader;
  GLuint mLastUsedTexture;
  base::Color mLastColorModulation;
  base::Color mLastOverlayColor;

  RenderMode mRenderMode;

  std::vector<GLfloat> mBatchData;
  std::vector<GLushort> mBatchIndices;

  TextureData mWaterSurfaceAnimTexture;

  GLuint mCurrentFbo;
  base::Size<int> mWindowSize;
  base::Size<int> mCurrentFramebufferSize;

  glm::mat4 mProjectionMatrix;
  std::optional<base::Rect<int>> mClipRect;
  glm::vec2 mGlobalTranslation;
  glm::vec2 mGlobalScale;
};

}
