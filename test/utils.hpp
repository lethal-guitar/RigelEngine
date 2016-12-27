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

#include <base/warnings.hpp>
#include <data/sound_ids.hpp>
#include <data/game_session_data.hpp>
#include <game_mode.hpp>

RIGEL_DISABLE_WARNINGS
#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>
RIGEL_RESTORE_WARNINGS


struct MockServiceProvider : public rigel::IGameServiceProvider {
  void fadeOutScreen() override {}
  void fadeInScreen() override {}

  void playSound(rigel::data::SoundId id) override {
    mLastTriggeredSoundId = id;
  }

  void playMusic(const std::string&) override {}
  void stopMusic() override {}
  void scheduleNewGameStart(int, rigel::data::Difficulty) override {}
  void scheduleEnterMainMenu() override {}
  void scheduleGameQuit() override {}
  bool isShareWareVersion() const override { return false; }

  void showDebugText(const std::string&) override {}

  boost::optional<rigel::data::SoundId> mLastTriggeredSoundId;
};
