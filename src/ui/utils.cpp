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

#include "utils.hpp"

#include "loader/resource_loader.hpp"


namespace rigel::ui
{

ImU32 toImgui(const base::Color& color)
{
  RIGEL_DISABLE_WARNINGS
  return IM_COL32(color.r, color.g, color.b, color.a);
  RIGEL_RESTORE_WARNINGS
}


renderer::Texture fullScreenImageAsTexture(
  renderer::Renderer* pRenderer,
  const loader::ResourceLoader& resources,
  std::string_view imageName)
{
  return renderer::Texture(
    pRenderer, resources.loadStandaloneFullscreenImage(imageName));
}


engine::TiledTexture makeUiSpriteSheet(
  renderer::Renderer* pRenderer,
  const loader::ResourceLoader& resourceLoader,
  const data::Palette16& palette)
{
  return engine::TiledTexture{
    renderer::Texture{pRenderer, resourceLoader.loadUiSpriteSheet(palette)},
    pRenderer};
}


void drawText(
  const std::string_view text,
  const int x,
  const int y,
  const base::Color& color)
{
  auto pDrawList = ImGui::GetForegroundDrawList();
  pDrawList->AddText(
    {static_cast<float>(x), static_cast<float>(y)},
    toImgui(color),
    text.data(),
    text.data() + text.size());
}


void drawLoadingScreenText()
{
  constexpr auto TEXT = "Loading...";
  const auto fontSize = 256.0f * ImGui::GetIO().FontGlobalScale;

  auto pFont = ImGui::GetFont();
  const auto textSize = pFont->CalcTextSizeA(fontSize, FLT_MAX, -1.0f, TEXT);
  const auto windowSize = ImGui::GetIO().DisplaySize;

  const auto position = ImVec2{
    windowSize.x / 2.0f - textSize.x / 2.0f,
    windowSize.y / 2.0f - textSize.y / 2.0f};

  auto pDrawList = ImGui::GetForegroundDrawList();
  pDrawList->AddText(
    nullptr, fontSize, position, ui::toImgui({255, 255, 255, 255}), TEXT);
}

} // namespace rigel::ui
