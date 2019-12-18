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

#include <base/warnings.hpp>
#include <engine/base_components.hpp>

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <variant>


namespace rigel::engine::components { struct Sprite; }
namespace rigel::game_logic {
  struct GlobalDependencies;
  struct GlobalState;
}
namespace rigel::renderer { class Renderer; }


namespace rigel::game_logic::behaviors {

struct GrabberClaw {
  struct Extending {};

  struct Grabbing {
    int mFramesElapsed = 0;
  };

  struct Retracting {};

  struct Waiting {
    int mFramesElapsed = 0;
  };

  using State = std::variant<Extending, Grabbing, Retracting, Waiting>;

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
  int mExtensionStep = 0;
};

}
