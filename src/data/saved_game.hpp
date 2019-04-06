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

#include "data/game_session_data.hpp"
#include "data/player_model.hpp"

#include <array>
#include <string>
#include <optional>


namespace rigel::data {

constexpr auto NUM_SAVE_SLOTS = 8u;

struct SavedGame {
  GameSessionId mSessionId;
  TutorialMessageState mTutorialMessagesAlreadySeen;
  std::string mName;
  WeaponType mWeapon = WeaponType::Normal;
  int mAmmo = 0;
  int mScore = 0;
};


using SaveSlotArray = std::array<std::optional<SavedGame>, NUM_SAVE_SLOTS>;

}
