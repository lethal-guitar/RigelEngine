/* Copyright (C) 2020, Nikolai Wuttke. All rights reserved.
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

#include "texture_atlas.hpp"

#include "base/warnings.hpp"
#include "renderer/renderer.hpp"

RIGEL_DISABLE_WARNINGS
#include <stb_rect_pack.h>
RIGEL_RESTORE_WARNINGS

#include <algorithm>
#include <stdexcept>


namespace rigel::renderer
{

namespace
{

constexpr auto ATLAS_WIDTH = 2048;
constexpr auto ATLAS_HEIGHT = 1024;

} // namespace


TextureAtlas::TextureAtlas(
  Renderer* pRenderer,
  const std::vector<data::Image>& images)
  : mpRenderer(pRenderer)
{
  mAtlasMap.resize(images.size());

  std::vector<stbrp_rect> rects;
  rects.reserve(images.size());

  auto index = 0;
  for (const auto& image : images)
  {
    rects.push_back(stbrp_rect{
      index,
      static_cast<stbrp_coord>(image.width()),
      static_cast<stbrp_coord>(image.height()),
      0,
      0,
      0});
    ++index;
  }

  stbrp_context context;
  std::vector<stbrp_node> nodes;
  nodes.resize(ATLAS_WIDTH);

  do
  {
    stbrp_init_target(
      &context,
      ATLAS_WIDTH,
      ATLAS_HEIGHT,
      nodes.data(),
      static_cast<int>(nodes.size()));

    const auto result =
      stbrp_pack_rects(&context, rects.data(), static_cast<int>(rects.size()));

    // Not all images might fit into the first texture. If that happens, we
    // separate off those that didn't fit, and then do another round, then
    // repeat again if needed, until we've managed to place all images.
    // We do this by shifting all successfully packed images to the end of
    // the vector, so that we can easily delete them after processing them,
    // with the unpacked images remaining.
    const auto iFirstPacked = result
      ? rects.begin()
      : std::partition(rects.begin(), rects.end(), [](const stbrp_rect& rect) {
          return !rect.was_packed;
        });

    if (!result && iFirstPacked == rects.end())
    {
      // If not even a single rect could be packed, we give up
      throw std::runtime_error{"Failed to build texture atlas"};
    }

    data::Image atlas{
      static_cast<size_t>(ATLAS_WIDTH), static_cast<size_t>(ATLAS_HEIGHT)};

    const auto textureIndex = static_cast<int>(mAtlasTextures.size());
    std::for_each(iFirstPacked, rects.end(), [&](const stbrp_rect& packedRect) {
      atlas.insertImage(packedRect.x, packedRect.y, images[packedRect.id]);
      mAtlasMap[packedRect.id] = TextureInfo{
        {{packedRect.x, packedRect.y}, {packedRect.w, packedRect.h}},
        textureIndex};
    });

    mAtlasTextures.emplace_back(mpRenderer, std::move(atlas));

    rects.erase(iFirstPacked, rects.end());
  } while (!rects.empty());
}


void TextureAtlas::draw(int index, const base::Rect<int>& destRect) const
{
  const auto& info = mAtlasMap[index];
  mAtlasTextures[info.mTextureIndex].render(info.mRect, destRect);
}


void TextureAtlas::draw(
  int index,
  const base::Rect<int>& srcRect,
  const base::Rect<int>& destRect) const
{
  const auto& info = mAtlasMap[index];
  auto actualSrcRect = srcRect;
  actualSrcRect.topLeft += info.mRect.topLeft;

  mAtlasTextures[info.mTextureIndex].render(actualSrcRect, destRect);
}


auto TextureAtlas::drawData(int index) const -> DrawData
{
  const auto& info = mAtlasMap[index];
  const auto& texture = mAtlasTextures[info.mTextureIndex];

  return {
    texture.data(),
    renderer::toTexCoords(info.mRect, texture.width(), texture.height())};
}

} // namespace rigel::renderer
