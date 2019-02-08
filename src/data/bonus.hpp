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

#include "data/player_model.hpp"

#include <cstdint>
#include <set>


namespace rigel::data {

constexpr int SCORE_ADDED_PER_BONUS = 100000;


// Enum values match the bonus numbers shown on the bonus screen
enum class Bonus : std::uint8_t {
  DestroyedAllCameras = 1,
  NoDamageTaken = 2,
  CollectedEveryWeapon = 3,
  CollectedAllMerchandise = 4,
  DestroyedAllSpinningLaserTurrets = 5,
  DestroyedAllFireBombs = 6,
  ShotAllBonusGlobes = 7
};


constexpr int asNumber(const Bonus bonus) {
  return static_cast<int>(bonus);
}


inline void addBonusScore(
  PlayerModel& playerModel,
  const std::set<Bonus>& bonuses
) {
  const auto numBonuses = static_cast<int>(bonuses.size());
  playerModel.giveScore(numBonuses * SCORE_ADDED_PER_BONUS);
}

}
