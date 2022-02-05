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

#include "renderer.hpp"

#include "assets/palette.hpp"
#include "data/game_options.hpp"
#include "data/game_traits.hpp"
#include "renderer/opengl.hpp"
#include "renderer/shader.hpp"
#include "renderer/shader_code.hpp"
#include "renderer/vertex_buffer_utils.hpp"
#include "sdl_utils/error.hpp"

RIGEL_DISABLE_WARNINGS
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
RIGEL_RESTORE_WARNINGS

#include <algorithm>
#include <array>
#include <iterator>


namespace rigel::renderer
{

namespace
{

const GLushort QUAD_INDICES[] = {0, 2, 1, 2, 3, 1};

constexpr std::array<GLenum, MAX_MULTI_TEXTURES> TEXTURE_UNIT_IDS{
  GL_TEXTURE0,
  GL_TEXTURE1,
  GL_TEXTURE2,
  GL_TEXTURE3,
  GL_TEXTURE4,
  GL_TEXTURE5,
  GL_TEXTURE6,
  GL_TEXTURE7};

constexpr auto MAX_QUADS_PER_BATCH = 1280u;
constexpr auto MAX_BATCH_SIZE = MAX_QUADS_PER_BATCH * std::size(QUAD_INDICES);


class DummyVao
{
#ifndef RIGEL_USE_GL_ES
public:
  DummyVao()
  {
    glGenVertexArrays(1, &mVao);
    glBindVertexArray(mVao);
  }

  ~DummyVao() { glDeleteVertexArrays(1, &mVao); }

  DummyVao(const DummyVao&) = delete;
  DummyVao& operator=(const DummyVao&) = delete;

private:
  GLuint mVao;
#endif
};


struct RenderTarget
{
  base::Extents mSize;
  GLuint mFbo;
};


enum class RenderMode : std::uint8_t
{
  SpriteBatch,
  NonTexturedRender,
  Points,
  CustomDrawing
};


glm::vec4 toGlColor(const base::Color& color)
{
  return glm::vec4{color.r, color.g, color.b, color.a} / 255.0f;
}


void* toAttribOffset(std::uintptr_t offset)
{
  return reinterpret_cast<void*>(offset);
}


void setScissorBox(
  const base::Rect<int>& clipRect,
  const base::Size<int>& frameBufferSize)
{
  const auto offsetAtBottom = frameBufferSize.height - clipRect.bottom();
  glScissor(
    clipRect.topLeft.x,
    offsetAtBottom - 1,
    clipRect.size.width,
    clipRect.size.height);
}


void setVertexLayout(const VertexLayout layout)
{
  switch (layout)
  {
    case VertexLayout::PositionAndTexCoords:
      glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, toAttribOffset(0));
      glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(float) * 4,
        toAttribOffset(sizeof(float) * 2));
      break;

    case VertexLayout::PositionAndColor:
      glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 6, toAttribOffset(0));
      glVertexAttribPointer(
        1,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(float) * 6,
        toAttribOffset(sizeof(float) * 2));
  }
}


auto getSize(SDL_Window* pWindow)
{
  int windowWidth = 0;
  int windowHeight = 0;
  SDL_GL_GetDrawableSize(pWindow, &windowWidth, &windowHeight);
  return base::Size<int>{windowWidth, windowHeight};
}


GLuint createGlTexture(
  const GLsizei width,
  const GLsizei height,
  const GLvoid* const pData)
{
  GLuint handle = 0;
  glGenTextures(1, &handle);

  glBindTexture(GL_TEXTURE_2D, handle);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGBA,
    width,
    height,
    0,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    pData);
  return handle;
}

} // namespace


struct Renderer::Impl
{
  struct State
  {
    std::optional<base::Rect<int>> mClipRect;
    base::Color mColorModulation{255, 255, 255, 255};
    base::Color mOverlayColor;
    glm::vec2 mGlobalTranslation{0.0f, 0.0f};
    glm::vec2 mGlobalScale{1.0f, 1.0f};
    TextureId mRenderTargetTexture = 0;
    bool mTextureRepeatEnabled = false;

    friend bool operator==(const State& lhs, const State& rhs)
    {
      // clang-format off
      return
        std::tie(
          lhs.mClipRect,
          lhs.mColorModulation,
          lhs.mOverlayColor,
          lhs.mGlobalTranslation,
          lhs.mGlobalScale,
          lhs.mRenderTargetTexture,
          lhs.mTextureRepeatEnabled) ==
        std::tie(
          rhs.mClipRect,
          rhs.mColorModulation,
          rhs.mOverlayColor,
          rhs.mGlobalTranslation,
          rhs.mGlobalScale,
          rhs.mRenderTargetTexture,
          rhs.mTextureRepeatEnabled);
      // clang-format on
    }

    friend bool operator!=(const State& lhs, const State& rhs)
    {
      return !(lhs == rhs);
    }

    bool needsExtendedShader() const
    {
      // clang-format off
      return
        mTextureRepeatEnabled ||
        mOverlayColor != base::Color{} ||
        mColorModulation != base::Color{255, 255, 255, 255};
      // clang-format on
    }
  };

  // hot - meant to fit into a single cache line.
  // needed for batching/rendering
  std::vector<GLfloat> mBatchData;
  std::vector<State> mStateStack{State{}};
  GLuint mLastUsedTexture = 0;
  GLuint mQuadIndicesEbo = 0;
  std::uint16_t mBatchSize = 0;
  RenderMode mRenderMode = RenderMode::SpriteBatch;
  bool mStateChanged = true;

  // warm - needed for committing state changes
  State mLastCommittedState;
  std::unordered_map<TextureId, RenderTarget> mRenderTargetDict;
  Shader mTexturedQuadShader;
  Shader mSimpleTexturedQuadShader;
  Shader mSolidColorShader;
  base::Size<int> mWindowSize;
  base::Size<int> mLastKnownWindowSize;
  SDL_Window* mpWindow;
  RenderMode mLastKnownRenderMode = RenderMode::SpriteBatch;

  // cold
  int mNumTextures = 0;
  DummyVao mDummyVao;
  GLuint mStreamVbo = 0;


  explicit Impl(SDL_Window* pWindow)
    : mTexturedQuadShader(TEXTURED_QUAD_SHADER)
    , mSimpleTexturedQuadShader(SIMPLE_TEXTURED_QUAD_SHADER)
    , mSolidColorShader(SOLID_COLOR_SHADER)
    , mWindowSize(getSize(pWindow))
    , mpWindow(pWindow)
  {
    // General configuration
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set up a VBO for streaming data to the GPU, stays bound all the time
    glGenBuffers(1, &mStreamVbo);
    glBindBuffer(GL_ARRAY_BUFFER, mStreamVbo);

    // Set up an index buffer with enough indices to handle the largest
    // possible batch size. This is only sent to the GPU once, reducing the
    // amount of data we need to send for each batch.
    {
      glGenBuffers(1, &mQuadIndicesEbo);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mQuadIndicesEbo);

      std::vector<GLushort> indices;
      indices.reserve(MAX_BATCH_SIZE);

      for (auto i = 0u; i < MAX_QUADS_PER_BATCH; ++i)
      {
        for (auto index : QUAD_INDICES)
        {
          indices.push_back(GLushort(index + 4 * i));
        }
      }

      glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(GLushort) * indices.size(),
        indices.data(),
        GL_STATIC_DRAW);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    // All shaders have exactly two vertex attributes
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glViewport(0, 0, mWindowSize.width, mWindowSize.height);
    commitShaderSelection(mStateStack.back());
    commitTransformationMatrix(mStateStack.back(), mWindowSize);
  }


  ~Impl()
  {
    // Make sure all textures and render targets have been destroyed
    // before the renderer is destroyed.
    assert(mRenderTargetDict.empty());
    assert(mNumTextures == 0);

    glDeleteBuffers(1, &mStreamVbo);
    glDeleteBuffers(1, &mQuadIndicesEbo);
  }


  void drawTexture(
    const TextureId texture,
    const TexCoords& sourceRect,
    const base::Rect<int>& destRect)
  {
    updateState(mRenderMode, RenderMode::SpriteBatch);

    if (texture != mLastUsedTexture)
    {
      submitBatch();

      glBindTexture(GL_TEXTURE_2D, texture);
      mLastUsedTexture = texture;
    }

    if (mBatchSize >= MAX_BATCH_SIZE)
    {
      submitBatch();
    }

    const auto vertices = createTexturedQuadVertices(sourceRect, destRect);
    mBatchData.insert(
      mBatchData.end(), std::begin(vertices), std::end(vertices));
    mBatchSize += std::uint16_t(std::size(QUAD_INDICES));
  }


  void submitBatch()
  {
    commitChangedState();

    if (mBatchData.empty())
    {
      return;
    }

    switch (mRenderMode)
    {
      case RenderMode::SpriteBatch:
        glBufferData(
          GL_ARRAY_BUFFER,
          sizeof(float) * mBatchData.size(),
          mBatchData.data(),
          GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mQuadIndicesEbo);
        glDrawElements(GL_TRIANGLES, mBatchSize, GL_UNSIGNED_SHORT, nullptr);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        break;

      case RenderMode::Points:
        glBufferData(
          GL_ARRAY_BUFFER,
          sizeof(float) * mBatchData.size(),
          mBatchData.data(),
          GL_STREAM_DRAW);
        glDrawArrays(GL_POINTS, 0, GLsizei(mBatchData.size() / 6));
        break;

      case RenderMode::CustomDrawing:
      case RenderMode::NonTexturedRender:
        // No batching yet for NonTexturedRender, and we aren't meant to ever
        // see mRenderMode set to CustomDrawing.
        assert(false);
        break;
    }

    mBatchData.clear();
    mBatchSize = 0;
  }


  void
    drawFilledRectangle(const base::Rect<int>& rect, const base::Color& color)
  {
    // Note: No batching for now
    updateState(mRenderMode, RenderMode::NonTexturedRender);
    commitChangedState();

    const auto left = float(rect.left());
    const auto right = float(rect.right()) + 1.0f;
    const auto top = float(rect.top());
    const auto bottom = float(rect.bottom()) + 1.0f;

    const auto colorVec = toGlColor(color);
    float vertices[] = {
      left,  bottom, colorVec.r, colorVec.g, colorVec.b, colorVec.a,
      right, bottom, colorVec.r, colorVec.g, colorVec.b, colorVec.a,
      left,  top,    colorVec.r, colorVec.g, colorVec.b, colorVec.a,
      right, top,    colorVec.r, colorVec.g, colorVec.b, colorVec.a,
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }


  void drawRectangle(const base::Rect<int>& rect, const base::Color& color)
  {
    // Note: No batching for now, drawRectangle is only used for debugging at
    // the moment
    updateState(mRenderMode, RenderMode::NonTexturedRender);
    commitChangedState();

    const auto left = float(rect.left());
    const auto right = float(rect.right());
    const auto top = float(rect.top());
    const auto bottom = float(rect.bottom());

    const auto colorVec = toGlColor(color);
    float vertices[] = {
      left,  top,    colorVec.r, colorVec.g, colorVec.b, colorVec.a,
      left,  bottom, colorVec.r, colorVec.g, colorVec.b, colorVec.a,
      right, bottom, colorVec.r, colorVec.g, colorVec.b, colorVec.a,
      right, top,    colorVec.r, colorVec.g, colorVec.b, colorVec.a,
      left,  top,    colorVec.r, colorVec.g, colorVec.b, colorVec.a};

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
    glDrawArrays(GL_LINE_STRIP, 0, 5);
  }


  void drawLine(
    const int x1,
    const int y1,
    const int x2,
    const int y2,
    const base::Color& color)
  {
    // Note: No batching for now, drawLine is only used for debugging at the
    // moment
    updateState(mRenderMode, RenderMode::NonTexturedRender);
    commitChangedState();

    const auto colorVec = toGlColor(color);

    // clang-format off
    float vertices[] = {
      float(x1), float(y1), colorVec.r, colorVec.g, colorVec.b, colorVec.a,
      float(x2), float(y2), colorVec.r, colorVec.g, colorVec.b, colorVec.a
    };
    // clang-format on

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
    glDrawArrays(GL_LINE_STRIP, 0, 2);
  }


  void drawPoint(const base::Vec2& position, const base::Color& color)
  {
    updateState(mRenderMode, RenderMode::Points);

    float vertices[] = {
      float(position.x),
      float(position.y),
      color.r / 255.0f,
      color.g / 255.0f,
      color.b / 255.0f,
      color.a / 255.0f};
    mBatchData.insert(
      std::end(mBatchData), std::begin(vertices), std::end(vertices));
  }


  void drawCustomQuadBatch(const CustomQuadBatchData& batch)
  {
    submitBatch();

    // Trigger committing render state again with the next regular
    // drawing command
    mLastKnownRenderMode = RenderMode::CustomDrawing;
    mLastUsedTexture = 0;
    mStateChanged = true;

    // Bind textures

    // We do it in reverse, because that way we end up with unit 0 as
    // the active texture again after the loop, which is the state we want
    // after returning from this function.
    // We could just do another glActiveTexture(GL_TEXTURE0) after the loop
    // but by doing it this way, we save one GL call.
    for (auto i = batch.mTextures.size(); i > 0; --i)
    {
      glActiveTexture(TEXTURE_UNIT_IDS[i - 1]);
      glBindTexture(GL_TEXTURE_2D, batch.mTextures[i - 1]);
    }

    // Use shader
    const auto transform = computeTransformationMatrix(
      mStateStack.back(), currentRenderTargetSize());
    batch.mpShader->use();
    batch.mpShader->setUniform("transform", transform);

    // Submit vertex buffer
    const auto numQuads =
      batch.mVertexBuffer.size() / std::tuple_size<QuadVertices>::value;
    const auto numIndices = GLsizei(numQuads * std::size(QUAD_INDICES));
    assert(numIndices < GLsizei(MAX_BATCH_SIZE));

    glBufferData(
      GL_ARRAY_BUFFER,
      sizeof(float) * batch.mVertexBuffer.size(),
      batch.mVertexBuffer.data(),
      GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mQuadIndicesEbo);
    glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }


  void pushState() { mStateStack.push_back(mStateStack.back()); }


  void popState()
  {
    assert(mStateStack.size() > 1);

    submitBatch();

    mStateChanged = mStateStack.back() != *std::prev(mStateStack.end(), 2);
    mStateStack.pop_back();
  }


  void resetState()
  {
    submitBatch();

    const auto defaultState = State{};

    if (mStateStack.back() != defaultState)
    {
      mStateStack.back() = defaultState;
      mStateChanged = true;
    }
  }


  void setOverlayColor(const base::Color& color)
  {
    updateState(mStateStack.back().mOverlayColor, color);
  }


  void setColorModulation(const base::Color& color)
  {
    updateState(mStateStack.back().mColorModulation, color);
  }


  void setTextureRepeatEnabled(const bool enable)
  {
    updateState(mStateStack.back().mTextureRepeatEnabled, enable);
  }


  void setGlobalTranslation(const base::Vec2& translation)
  {
    const auto glTranslation = glm::vec2{translation.x, translation.y};
    updateState(mStateStack.back().mGlobalTranslation, glTranslation);
  }


  void setGlobalScale(const base::Vec2f& scale)
  {
    const auto glScale = glm::vec2{scale.x, scale.y};
    updateState(mStateStack.back().mGlobalScale, glScale);
  }


  void setClipRect(const std::optional<base::Rect<int>>& clipRect)
  {
    updateState(mStateStack.back().mClipRect, clipRect);
  }


  void setRenderTarget(const TextureId target)
  {
    updateState(mStateStack.back().mRenderTargetTexture, target);
  }

  data::Image grabCurrentFramebuffer()
  {
    submitBatch();

    const auto size = currentRenderTargetSize();
    auto pixels = data::PixelBuffer{size_t(size.width * size.height * 4)};
    glReadPixels(
      0, 0, size.width, size.height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    return data::Image{
      std::move(pixels), size_t(size.width), size_t(size.height)}
      .flipped();
  }


  template <typename StateT>
  void updateState(StateT& state, const StateT& newValue)
  {
    if (state != newValue)
    {
      submitBatch();

      state = newValue;
      mStateChanged = true;
    }
  }


  void swapBuffers()
  {
    assert(mStateStack.back().mRenderTargetTexture == 0);

    submitBatch();
    SDL_GL_SwapWindow(mpWindow);

    const auto actualWindowSize = getSize(mpWindow);
    if (mWindowSize != actualWindowSize)
    {
      mWindowSize = actualWindowSize;
      mStateChanged = true;
    }
  }


  void clear(const base::Color& clearColor)
  {
    commitChangedState();

    const auto glColor = toGlColor(clearColor);
    glClearColor(glColor.r, glColor.g, glColor.b, glColor.a);
    glClear(GL_COLOR_BUFFER_BIT);
  }


  void commitChangedState()
  {
    if (!mStateChanged)
    {
      return;
    }

    const auto& state = mStateStack.back();

    auto transformNeedsUpdate =
      state.mGlobalTranslation != mLastCommittedState.mGlobalTranslation ||
      state.mGlobalScale != mLastCommittedState.mGlobalScale;

    if (
      mRenderMode != mLastKnownRenderMode ||
      state.needsExtendedShader() != mLastCommittedState.needsExtendedShader())
    {
      commitShaderSelection(state);
      transformNeedsUpdate = true;
    }

    if (state.mRenderTargetTexture != mLastCommittedState.mRenderTargetTexture)
    {
      const auto framebufferSize = currentRenderTargetSize();

      commitRenderTarget(state);
      glViewport(0, 0, framebufferSize.width, framebufferSize.height);
      commitClipRect(state, framebufferSize);
      commitVertexAttributeFormat(state);

      transformNeedsUpdate = true;
    }
    else
    {
      if (
        mWindowSize != mLastKnownWindowSize && state.mRenderTargetTexture == 0)
      {
        glViewport(0, 0, mWindowSize.width, mWindowSize.height);
        commitClipRect(state, mWindowSize);
        transformNeedsUpdate = true;
      }
      else if (state.mClipRect != mLastCommittedState.mClipRect)
      {
        commitClipRect(state, currentRenderTargetSize());
      }
    }

    if (mRenderMode == RenderMode::SpriteBatch && state.needsExtendedShader())
    {
      if (state.mColorModulation != mLastCommittedState.mColorModulation)
      {
        mTexturedQuadShader.setUniform(
          "colorModulation", toGlColor(state.mColorModulation));
      }

      if (state.mOverlayColor != mLastCommittedState.mOverlayColor)
      {
        mTexturedQuadShader.setUniform(
          "overlayColor", toGlColor(state.mOverlayColor));
      }

      if (
        state.mTextureRepeatEnabled !=
        mLastCommittedState.mTextureRepeatEnabled)
      {
        mTexturedQuadShader.setUniform(
          "enableRepeat", state.mTextureRepeatEnabled);
      }
    }

    if (transformNeedsUpdate)
    {
      commitTransformationMatrix(state, currentRenderTargetSize());
    }

    mLastCommittedState = state;
    mLastKnownRenderMode = mRenderMode;
    mLastKnownWindowSize = mWindowSize;
    mStateChanged = false;
  }


  void commitRenderTarget(const State& state)
  {
    if (state.mRenderTargetTexture != 0)
    {
      const auto iData = mRenderTargetDict.find(state.mRenderTargetTexture);
      assert(iData != mRenderTargetDict.end());
      glBindFramebuffer(GL_FRAMEBUFFER, iData->second.mFbo);
    }
    else
    {
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
  }


  void commitClipRect(const State& state, const base::Extents& framebufferSize)
  {
    if (state.mClipRect)
    {
      glEnable(GL_SCISSOR_TEST);
      setScissorBox(*state.mClipRect, framebufferSize);
    }
    else
    {
      glDisable(GL_SCISSOR_TEST);
    }
  }


  Shader& shaderToUse(const State& state)
  {
    switch (mRenderMode)
    {
      case RenderMode::SpriteBatch:
        if (state.needsExtendedShader())
        {
          return mTexturedQuadShader;
        }

        return mSimpleTexturedQuadShader;

      case RenderMode::Points:
      case RenderMode::NonTexturedRender:
        return mSolidColorShader;

      default:
        break;
    }

    assert(false);
    return mTexturedQuadShader;
  }


  void commitVertexAttributeFormat(const State& state)
  {
    setVertexLayout(shaderToUse(state).vertexLayout());
  }


  void commitShaderSelection(const State& state)
  {
    auto& shader = shaderToUse(state);
    shader.use();
    setVertexLayout(shader.vertexLayout());

    if (shader.handle() == mTexturedQuadShader.handle())
    {
      mTexturedQuadShader.setUniform(
        "enableRepeat", state.mTextureRepeatEnabled);
      mTexturedQuadShader.setUniform(
        "colorModulation", toGlColor(state.mColorModulation));
      mTexturedQuadShader.setUniform(
        "overlayColor", toGlColor(state.mOverlayColor));
    }
  }


  glm::mat4 computeTransformationMatrix(
    const State& state,
    const base::Extents& framebufferSize)
  {
    const auto projection = glm::ortho(
      0.0f, float(framebufferSize.width), float(framebufferSize.height), 0.0f);
    return glm::scale(
      glm::translate(projection, glm::vec3(state.mGlobalTranslation, 0.0f)),
      glm::vec3(state.mGlobalScale, 1.0f));
  }


  void commitTransformationMatrix(
    const State& state,
    const base::Extents& framebufferSize)
  {
    const auto projectionMatrix =
      computeTransformationMatrix(state, framebufferSize);
    shaderToUse(state).setUniform("transform", projectionMatrix);
  }


  TextureId createRenderTargetTexture(const int width, const int height)
  {
    submitBatch();

    const auto textureHandle =
      createGlTexture(GLsizei(width), GLsizei(height), nullptr);

    GLuint fboHandle;
    glGenFramebuffers(1, &fboHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);
    glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureHandle, 0);

    glBindTexture(GL_TEXTURE_2D, mLastUsedTexture);
    commitRenderTarget(mLastCommittedState);

    mRenderTargetDict.insert({textureHandle, {{width, height}, fboHandle}});

    return textureHandle;
  }


  TextureId createTexture(const data::Image& image)
  {
    submitBatch();

    // OpenGL wants pixel data in bottom-up format, so we need to flip the
    // image
    const auto flippedImage = image.flipped();

    const auto handle = createGlTexture(
      GLsizei(flippedImage.width()),
      GLsizei(flippedImage.height()),
      flippedImage.pixelData().data());
    glBindTexture(GL_TEXTURE_2D, mLastUsedTexture);

    ++mNumTextures;
    return handle;
  }


  void destroyTexture(TextureId texture)
  {
    submitBatch();

    const auto iRenderTarget = mRenderTargetDict.find(texture);
    if (iRenderTarget != mRenderTargetDict.end())
    {
      glDeleteFramebuffers(1, &iRenderTarget->second.mFbo);
      mRenderTargetDict.erase(iRenderTarget);
    }
    else
    {
      --mNumTextures;
    }

    glDeleteTextures(1, &texture);
  }


  void setFilteringEnabled(const TextureId texture, const bool enabled)
  {
    submitBatch();

    glBindTexture(GL_TEXTURE_2D, texture);

    if (enabled)
    {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
    {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    glBindTexture(GL_TEXTURE_2D, mLastUsedTexture);
  }

  void setNativeRepeatEnabled(TextureId texture, bool enabled)
  {
    submitBatch();

    glBindTexture(GL_TEXTURE_2D, texture);

    if (enabled)
    {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    else
    {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    glBindTexture(GL_TEXTURE_2D, mLastUsedTexture);
  }

  base::Size<int> currentRenderTargetSize() const
  {
    const auto& state = mStateStack.back();
    if (state.mRenderTargetTexture != 0)
    {
      const auto iData = mRenderTargetDict.find(state.mRenderTargetTexture);
      assert(iData != mRenderTargetDict.end());
      return iData->second.mSize;
    }

    return mWindowSize;
  }
};


Renderer::Renderer(SDL_Window* pWindow)
  : mpImpl(std::make_unique<Impl>(pWindow))
{
}


Renderer::~Renderer() = default;


void Renderer::setOverlayColor(const base::Color& color)
{
  mpImpl->setOverlayColor(color);
}


void Renderer::setColorModulation(const base::Color& colorModulation)
{
  mpImpl->setColorModulation(colorModulation);
}


void Renderer::setTextureRepeatEnabled(const bool enable)
{
  mpImpl->setTextureRepeatEnabled(enable);
}


void Renderer::drawTexture(
  const TextureId texture,
  const TexCoords& sourceRect,
  const base::Rect<int>& destRect)
{
  mpImpl->drawTexture(texture, sourceRect, destRect);
}


void Renderer::submitBatch()
{
  mpImpl->submitBatch();
}


void Renderer::drawFilledRectangle(
  const base::Rect<int>& rect,
  const base::Color& color)
{
  mpImpl->drawFilledRectangle(rect, color);
}


void Renderer::drawRectangle(
  const base::Rect<int>& rect,
  const base::Color& color)
{
  mpImpl->drawRectangle(rect, color);
}


void Renderer::drawLine(
  const int x1,
  const int y1,
  const int x2,
  const int y2,
  const base::Color& color)
{
  mpImpl->drawLine(x1, y1, x2, y2, color);
}


void Renderer::drawPoint(const base::Vec2& position, const base::Color& color)
{
  mpImpl->drawPoint(position, color);
}


void Renderer::drawCustomQuadBatch(const CustomQuadBatchData& batch)
{
  mpImpl->drawCustomQuadBatch(batch);
}


void Renderer::pushState()
{
  mpImpl->pushState();
}


void Renderer::popState()
{
  mpImpl->popState();
}


void Renderer::resetState()
{
  mpImpl->resetState();
}


void Renderer::setGlobalTranslation(const base::Vec2& translation)
{
  mpImpl->setGlobalTranslation(translation);
}


base::Vec2 Renderer::globalTranslation() const
{
  return base::Vec2{
    static_cast<int>(mpImpl->mStateStack.back().mGlobalTranslation.x),
    static_cast<int>(mpImpl->mStateStack.back().mGlobalTranslation.y)};
}


void Renderer::setGlobalScale(const base::Vec2f& scale)
{
  mpImpl->setGlobalScale(scale);
}


base::Vec2f Renderer::globalScale() const
{
  return {
    mpImpl->mStateStack.back().mGlobalScale.x,
    mpImpl->mStateStack.back().mGlobalScale.y};
}


void Renderer::setClipRect(const std::optional<base::Rect<int>>& clipRect)
{
  mpImpl->setClipRect(clipRect);
}


std::optional<base::Rect<int>> Renderer::clipRect() const
{
  return mpImpl->mStateStack.back().mClipRect;
}


base::Size<int> Renderer::currentRenderTargetSize() const
{
  return mpImpl->currentRenderTargetSize();
}


base::Size<int> Renderer::windowSize() const
{
  return mpImpl->mWindowSize;
}


void Renderer::setRenderTarget(const TextureId target)
{
  mpImpl->setRenderTarget(target);
}


data::Image Renderer::grabCurrentFramebuffer()
{
  return mpImpl->grabCurrentFramebuffer();
}


void Renderer::swapBuffers()
{
  mpImpl->swapBuffers();
}


void Renderer::clear(const base::Color& clearColor)
{
  mpImpl->clear(clearColor);
}


TextureId Renderer::createRenderTargetTexture(const int width, const int height)
{
  return mpImpl->createRenderTargetTexture(width, height);
}


TextureId Renderer::createTexture(const data::Image& image)
{
  return mpImpl->createTexture(image);
}


void Renderer::destroyTexture(TextureId texture)
{
  mpImpl->destroyTexture(texture);
}


void Renderer::setFilteringEnabled(const TextureId texture, const bool enabled)
{
  mpImpl->setFilteringEnabled(texture, enabled);
}

void Renderer::setNativeRepeatEnabled(
  const TextureId texture,
  const bool enabled)
{
  mpImpl->setNativeRepeatEnabled(texture, enabled);
}

} // namespace rigel::renderer
