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
#include "base/static_vector.hpp"
#include "data/game_traits.hpp"
#include "data/unit_conversions.hpp"
#include "renderer/vertex_buffer_utils.hpp"
#include "renderer/viewport_utils.hpp"

#include <cfenv>
#include <iostream>


namespace rigel::engine
{

using namespace data;

using data::map::BackdropScrollMode;


namespace
{

auto unpack(const PackedTileData data)
{
  return std::tuple{data & 0xFFFF, (data & 0xFFFF0000) >> 16};
}


const auto ANIM_STATES = 4;
const auto FAST_ANIM_FRAME_DELAY = 1;
const auto SLOW_ANIM_FRAME_DELAY = 2;
const auto PARALLAX_FACTOR = 4.0f;
const auto AUTO_SCROLL_PX_PER_SECOND_HORIZONTAL = 30.0f;
const auto AUTO_SCROLL_PX_PER_SECOND_VERTICAL = 60.0f;
const auto MAX_BLOCKS = 32;


struct TileBlockData
{
  std::vector<float> mVertices;
  std::vector<AnimatedTile> mAnimatedTiles;
};


void buildBlock(
  const int blockX,
  const int blockY,
  TileRenderData& renderData,
  const data::map::Map& map,
  const TiledTexture& tileSetTexture,
  renderer::Renderer* pRenderer)
{
  const auto blockStartX = blockX * BLOCK_SIZE;
  const auto blockEndX = (blockX + 1) * BLOCK_SIZE;
  const auto blockStartY = blockY * BLOCK_SIZE;
  const auto blockEndY = (blockY + 1) * BLOCK_SIZE;

  auto blockData = std::array<TileBlockData, 2>{};


  auto addToBlock =
    [&](const map::TileIndex tileIndex, const int x, const int y) {
      if (tileIndex == 0)
      {
        return;
      }

      const auto isForeground =
        map.attributeDict().attributes(tileIndex).isForeGround();
      const auto targetIndex = isForeground ? 1 : 0;
      auto& targetBlockData = blockData[targetIndex];

      const auto isAnimated =
        map.attributeDict().attributes(tileIndex).isAnimated();
      if (isAnimated)
      {
        targetBlockData.mAnimatedTiles.push_back({{x, y}, tileIndex});
      }
      else
      {
        const auto vertices = tileSetTexture.generateVertices(tileIndex, x, y);
        targetBlockData.mVertices.insert(
          targetBlockData.mVertices.end(), vertices.begin(), vertices.end());
      }
    };


  // Fill block data with tiles
  for (auto y = blockStartY; y < blockEndY && y < map.height(); ++y)
  {
    for (auto x = blockStartX; x < blockEndX && x < map.width(); ++x)
    {
      addToBlock(map.tileAt(0, x, y), x, y);
      addToBlock(map.tileAt(1, x, y), x, y);
    }
  }

  // Commit block data
  for (auto layer = 0; layer < 2; ++layer)
  {
    auto& data = blockData[layer];

    auto buffer = data.mVertices.empty()
      ? renderer::INVALID_VERTEX_BUFFER_ID
      : pRenderer->createVertexBuffer(data.mVertices);

    renderData.mLayers[layer].push_back(
      {buffer, std::move(data.mAnimatedTiles)});
  }
}


TileRenderData buildRenderData(
  const data::map::Map& map,
  const TiledTexture& tileSetTexture,
  renderer::Renderer* pRenderer)
{
  const auto numBlocksX = base::integerDivCeil(map.width(), BLOCK_SIZE);
  const auto numBlocksY = base::integerDivCeil(map.height(), BLOCK_SIZE);

  TileRenderData result{{numBlocksX, numBlocksY}, pRenderer};

  for (auto blockY = 0; blockY < numBlocksY; ++blockY)
  {
    for (auto blockX = 0; blockX < numBlocksX; ++blockX)
    {
      buildBlock(blockX, blockY, result, map, tileSetTexture, pRenderer);
    }
  }

  return result;
}


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
        cameraPosition.x * PARALLAX_FACTOR,
        float(GameTraits::viewportHeightPx) - backdropAutoScrollOffset};
    }
  }

  return {};
}


constexpr auto TILE_SET_IMAGE_LOGICAL_SIZE = base::Size{
  tilesToPixels(data::GameTraits::CZone::tileSetImageWidth),
  tilesToPixels(data::GameTraits::CZone::tileSetImageHeight)};

} // namespace


std::vector<PackedTileData>
  copyMapData(const base::Rect<int>& section, const data::map::Map& map)
{
  auto result = std::vector<PackedTileData>{};
  result.resize(section.size.width * section.size.height);

  for (auto layer = 0; layer < 2; ++layer)
  {
    auto iMapData = result.begin();
    for (auto y = section.top(); y <= section.bottom(); ++y)
    {
      for (auto x = section.left(); x <= section.right(); ++x)
      {
        const auto tileValue = map.tileAt(layer, x, y);
        *iMapData |= tileValue << (layer * 16);
        ++iMapData;
      }
    }
  }

  return result;
}


TileRenderData::TileRenderData(base::Size size, renderer::Renderer* pRenderer)
  : mSize(size)
  , mpRenderer(pRenderer)
{
}


TileRenderData::~TileRenderData()
{
  for (const auto& layer : mLayers)
  {
    for (const auto& block : layer)
    {
      if (block.mTilesBuffer != renderer::INVALID_VERTEX_BUFFER_ID)
      {
        mpRenderer->destroyVertexBuffer(block.mTilesBuffer);
      }
    }
  }
}


MapRenderer::MapRenderer(
  renderer::Renderer* pRenderer,
  const data::map::Map& map,
  const data::map::TileAttributeDict* pTileAttributes,
  MapRenderData&& renderData)
  : mpRenderer(pRenderer)
  , mpTileAttributes(pTileAttributes)
  , mTileSetTexture(
      renderer::Texture(pRenderer, renderData.mTileSetImage),
      TILE_SET_IMAGE_LOGICAL_SIZE,
      pRenderer)
  , mBackdropTexture(mpRenderer, renderData.mBackdropImage)
  , mRenderData(buildRenderData(map, mTileSetTexture, pRenderer))
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
  const base::Size& sectionSize) const
{
  renderMapTiles(sectionStart, sectionSize, DrawMode::Background);
}


void MapRenderer::renderForeground(
  const base::Vec2& sectionStart,
  const base::Size& sectionSize) const
{
  renderMapTiles(sectionStart, sectionSize, DrawMode::Foreground);
}


renderer::TexCoords MapRenderer::calculateBackdropTexCoords(
  const base::Vec2f& cameraPosition,
  const base::Size& viewportSize) const
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
  const base::Size& viewportSize) const
{
  const auto saved = renderer::saveState(mpRenderer);
  mpRenderer->setTextureRepeatEnabled(true);
  mpRenderer->drawTexture(
    mBackdropTexture.data(),
    calculateBackdropTexCoords(cameraPosition, viewportSize),
    {{}, data::tilesToPixels(viewportSize)});
}


void MapRenderer::renderMapTiles(
  const base::Vec2& sectionStart,
  const base::Size& sectionSize,
  const DrawMode drawMode) const
{
  const auto blockX = sectionStart.x / BLOCK_SIZE;
  const auto blockY = sectionStart.y / BLOCK_SIZE;
  const auto offsetInBlockX = sectionStart.x % BLOCK_SIZE;
  const auto offsetInBlockY = sectionStart.y % BLOCK_SIZE;
  const auto numBlocksX = base::integerDivCeil(sectionSize.width, BLOCK_SIZE) +
    std::min(offsetInBlockX, 1);
  const auto numBlocksY = base::integerDivCeil(sectionSize.height, BLOCK_SIZE) +
    std::min(offsetInBlockY, 1);

  auto forEachVisibleBlock = [&](auto&& func) {
    const auto layerIndex = static_cast<size_t>(drawMode);

    for (auto y = blockY;
         y < std::min(blockY + numBlocksY, mRenderData.mSize.height);
         ++y)
    {
      for (auto x = blockX;
           x < std::min(blockX + numBlocksX, mRenderData.mSize.width);
           ++x)
      {
        const auto blockIndex = x + y * mRenderData.mSize.width;
        func(mRenderData.mLayers[layerIndex][blockIndex]);
      }
    }
  };


  base::static_vector<renderer::VertexBufferId, MAX_BLOCKS> blocksToRender;

  forEachVisibleBlock([&](const TileBlock& block) {
    if (block.mTilesBuffer != renderer::INVALID_VERTEX_BUFFER_ID)
    {
      blocksToRender.push_back(block.mTilesBuffer);
    }
  });

  const auto translation = data::tilesToPixels(sectionStart) * -1;

  const auto saved = renderer::saveState(mpRenderer);
  renderer::setLocalTranslation(mpRenderer, translation);

  mpRenderer->submitVertexBuffers(blocksToRender, mTileSetTexture.textureId());

  forEachVisibleBlock([&](const TileBlock& block) {
    for (const auto& animated : block.mAnimatedTiles)
    {
      const auto tileIndexToDraw = animatedTileIndex(animated.mIndex);
      mTileSetTexture.renderTile(
        tileIndexToDraw, animated.mPosition.x, animated.mPosition.y);
    }
  });
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
  // Tile index 0 is used to represent a transparent tile, i.e. the backdrop
  // should be visible. Therefore, don't draw if the index is 0.
  if (index != 0)
  {
    const auto tileIndexToDraw = animatedTileIndex(index);
    mTileSetTexture.renderTileAtPixelPos(tileIndexToDraw, pixelPosition);
  }
}


void MapRenderer::renderDynamicSection(
  const data::map::Map& map,
  const base::Rect<int>& coordinates,
  const base::Vec2& pixelPosition,
  const DrawMode drawMode) const
{
  for (auto layer = 0; layer < 2; ++layer)
  {
    for (auto y = coordinates.top();
         y < coordinates.top() + coordinates.size.height;
         ++y)
    {
      for (auto x = coordinates.left();
           x < coordinates.left() + coordinates.size.width;
           ++x)
      {
        if (x >= map.width() || y >= map.height())
        {
          continue;
        }

        const auto tileIndex = map.tileAt(layer, x, y);
        const auto isForeground =
          mpTileAttributes->attributes(tileIndex).isForeGround();
        const auto shouldRenderForeground = drawMode == DrawMode::Foreground;
        if (isForeground != shouldRenderForeground)
        {
          continue;
        }

        const auto offsetInSection =
          data::tilesToPixels(base::Vec2{x, y} - coordinates.topLeft);
        renderSingleTile(tileIndex, pixelPosition + offsetInSection);
      }
    }
  }
}


void MapRenderer::renderCachedSection(
  const base::Vec2& pixelPosition,
  base::ArrayView<PackedTileData> data,
  const int width,
  const DrawMode drawMode) const
{
  auto drawTile = [&](const auto tileIndex, const base::Vec2& screenPos) {
    const auto isForeground =
      mpTileAttributes->attributes(tileIndex).isForeGround();
    const auto shouldRenderForeground = drawMode == DrawMode::Foreground;
    if (isForeground == shouldRenderForeground)
    {
      renderSingleTile(tileIndex, screenPos);
    }
  };


  const auto height = int(data.size()) / width;

  auto iMapData = data.begin();
  for (auto y = 0; y < height; ++y)
  {
    for (auto x = 0; x < width; ++x)
    {
      const auto screenPos =
        data::tilesToPixels(base::Vec2{x, y}) + pixelPosition;

      const auto [layer0, layer1] = unpack(*iMapData);
      drawTile(layer0, screenPos);
      drawTile(layer1, screenPos);

      ++iMapData;
    }
  }
}


map::TileIndex
  MapRenderer::animatedTileIndex(const map::TileIndex tileIndex) const
{
  if (mpTileAttributes->attributes(tileIndex).isAnimated())
  {
    const auto fastAnimOffset =
      (mElapsedFrames / FAST_ANIM_FRAME_DELAY) % ANIM_STATES;
    const auto slowAnimOffset =
      (mElapsedFrames / SLOW_ANIM_FRAME_DELAY) % ANIM_STATES;

    const auto isFastAnim =
      mpTileAttributes->attributes(tileIndex).isFastAnimation();
    return tileIndex + (isFastAnim ? fastAnimOffset : slowAnimOffset);
  }
  else
  {
    return tileIndex;
  }
}

} // namespace rigel::engine
