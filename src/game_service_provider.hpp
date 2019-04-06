/* Copyright (C) 2017, Nikolai Wuttke. All rights reserved.
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

#include "data/game_session_data.hpp"
#include "data/sound_ids.hpp"

#include <string>

namespace rigel::data { struct SavedGame; }


namespace rigel {

/** Interface for functionality available to game modes */
struct IGameServiceProvider {
  virtual ~IGameServiceProvider() = default;

  // Blocking calls
  virtual void fadeOutScreen() = 0;
  virtual void fadeInScreen() = 0;

  // Non-blocking calls
  virtual void playSound(data::SoundId id) = 0;
  virtual void stopSound(data::SoundId id) = 0;
  virtual void playMusic(const std::string& name) = 0;
  virtual void stopMusic() = 0;
  virtual void scheduleNewGameStart(
    int episode,
    data::Difficulty difficulty) = 0;
  virtual void scheduleStartFromSavedGame(const data::SavedGame& save) = 0;
  virtual void scheduleEnterMainMenu() = 0;
  virtual void scheduleGameQuit() = 0;
  virtual bool isShareWareVersion() const = 0;

  virtual void showDebugText(const std::string& text) = 0;
};

}
