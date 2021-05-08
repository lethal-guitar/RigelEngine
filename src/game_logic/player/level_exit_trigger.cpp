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

#include "level_exit_trigger.hpp"

#include "common/global.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic::behaviors {

void LevelExitTrigger::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool isOnScreen,
  entityx::Entity entity
) {
  using engine::components::WorldPosition;

  const auto& position = *entity.component<WorldPosition>();
  const auto& playerBBox = s.mpPlayer->worldSpaceHitBox();

  // clang-format off
  const auto playerAboveOrAtTriggerHeight =
    playerBBox.bottom() <= position.y;
  const auto touchingTriggerOnXAxis =
    position.x >= playerBBox.left() &&
    position.x <= (playerBBox.right() + 1);
  // clang-format on

  if (playerAboveOrAtTriggerHeight && touchingTriggerOnXAxis) {
    d.mpEvents->emit(rigel::events::ExitReached{});
  }
}

}
