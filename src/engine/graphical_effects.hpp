/* Copyright (C) 2022, Nikolai Wuttke. All rights reserved.
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

#include "renderer/custom_quad_batch.hpp"
#include "renderer/shader.hpp"
#include "renderer/texture.hpp"


namespace rigel::data
{
struct GameOptions;
}

namespace rigel::renderer
{
class Renderer;
}

namespace rigel::engine
{

struct WaterEffectArea
{
  base::Rect<int> mArea;
  bool mIsAnimated;
};


class SpecialEffectsRenderer
{
public:
  SpecialEffectsRenderer(
    renderer::Renderer* pRenderer,
    const data::GameOptions& options);

  void rebuildBackgroundBuffer(const data::GameOptions& options);
  [[nodiscard]] auto bindBackgroundBuffer() { return mBackgroundBuffer.bind(); }
  void drawBackgroundBuffer();

  void drawWaterEffect(
    base::ArrayView<WaterEffectArea> areas,
    int surfaceAnimationStep);

private:
  renderer::Renderer* mpRenderer;
  renderer::Shader mShader;
  renderer::CustomQuadBatch mBatch;
  renderer::RenderTargetTexture mBackgroundBuffer;
  renderer::Texture mWaterSurfaceAnimTexture;
  renderer::Texture mWaterEffectPaletteTexture;
  renderer::MonoTexture mRgbToPaletteIndexMap;
};

} // namespace rigel::engine
