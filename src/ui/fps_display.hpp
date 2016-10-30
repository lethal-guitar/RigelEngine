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

#pragma once

#include "menu_element_renderer.hpp"

#include <engine/timing.hpp>


namespace rigel { namespace loader {
  class ResourceLoader;
}}


namespace rigel { namespace ui {

class FpsDisplay {
public:
  FpsDisplay(SDL_Renderer* pRenderer, const loader::ResourceLoader& resources);

  void updateAndRender(engine::TimeDelta elapsed);

private:
  MenuElementRenderer mTextRenderer;

  float mSmoothedFrameTime = 0.0f;
  float mWeightedFrameTime = 0.0f;
};

}}
