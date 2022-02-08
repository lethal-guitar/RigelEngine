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

#include "map_renderer.hpp"

#include "base/math_utils.hpp"
#include "data/game_traits.hpp"
#include "data/unit_conversions.hpp"

#include <cfenv>
#include <iostream>


namespace rigel::engine
{

using namespace data;

using data::map::BackdropScrollMode;


namespace
{

const auto ANIM_STATES = 4;
const auto FAST_ANIM_FRAME_DELAY = 1;
const auto SLOW_ANIM_FRAME_DELAY = 2;
const auto PARALLAX_FACTOR = 4.0f;
const auto AUTO_SCROLL_PX_PER_SECOND_HORIZONTAL = 30.0f;
const auto AUTO_SCROLL_PX_PER_SECOND_VERTICAL = 60.0f;


base::Vec2f backdropOffset(
  const base::Vec2f& cameraPosition,
  const BackdropScrollMode scrollMode,
  const float backdropAutoScrollOffset)
{
  const auto parallaxBoth = scrollMode == BackdropScrollMode::ParallaxBoth;
  const auto parallaxHorizontal =
    scrollMode == BackdropScrollMode::ParallaxHorizontal || parallaxBoth;

  const auto autoScrollX = scrollMode == BackdropScrollMode::AutoHorizontal;
  const auto autoScrollY = scrollMode == BackdropScrollMode::AutoVertical;

  if (parallaxHorizontal || parallaxBoth)
  {
    return {
      parallaxHorizontal ? cameraPosition.x * PARALLAX_FACTOR : 0.0f,
      parallaxBoth ? cameraPosition.y * PARALLAX_FACTOR : 0.0f};
  }
  else if (autoScrollX || autoScrollY)
  {
    if (autoScrollX)
    {
      return {backdropAutoScrollOffset, 0.0f};
    }
    else
    {
      return {
        0.0f, float(GameTraits::viewportHeightPx) - backdropAutoScrollOffset};
    }
  }

  return {};
}


constexpr auto TILE_SET_IMAGE_LOGICAL_SIZE = base::Extents{
  tilesToPixels(data::GameTraits::CZone::tileSetImageWidth),
  tilesToPixels(data::GameTraits::CZone::tileSetImageHeight)};

} // namespace


MapRenderer::MapRenderer(
  renderer::Renderer* pRenderer,
  const data::map::Map* pMap,
  MapRenderData&& renderData)
  : mpRenderer(pRenderer)
  , mpMap(pMap)
  , mTileSetTexture(
      renderer::Texture(pRenderer, renderData.mTileSetImage),
      TILE_SET_IMAGE_LOGICAL_SIZE,
      pRenderer)
  , mBackdropTexture(mpRenderer, renderData.mBackdropImage)
  , mScrollMode(renderData.mBackdropScrollMode)
{
  if (renderData.mSecondaryBackdropImage)
  {
    mAlternativeBackdropTexture =
      renderer::Texture(mpRenderer, *renderData.mSecondaryBackdropImage);
  }
}


void MapRenderer::synchronizeTo(const MapRenderer& other)
{
  mBackdropAutoScrollOffset = other.mBackdropAutoScrollOffset;
  mElapsedFrames = other.mElapsedFrames;
}


bool MapRenderer::hasHighResReplacements() const
{
  return mBackdropTexture.width() > data::GameTraits::viewportWidthPx ||
    mBackdropTexture.height() > data::GameTraits::viewportHeightPx ||
    mAlternativeBackdropTexture.width() > data::GameTraits::viewportWidthPx ||
    mAlternativeBackdropTexture.height() > data::GameTraits::viewportHeightPx ||
    mTileSetTexture.isHighRes();
}


void MapRenderer::switchBackdrops()
{
  std::swap(mBackdropTexture, mAlternativeBackdropTexture);
}


void MapRenderer::renderBackground(
  const base::Vec2& sectionStart,
  const base::Extents& sectionSize) const
{
  renderMapTiles(sectionStart, sectionSize, DrawMode::Background);
}


void MapRenderer::renderForeground(
  const base::Vec2& sectionStart,
  const base::Extents& sectionSize) const
{
  renderMapTiles(sectionStart, sectionSize, DrawMode::Foreground);
}


renderer::TexCoords MapRenderer::calculateBackdropTexCoords(
  const base::Vec2f& cameraPosition,
  const base::Extents& viewportSize) const
{
  // This function determines the texture coordinates we need to use for
  // drawing the backdrop into the current view port (which could be
  // wide-screen or classic), while taking the current backdrop offset (either
  // from parallax, or automatic scrolling) into account. Essentially, we want
  // to determine the rectangle defining the section of the backdrop graphic
  // that we need to display. The rectangle might be wider than the backdrop
  // itself, which then causes the backdrop texture to wrap around and repeat
  // thanks to texture repeat being enabled when drawing the backdrop.
  //
  // The logic is somewhat complicated, because it needs to work for any
  // background image resolution, and any background image aspect ratio - we
  // want to support things like wide backgrounds. For original artwork and
  // replacements in the same resolution, we need to take aspect ratio
  // correction into account, but only when doing per-element upscaling.
  // For higher resolution replacements, we want to maintain the artwork's
  // aspect ratio, and we want to display it correctly even if the aspect
  // ratio of the current screen resolution is different (e.g., showing a
  // 16:9 background image on a 16:10 screen).
  //
  // We need to determine how to map the viewport rectangle (which is
  // not the entire screen) into the background image's texture space.
  // The idea is that we always scale the background vertically to match
  // the current render target size, and then work out the width from there.

  // Let's start with determining the scale factors.
  const auto windowWidth = float(mpRenderer->currentRenderTargetSize().width);
  const auto windowHeight = float(mpRenderer->currentRenderTargetSize().height);
  const auto scaleY = windowHeight / mBackdropTexture.height();

  // Now that we know the scaling factor, we can determine the ratio between
  // the screen's width and the scaled background's width. Here we need to
  // take aspect ratio correction into account, in case we are working with
  // original art resolution and per-element upscaling.
  const auto isOriginalSize =
    mBackdropTexture.width() == GameTraits::viewportWidthPx &&
    mBackdropTexture.height() == GameTraits::viewportHeightPx;
  const auto needsAspectRatioCorrection =
    isOriginalSize && windowHeight != GameTraits::viewportHeightPx;
  const auto correctionFactor = needsAspectRatioCorrection
    ? data::GameTraits::aspectCorrectionStretchFactor
    : 1.0f;
  const auto scaleX = scaleY / correctionFactor;

  // We can now determine the width of the background when applying scaling,
  // and based on that, we can determine the "remapping factor" that we
  // need to apply in order to avoid horizontal stretching.
  // Basically, this is a measure of how much wider/narrower the background
  // image is in relation to the screen.
  const auto scaledWidth = scaleX * mBackdropTexture.width();
  const auto remappingFactor = windowWidth / scaledWidth;

  // Then, we need to know what portion of the full screen is occupied by
  // the view port. Basically, what percentage of the background size can we
  // use to match the dimensions of the destination rectangle used for
  // drawing, which is equal in size to the current view port.
  const auto targetWidth = float(data::tilesToPixels(viewportSize.width)) *
    mpRenderer->globalScale().x;
  const auto targetHeight = float(data::tilesToPixels(viewportSize.height)) *
    mpRenderer->globalScale().y;
  const auto visibleTargetPortionX = targetWidth / windowWidth;
  const auto visibleTargetPortionY = targetHeight / windowHeight;

  // Finally, compute the offset, and map it into the coordinate system of
  // the backdrop texture.
  const auto offset =
    backdropOffset(cameraPosition, mScrollMode, mBackdropAutoScrollOffset);

  // With all that, we can now define our rectangle in texture coordinate
  // space (i.e. from 0..1 on both axes).
  // In auto-scroll mode, the offset is already in the coordinate system of
  // the backdrop texture, so we don't need to remap.
  const auto isAutoScrolling =
    mScrollMode == BackdropScrollMode::AutoHorizontal ||
    mScrollMode == BackdropScrollMode::AutoVertical;
  const auto offsetX = isAutoScrolling
    ? offset.x
    : offset.x * mpRenderer->globalScale().x / scaleX;
  const auto offsetY = isAutoScrolling
    ? offset.y
    : offset.y * mpRenderer->globalScale().y / scaleY;

  const auto left = offsetX / mBackdropTexture.width();
  const auto top = offsetY / mBackdropTexture.height();
  const auto right = left + visibleTargetPortionX * remappingFactor;
  const auto bottom = top + visibleTargetPortionY;

  return renderer::TexCoords{left, top, right, bottom};
}


void MapRenderer::renderBackdrop(
  const base::Vec2f& cameraPosition,
  const base::Extents& viewportSize) const
{
  const auto saved = renderer::saveState(mpRenderer);
  mpRenderer->setTextureRepeatEnabled(true);
  mpRenderer->drawTexture(
    mBackdropTexture.data(),
    calculateBackdropTexCoords(cameraPosition, viewportSize),
    {{}, data::tileExtentsToPixelExtents(viewportSize)});
}


void MapRenderer::renderMapTiles(
  const base::Vec2& sectionStart,
  const base::Extents& sectionSize,
  const DrawMode drawMode) const
{
  for (int layer = 0; layer < 2; ++layer)
  {
    for (int y = 0; y < sectionSize.height; ++y)
    {
      for (int x = 0; x < sectionSize.width; ++x)
      {
        const auto col = x + sectionStart.x;
        const auto row = y + sectionStart.y;
        if (col >= mpMap->width() || row >= mpMap->height())
        {
          continue;
        }

        const auto tileIndex = mpMap->tileAt(layer, col, row);
        const auto isForeground =
          mpMap->attributeDict().attributes(tileIndex).isForeGround();
        const auto shouldRenderForeground = drawMode == DrawMode::Foreground;

        if (isForeground != shouldRenderForeground)
        {
          continue;
        }

        renderTile(tileIndex, x, y);
      }
    }
  }

  if (drawMode == DrawMode::Foreground)
  {
    const auto outOfBoundsTilesX =
      std::max(0, sectionStart.x + sectionSize.width - mpMap->width());
    const auto outOfBoundsTilesY =
      std::max(0, sectionStart.y + sectionSize.height - mpMap->height());

    if (outOfBoundsTilesX)
    {
      const auto outOfBoundsStart = sectionSize.width - outOfBoundsTilesX;
      const auto outOfBoundsRect = base::Rect<int>{
        data::tileVectorToPixelVector({outOfBoundsStart, 0}),
        data::tileExtentsToPixelExtents(
          {outOfBoundsTilesX, sectionSize.height}),
      };

      mpRenderer->drawFilledRectangle(outOfBoundsRect, {0, 0, 0, 255});
    }

    if (outOfBoundsTilesY)
    {
      const auto outOfBoundsStart = sectionSize.height - outOfBoundsTilesY;
      const auto outOfBoundsRect = base::Rect<int>{
        data::tileVectorToPixelVector({0, outOfBoundsStart}),
        data::tileExtentsToPixelExtents({sectionSize.width, outOfBoundsTilesY}),
      };

      mpRenderer->drawFilledRectangle(outOfBoundsRect, {0, 0, 0, 255});
    }
  }
}


void MapRenderer::updateAnimatedMapTiles()
{
  ++mElapsedFrames;
}


void MapRenderer::updateBackdropAutoScrolling(const engine::TimeDelta dt)
{
  const auto scrollSpeed = std::invoke([&]() {
    const auto scale =
      float(mBackdropTexture.height()) / GameTraits::viewportHeightPx;

    if (mScrollMode == BackdropScrollMode::AutoHorizontal)
    {
      return AUTO_SCROLL_PX_PER_SECOND_HORIZONTAL * scale;
    }
    else if (mScrollMode == BackdropScrollMode::AutoVertical)
    {
      return AUTO_SCROLL_PX_PER_SECOND_VERTICAL * scale;
    }
    else
    {
      return 0.0f;
    }
  });

  const auto maxOffset = std::invoke([&]() {
    if (mScrollMode == BackdropScrollMode::AutoHorizontal)
    {
      return float(mBackdropTexture.width());
    }
    else if (mScrollMode == BackdropScrollMode::AutoVertical)
    {
      return float(mBackdropTexture.height());
    }
    else
    {
      return 1.0f;
    }
  });

  mBackdropAutoScrollOffset += float(dt * scrollSpeed);
  mBackdropAutoScrollOffset = std::fmod(mBackdropAutoScrollOffset, maxOffset);
}


void MapRenderer::renderSingleTile(
  const data::map::TileIndex index,
  const base::Vec2& pixelPosition) const
{
  // TODO: Can we reduce duplication with renderTile()?

  // Tile index 0 is used to represent a transparent tile, i.e. the backdrop
  // should be visible. Therefore, don't draw if the index is 0.
  if (index != 0)
  {
    const auto tileIndexToDraw = animatedTileIndex(index);
    mTileSetTexture.renderTileAtPixelPos(tileIndexToDraw, pixelPosition);
  }
}


void MapRenderer::renderTile(
  const data::map::TileIndex tileIndex,
  const int x,
  const int y) const
{
  // Tile index 0 is used to represent a transparent tile, i.e. the backdrop
  // should be visible. Therefore, don't draw if the index is 0.
  if (tileIndex != 0)
  {
    const auto tileIndexToDraw = animatedTileIndex(tileIndex);
    mTileSetTexture.renderTile(tileIndexToDraw, x, y);
  }
}


map::TileIndex
  MapRenderer::animatedTileIndex(const map::TileIndex tileIndex) const
{
  if (mpMap->attributeDict().attributes(tileIndex).isAnimated())
  {
    const auto fastAnimOffset =
      (mElapsedFrames / FAST_ANIM_FRAME_DELAY) % ANIM_STATES;
    const auto slowAnimOffset =
      (mElapsedFrames / SLOW_ANIM_FRAME_DELAY) % ANIM_STATES;

    const auto isFastAnim =
      mpMap->attributeDict().attributes(tileIndex).isFastAnimation();
    return tileIndex + (isFastAnim ? fastAnimOffset : slowAnimOffset);
  }
  else
  {
    return tileIndex;
  }
}

} // namespace rigel::engine
