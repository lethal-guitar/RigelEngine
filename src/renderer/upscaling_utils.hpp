/* Copyright (C) 2019, Nikolai Wuttke. All rights reserved.
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
#include "renderer/texture.hpp"


namespace rigel::data
{
struct GameOptions;
}


namespace rigel::renderer
{

class Renderer;

struct ViewPortInfo
{
  base::Vector mOffset;
  base::Size<int> mSize;
  base::Vec2T<float> mScale;
};


struct WidescreenViewPortInfo
{
  int mWidthTiles;
  int mWidthPx;
  int mLeftPaddingPx;
};


ViewPortInfo determineViewPort(const Renderer* pRenderer);

/** Returns true if wide-screen mode is feasible for the current window size.
 *
 * If the current window size has an aspect ratio that is less than 4:3, there
 * is no point in using wide-screen mode.
 */
bool canUseWidescreenMode(const Renderer* pRenderer);

WidescreenViewPortInfo determineWidescreenViewPort(const Renderer* pRenderer);

base::Vector scaleVec(const base::Vector& vec, const base::Vec2T<float>& scale);
base::Extents
  scaleSize(const base::Extents& size, const base::Vec2T<float>& scale);

RenderTargetTexture createFullscreenRenderTarget(
  Renderer* pRenderer,
  const data::GameOptions& options);

} // namespace rigel::renderer
