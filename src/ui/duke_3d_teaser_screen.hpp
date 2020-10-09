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

#pragma once

#include "engine/timing.hpp"
#include "renderer/texture.hpp"

namespace rigel::loader { class ResourceLoader; }
namespace rigel::renderer { class Renderer; }


namespace rigel::ui {

class Duke3DTeaserScreen {
public:
  Duke3DTeaserScreen(
    const loader::ResourceLoader& resources,
    renderer::Renderer* pRenderer);

  void updateAndRender(const engine::TimeDelta dt);
  bool isFinished() const;

private:
  renderer::OwningTexture mTextImage;
  renderer::Renderer* mpRenderer;
  engine::TimeDelta mElapsedTime = 0.0;
};

}
