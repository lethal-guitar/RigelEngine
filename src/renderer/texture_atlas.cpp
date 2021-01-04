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

#include <stdexcept>


namespace rigel::renderer {

namespace {

constexpr auto ATLAS_WIDTH = 2048;
constexpr auto ATLAS_HEIGHT = 1024;

}


TextureAtlas::TextureAtlas(
  Renderer* pRenderer,
  const std::vector<data::Image>& images
)
  : mpRenderer(pRenderer)
{
  std::vector<stbrp_rect> rects;
  rects.reserve(images.size());

  auto index = 0;
  for (const auto& image : images) {
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

  stbrp_init_target(
    &context,
    ATLAS_WIDTH,
    ATLAS_HEIGHT,
    nodes.data(),
    static_cast<int>(nodes.size()));

  const auto result =
    stbrp_pack_rects(&context, rects.data(), static_cast<int>(images.size()));
  if (!result) {
    throw std::runtime_error("Failed to build texture atlas");
  }

  data::Image atlas{ATLAS_WIDTH, ATLAS_HEIGHT};

  for (const auto& packedRect : rects) {
    atlas.insertImage(packedRect.x, packedRect.y, images[packedRect.id]);
  }

  mAtlasTexture = OwningTexture{mpRenderer, std::move(atlas)};

  mCoordinatesMap.reserve(images.size());
  for (const auto& packedRect : rects) {
    mCoordinatesMap.push_back(toTexCoords(
      {{packedRect.x, packedRect.y}, {packedRect.w, packedRect.h}},
      mAtlasTexture.width(),
      mAtlasTexture.height()));
  }
}


void TextureAtlas::draw(int index, const base::Rect<int>& destRect) const {
  mpRenderer->drawTexture(
    mAtlasTexture.data(),
    mCoordinatesMap[index],
    destRect);
}

}
