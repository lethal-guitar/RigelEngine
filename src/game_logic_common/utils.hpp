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

#include "data/game_traits.hpp"
#include "renderer/renderer.hpp"
#include "renderer/viewport_utils.hpp"


namespace rigel::game_logic
{

[[nodiscard]] inline auto setupIngameViewport(
  renderer::Renderer* pRenderer,
  const int screenShakeOffsetX)
{
  auto saved = renderer::saveState(pRenderer);

  const auto offset =
    data::GameTraits::inGameViewportOffset + base::Vec2{screenShakeOffsetX, 0};
  renderer::setLocalTranslation(pRenderer, offset);
  renderer::setLocalClipRect(
    pRenderer, base::Rect<int>{{}, data::GameTraits::inGameViewportSize});

  return saved;
}

} // namespace rigel::game_logic
