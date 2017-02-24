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

#include "data/game_traits.hpp"

RIGEL_DISABLE_WARNINGS
#include <glm/gtc/matrix_transform.hpp>
RIGEL_RESTORE_WARNINGS


namespace rigel { namespace engine {

namespace {

// The game's original 320x200 resolution would give us a 16:10 aspect ratio
// when using square pixels, but monitors of the time had a 4:3 aspect ratio,
// and that's what the game's graphics were designed for (very noticeable e.g.
// with the earth in the Apogee logo). It worked out fine back then because
// CRTs can show non-square pixels, but that's not possible with today's
// screens anymore. Therefore, we need to stretch the image slightly before
// actually rendering it. We do that by rendering the game into a 320x200
// render target, and then stretching that onto our logical display which has a
// slightly bigger vertical resolution in order to get a 4:3 aspect ratio.
const auto ASPECT_RATIO_CORRECTED_VIEW_PORT_HEIGHT = 240;

// By making the logical display bigger than the aspect-ratio corrected
// original resolution, we can show text with debug info (e.g. FPS) without it
// taking up too much space or being hard to read.
const auto SCALE_FACTOR = 2;

const auto LOGICAL_DISPLAY_WIDTH =
  data::GameTraits::viewPortWidthPx * SCALE_FACTOR;
const auto LOGICAL_DISPLAY_HEIGHT =
  ASPECT_RATIO_CORRECTED_VIEW_PORT_HEIGHT * SCALE_FACTOR;


const GLushort QUAD_INDICES[] = { 0, 1, 2, 2, 3, 1 };


const auto VERTEX_SOURCE = R"shd(
#version 150

in vec2 position;
in vec2 texCoord;

out vec2 texCoordFrag;

uniform mat4 transform;

void main() {
  gl_Position = transform * vec4(position, 0.0, 1.0);
  texCoordFrag = vec2(texCoord.x, 1.0 - texCoord.y);
}
)shd";

const auto FRAGMENT_SOURCE = R"shd(
#version 150

out vec4 outputColor;

in vec2 texCoordFrag;

uniform sampler2D textureData;
uniform vec3 overlayColor;
uniform float overlayAmount;

uniform vec4 colorModulation;

void main() {
  vec4 baseColor = texture(textureData, texCoordFrag);
  vec3 overlay = overlayColor * overlayAmount;
  outputColor = baseColor * colorModulation + vec4(overlay, 0.0);
}
)shd";

const auto VERTEX_SOURCE_SOLID = R"shd(
#version 150

in vec2 position;

uniform mat4 transform;

void main() {
  gl_Position = transform * vec4(position, 0.0, 1.0);
}
)shd";

const auto FRAGMENT_SOURCE_SOLID = R"shd(
#version 150

out vec4 outputColor;

uniform vec4 color;

void main() {
  outputColor = color;
}
)shd";


base::Rect<int> determineDefaultViewport(SDL_Window* pWindow) {
  // This calculates the required view port coordinates to have aspect-ratio
  // correct scaling from the internal display resolution to the window's
  // actual size.
  int windowWidthInt = 0;
  int windowHeightInt = 0;
  SDL_GetWindowSize(pWindow, &windowWidthInt, &windowHeightInt);

  const auto windowWidth = float(windowWidthInt);
  const auto windowHeight = float(windowHeightInt);

  const auto widthRatio = windowWidth / LOGICAL_DISPLAY_WIDTH;
  const auto heightRatio = windowHeight / LOGICAL_DISPLAY_HEIGHT;

  const auto smallerRatio = std::min(widthRatio, heightRatio);
  const auto usableWidth = LOGICAL_DISPLAY_WIDTH * smallerRatio;
  const auto usableHeight = LOGICAL_DISPLAY_HEIGHT * smallerRatio;

  // Calculate the appropriate offset to center the viewport inside the window
  const auto offsetX = (windowWidth - usableWidth) / 2.0f;
  const auto offsetY = (windowHeight - usableHeight) / 2.0f;

  return {
    {int(offsetX), int(offsetY)},
    {int(usableWidth), int(usableHeight)}
  };
}


void* toAttribOffset(std::uintptr_t offset) {
  return reinterpret_cast<void*>(offset);
}


glm::vec4 toGlColor(const base::Color& color) {
  return glm::vec4{color.r, color.g, color.b, color.a} / 255.0f;
}

}


Renderer::Renderer(SDL_Window* pWindow)
  : mpWindow(pWindow)
  , mTexturedQuadShader(
      VERTEX_SOURCE, FRAGMENT_SOURCE, {"position", "texCoord"})
  , mSolidColorShader(
      VERTEX_SOURCE_SOLID, FRAGMENT_SOURCE_SOLID, {"position"})
  , mLastUsedShader(0)
  , mLastUsedTexture(0)
  , mRenderMode(RenderMode::SpriteBatch)
  , mCurrentFbo(0)
  , mCurrentFramebufferWidth(LOGICAL_DISPLAY_WIDTH)
  , mCurrentFramebufferHeight(LOGICAL_DISPLAY_HEIGHT)
  , mDefaultViewport(determineDefaultViewport(pWindow))
{
  // General configuration
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  SDL_GL_SetSwapInterval(1);

  // Setup a VBO for streaming data to the GPU, stays bound all the time
  glGenVertexArrays(1, &mStreamVao);
  glBindVertexArray(mStreamVao);
  glGenBuffers(1, &mStreamVbo);
  glBindBuffer(GL_ARRAY_BUFFER, mStreamVbo);
  glGenBuffers(1, &mStreamEbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mStreamEbo);

  // One-time setup for textured quad shader
  useShaderIfChanged(mTexturedQuadShader);
  mTexturedQuadShader.setUniform("textureData", 0);

  // Remaining setup
  setRenderMode(RenderMode::SpriteBatch);
  onRenderTargetChanged();
}


Renderer::~Renderer() {
  glDeleteBuffers(1, &mStreamVbo);
  glDeleteVertexArrays(1, &mStreamVao);
}


base::Rect<int> Renderer::fullScreenRect() const {
  return {
    {0, 0},
    {mCurrentFramebufferWidth, mCurrentFramebufferHeight}
  };
}


void Renderer::drawTexture(
  const TextureData& textureData,
  const base::Rect<int>& sourceRect,
  const base::Rect<int>& destRect,
  const base::Color& colorModulation
) {
  if (!isVisible(destRect)) {
    return;
  }

  setRenderModeIfChanged(RenderMode::SpriteBatch);

  const auto colorModulationChanged = colorModulation != mLastColorModulation;
  const auto textureChanged = textureData.mHandle != mLastUsedTexture;

  if (colorModulationChanged || textureChanged) {
    submitBatch();
  }

  if (colorModulationChanged) {
    mTexturedQuadShader.setUniform(
      "colorModulation", toGlColor(colorModulation));
    mLastColorModulation = colorModulation;
  }

  if (textureChanged) {
    glBindTexture(GL_TEXTURE_2D, textureData.mHandle);
    mLastUsedTexture = textureData.mHandle;
  }

  glm::vec2 spriteOffset(
    float(destRect.topLeft.x),
    float(destRect.topLeft.y));
  glm::vec2 spriteScale(
    float(destRect.size.width),
    float(destRect.size.height));
  glm::vec2 texOffset(
    sourceRect.topLeft.x / float(textureData.mWidth),
    sourceRect.topLeft.y / float(textureData.mHeight));
  glm::vec2 texScale(
    sourceRect.size.width / float(textureData.mWidth),
    sourceRect.size.height / float(textureData.mHeight));

  const auto left = spriteOffset.x;
  const auto right = spriteScale.x + spriteOffset.x;
  const auto top = spriteOffset.y;
  const auto bottom = spriteScale.y + spriteOffset.y;
  const auto leftTex = texOffset.x;
  const auto rightTex = texScale.x + texOffset.x;
  const auto topTex = texOffset.y;
  const auto bottomTex = texScale.y + texOffset.y;

  GLfloat vertices[] = {
    left,  bottom,  leftTex,  bottomTex,
    left,  top,     leftTex,  topTex,
    right, bottom,  rightTex, bottomTex,
    right, top,     rightTex, topTex,
  };

  const auto currentVertexCount = GLushort(mBatchData.size() / 4);
  GLushort indices[6];
  std::transform(
    std::cbegin(QUAD_INDICES),
    std::cend(QUAD_INDICES),
    std::begin(indices),
    [currentVertexCount](const GLushort index) {
      return index + currentVertexCount;
    });

  // TODO: Limit maximum batch size
  mBatchData.insert(
    mBatchData.end(), std::cbegin(vertices), std::cend(vertices));
  mBatchIndices.insert(
    mBatchIndices.end(), std::cbegin(indices), std::cend(indices));
}


void Renderer::submitBatch() {
  if (mBatchData.empty()) {
    return;
  }

  glBufferData(
    GL_ARRAY_BUFFER,
    sizeof(float) * mBatchData.size(),
    mBatchData.data(),
    GL_STREAM_DRAW);
  glBufferData(
    GL_ELEMENT_ARRAY_BUFFER,
    sizeof(GLushort) * mBatchIndices.size(),
    mBatchIndices.data(),
    GL_STREAM_DRAW);
  glDrawElements(
    GL_TRIANGLES,
    GLsizei(mBatchIndices.size()),
    GL_UNSIGNED_SHORT,
    0);

  mBatchData.clear();
  mBatchIndices.clear();
}


void Renderer::drawRectangle(
  const base::Rect<int>& rect,
  const base::Color& color
) {
  // Note: No batching for now, drawRectangle is only used for debugging at
  // the moment
  if (!isVisible(rect)) {
    return;
  }

  setRenderModeIfChanged(RenderMode::NonTexturedRender);

  const auto colorVec = glm::vec4{color.r, color.g, color.b, color.a} / 255.0f;
  mSolidColorShader.setUniform("color", colorVec);

  const auto left = float(rect.left());
  const auto right = float(rect.right());
  const auto top = float(rect.top());
  const auto bottom = float(rect.bottom());

  float vertices[] = {
    left, top,
    left, bottom,
    right, bottom,
    right, top,
    left, top
  };

  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
  glDrawArrays(GL_LINE_STRIP, 0, 5);
}


void Renderer::drawLine(
  const int x1,
  const int y1,
  const int x2,
  const int y2,
  const base::Color& color
) {
  // Note: No batching for now, drawLine is only used for debugging at the
  // moment
  setRenderModeIfChanged(RenderMode::NonTexturedRender);

  const auto colorVec = glm::vec4{color.r, color.g, color.b, color.a} / 255.0f;
  mSolidColorShader.setUniform("color", colorVec);

  float vertices[] = {
    float(x1), float(y1),
    float(x2), float(y2)
  };

  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
  glDrawArrays(GL_LINE_STRIP, 0, 2);
}


Renderer::RenderTarget Renderer::currentRenderTarget() const {
  return {
    mCurrentFramebufferWidth,
    mCurrentFramebufferHeight,
    mCurrentFbo
  };
}


void Renderer::setRenderTarget(const RenderTarget& target) {
  if (target.mFbo == mCurrentFbo) {
    return;
  }

  submitBatch();

  if (!target.isDefault()) {
    mCurrentFramebufferWidth = target.mWidth;
    mCurrentFramebufferHeight = target.mHeight;
    mCurrentFbo = target.mFbo;
  } else {
    mCurrentFramebufferWidth = LOGICAL_DISPLAY_WIDTH;
    mCurrentFramebufferHeight = LOGICAL_DISPLAY_HEIGHT;
    mCurrentFbo = 0;
  }

  onRenderTargetChanged();
}


void Renderer::swapBuffers() {
  submitBatch();
  SDL_GL_SwapWindow(mpWindow);
}


void Renderer::clear() {
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
}


void Renderer::setRenderModeIfChanged(const RenderMode mode) {
  if (mRenderMode != mode) {
    setRenderMode(mode);
  }
}


void Renderer::setRenderMode(const RenderMode mode) {
  submitBatch();

  switch (mode) {
    case RenderMode::SpriteBatch:
      useShaderIfChanged(mTexturedQuadShader);
      glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(float) * 4,
        toAttribOffset(0));
      glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(float) * 4,
        toAttribOffset(2 * sizeof(float)));
      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);
      break;

    case RenderMode::NonTexturedRender:
      useShaderIfChanged(mSolidColorShader);
      glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        0,
        toAttribOffset(0));
      glEnableVertexAttribArray(0);
      glDisableVertexAttribArray(1);
      break;
  }

  mRenderMode = mode;
}


Renderer::RenderTargetHandles Renderer::createRenderTargetTexture(
  const int width,
  const int height
) {
  const auto textureHandle =
    createGlTexture(GLsizei(width), GLsizei(height), nullptr);
  glBindTexture(GL_TEXTURE_2D, textureHandle);

  GLuint fboHandle;
  glGenFramebuffers(1, &fboHandle);
  glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);
  glFramebufferTexture2D(
    GL_FRAMEBUFFER,
    GL_COLOR_ATTACHMENT0,
    GL_TEXTURE_2D,
    textureHandle,
    0);

  glBindFramebuffer(GL_FRAMEBUFFER, mCurrentFbo);
  glBindTexture(GL_TEXTURE_2D, mLastUsedTexture);

  return {textureHandle, fboHandle};
}


auto Renderer::createTexture(const data::Image& image) -> TextureData {
  // OpenGL wants pixel data in bottom-up format, so transform it accordingly
  std::vector<std::uint8_t> pixelData;
  pixelData.resize(image.width() * image.height() * 4);
  for (std::size_t y = 0; y < image.height(); ++y) {
    const auto sourceRow = image.height() - (y + 1);
    const auto yOffsetSource = image.width() * sourceRow;
    const auto yOffset = y * image.width() * 4;

    for (std::size_t x = 0; x < image.width(); ++x) {
      const auto& pixel = image.pixelData()[x + yOffsetSource];
      pixelData[x*4 +     yOffset] = pixel.r;
      pixelData[x*4 + 1 + yOffset] = pixel.g;
      pixelData[x*4 + 2 + yOffset] = pixel.b;
      pixelData[x*4 + 3 + yOffset] = pixel.a;
    }
  }

  auto handle = createGlTexture(
    GLsizei(image.width()),
    GLsizei(image.height()),
    pixelData.data());
  return {int(image.width()), int(image.height()), handle};
}


GLuint Renderer::createGlTexture(
  const GLsizei width,
  const GLsizei height,
  const GLvoid* const pData
) {
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
  glBindTexture(GL_TEXTURE_2D, mLastUsedTexture);

  return handle;
}



void Renderer::useShaderIfChanged(Shader& shader) {
  if (shader.handle() != mLastUsedShader) {
    shader.use();
    mLastUsedShader = shader.handle();
  }
}


bool Renderer::isVisible(const base::Rect<int>& rect) const {
  return rect.intersects(fullScreenRect());
}


void Renderer::onRenderTargetChanged() {
  glBindFramebuffer(GL_FRAMEBUFFER, mCurrentFbo);
  if (mCurrentFbo == 0) {
    glViewport(
      mDefaultViewport.topLeft.x,
      mDefaultViewport.topLeft.y,
      mDefaultViewport.size.width,
      mDefaultViewport.size.height);
  } else {
    glViewport(0, 0, mCurrentFramebufferWidth, mCurrentFramebufferHeight);
  }

  mProjectionMatrix = glm::ortho(
    0.0f,
    float(mCurrentFramebufferWidth),
    float(mCurrentFramebufferHeight),
    0.0f);

  // TODO: Update only for currently used shader, do it lazily for the other
  // one when switching render mode.
  useShaderIfChanged(mTexturedQuadShader);
  mTexturedQuadShader.setUniform("transform", mProjectionMatrix);
  useShaderIfChanged(mSolidColorShader);
  mSolidColorShader.setUniform("transform", mProjectionMatrix);

  // Need to re-configure vertex attrib state after switching shaders.
  setRenderMode(mRenderMode);
}

}}
