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
#include "engine/opengl.hpp"
#include "engine/shader.hpp"

RIGEL_DISABLE_WARNINGS
#include <glm/mat4x4.hpp>
#include <SDL_video.h>
RIGEL_RESTORE_WARNINGS

#include <cstdint>
#include <tuple>


namespace rigel { namespace engine {

class Renderer {
public:
  // TODO: Re-evaluate how render targets work
  struct RenderTarget {
    RenderTarget() = default;
    RenderTarget(const int width, const int height, const GLuint fbo)
      : mWidth(width)
      , mHeight(height)
      , mFbo(fbo)
    {
    }

    bool isDefault() const {
      return mFbo == 0;
    }

    bool operator==(const RenderTarget& other) const {
      return
        std::tie(mWidth, mHeight, mFbo) ==
        std::tie(other.mWidth, other.mHeight, other.mFbo);
    }

    bool operator!=(const RenderTarget& other) const {
      return !(*this == other);
    }

    int mWidth = 0;
    int mHeight = 0;
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


  explicit Renderer(SDL_Window* pWindow);
  ~Renderer();

  base::Rect<int> fullScreenRect() const;

  void setOverlayColor(const base::Color& color);

  void drawTexture(
    const TextureData& textureData,
    const base::Rect<int>& pSourceRect,
    const base::Rect<int>& pDestRect,
    const base::Color& colorModulation);

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

  RenderTarget currentRenderTarget() const;
  void setRenderTarget(const RenderTarget& target);

  void clear();
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
    Points
  };

  bool isVisible(const base::Rect<int>& rect) const;

  void useShaderIfChanged(Shader& shader);
  void setRenderModeIfChanged(RenderMode mode);
  void setRenderMode(RenderMode mode);
  void onRenderTargetChanged();

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

  GLuint mLastUsedShader;
  GLuint mLastUsedTexture;
  base::Color mLastColorModulation;
  base::Color mLastOverlayColor;

  RenderMode mRenderMode;

  std::vector<GLfloat> mBatchData;
  std::vector<GLushort> mBatchIndices;

  GLuint mCurrentFbo;
  int mCurrentFramebufferWidth;
  int mCurrentFramebufferHeight;

  glm::mat4 mProjectionMatrix;
  base::Rect<int> mDefaultViewport;
};

}}
