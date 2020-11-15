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
#include "common/game_mode.hpp"
#include "data/bonus.hpp"
#include "data/saved_game.hpp"
#include "game_logic/game_world.hpp"
#include "game_logic/input.hpp"
#include "ui/ingame_menu.hpp"

RIGEL_DISABLE_WARNINGS
#include <SDL.h>
RIGEL_RESTORE_WARNINGS


namespace rigel {

class GameRunner {
public:
  GameRunner(
    data::PlayerModel* pPlayerModel,
    const data::GameSessionId& sessionId,
    GameMode::Context context,
    std::optional<base::Vector> playerPositionOverride = std::nullopt,
    bool showWelcomeMessage = false);

  void handleEvent(const SDL_Event& event);
  void updateAndRender(engine::TimeDelta dt);

  bool levelFinished() const;
  bool gameQuit() const;
  std::optional<data::SavedGame> requestedGameToLoad() const;

  std::set<data::Bonus> achievedBonuses() const;

private:
  void updateWorld(engine::TimeDelta dt);
  bool updateMenu(engine::TimeDelta dt);
  void handlePlayerKeyboardInput(const SDL_Event& event);
  void handlePlayerGameControllerInput(const SDL_Event& event);
  void handleDebugKeys(const SDL_Event& event);
  void renderDebugText();

  GameMode::Context mContext;

  game_logic::GameWorld mWorld;
  game_logic::PlayerInput mPlayerInput;
  base::Vector mAnalogStickVector;
  engine::TimeDelta mAccumulatedTime = 0.0;
  ui::IngameMenu mMenu;
  bool mShowDebugText = false;
  bool mSingleStepping = false;
  bool mDoNextSingleStep = false;
  bool mLevelFinishedByDebugKey = false;
};


inline bool GameRunner::levelFinished() const {
  return mWorld.levelFinished() || mLevelFinishedByDebugKey;
}


inline bool GameRunner::gameQuit() const {
  return mMenu.quitRequested();
}


inline std::optional<data::SavedGame> GameRunner::requestedGameToLoad() const {
  return mMenu.requestedGameToLoad();
}


inline std::set<data::Bonus> GameRunner::achievedBonuses() const {
  return mWorld.achievedBonuses();
}

}
