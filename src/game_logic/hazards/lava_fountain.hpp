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

#include <base/spatial_types.hpp>
#include <base/warnings.hpp>

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <variant>

namespace rigel::game_logic {
  struct GlobalDependencies;
  struct GlobalState;
}
namespace rigel::engine::components { struct Sprite; }
namespace rigel::renderer { class Renderer; }


namespace rigel::game_logic::behaviors {

struct LavaFountain {
  struct Waiting {
    int mFramesElapsed = 0;
  };

  struct Erupting {
    int mSequenceIndex = 0;
  };

  using State = std::variant<Waiting, Erupting>;

  void update(
    GlobalDependencies& dependencies,
    GlobalState& state,
    bool isOnScreen,
    entityx::Entity entity);

  static void render(
    renderer::Renderer* pRenderer,
    entityx::Entity entity,
    const engine::components::Sprite& sprite,
    const base::Vector& positionInScreenSpace);

  State mState;
};

}
