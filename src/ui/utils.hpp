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

#include "base/color.hpp"
#include "engine/tiled_texture.hpp"
#include "loader/palette.hpp"
#include "renderer/texture.hpp"

#include <string_view>


namespace rigel::loader {
  class ResourceLoader;
}

namespace rigel::ui {

renderer::OwningTexture fullScreenImageAsTexture(
  renderer::Renderer* pRenderer,
  const loader::ResourceLoader& resources,
  const std::string& imageName);

engine::TiledTexture makeUiSpriteSheet(
  renderer::Renderer* pRenderer,
  const loader::ResourceLoader& resourceLoader,
  const loader::Palette16& palette);

void drawText(std::string_view text, int x, int y, const base::Color& color);

}
