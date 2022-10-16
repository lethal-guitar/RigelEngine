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

#include "base/spatial_types.hpp"
#include "base/warnings.hpp"
#include "data/bonus.hpp"
#include "data/saved_game.hpp"
#include "frontend/game_mode.hpp"
#include "frontend/input_handler.hpp"
#include "game_logic_common/igame_world.hpp"
#include "game_logic_common/input.hpp"
#include "ui/ingame_menu.hpp"

RIGEL_DISABLE_WARNINGS
#include <SDL.h>
RIGEL_RESTORE_WARNINGS

#include <memory>


namespace rigel
{

class GameRunner
{
public:
  GameRunner(
    data::PlayerModel* pPlayerModel,
    const data::GameSessionId& sessionId,
    GameMode::Context context,
    std::optional<base::Vec2> playerPositionOverride = std::nullopt,
    bool showWelcomeMessage = false);

  void handleEvent(const SDL_Event& event);
  void updateAndRender(engine::TimeDelta dt);
  bool needsPerElementUpscaling() const;

  bool levelFinished() const;
  bool gameQuit() const;
  std::optional<data::SavedGame> requestedGameToLoad() const;

  std::set<data::Bonus> achievedBonuses() const;

private:
  float interpolationFactor(engine::TimeDelta dt) const;
  void updateWorld(engine::TimeDelta dt);
  bool updateMenu(engine::TimeDelta dt);
  void handleDebugKeys(const SDL_Event& event);
  void renderDebugText();

  GameMode::Context mContext;

  std::unique_ptr<game_logic::IGameWorld> mpWorld;
  InputHandler mInputHandler;
  engine::TimeDelta mAccumulatedTime = 0.0;
  ui::IngameMenu mMenu;
  bool mShowDebugText = false;
  bool mSingleStepping = false;
  bool mDoNextSingleStep = false;
  bool mLevelFinishedByDebugKey = false;
};

} // namespace rigel
