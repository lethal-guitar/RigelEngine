/* Copyright (C) 2018, Nikolai Wuttke. All rights reserved.
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

#include "player_model.hpp"


namespace rigel { namespace data {

bool PlayerModel::isAtFullHealth() const {
  return mHealth == MAX_HEALTH;
}


void PlayerModel::updateTemporaryItemExpiry() {
  if (hasItem(InventoryItemType::RapidFire)) {
    ++mFramesElapsedHavingRapidFire;
    if (mFramesElapsedHavingRapidFire >= TEMPORARY_ITEM_EXPIRATION_TIME) {
      removeItem(InventoryItemType::RapidFire);
      mFramesElapsedHavingRapidFire = 0;
    }
  }
}


void PlayerModel::resetForNewLevel() {
  mHealth = MAX_HEALTH;
  mCollectedLetters.clear();
  mInventory.clear();
  mFramesElapsedHavingRapidFire = mFramesElapsedHavingCloak = 0;
}


PlayerModel::LetterCollectionState PlayerModel::addLetter(
  const CollectableLetterType type
) {
  using L = CollectableLetterType;
  static std::vector<L> sExpectedOrder = {
    L::N, L::U, L::K, L::E, L::M};


  mCollectedLetters.push_back(type);

  if (mCollectedLetters.size() < 5) {
    return LetterCollectionState::Incomplete;
  } else {
    return mCollectedLetters == sExpectedOrder
      ? LetterCollectionState::InOrder
      : LetterCollectionState::WrongOrder;
  }
}

}}
