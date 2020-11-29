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

#include "data/saved_game.hpp"

#include <algorithm>
#include <cassert>


namespace rigel::data {

PlayerModel::PlayerModel()
  : mWeapon(WeaponType::Normal)
  , mScore(0)
  , mAmmo(MAX_AMMO)
  , mHealth(MAX_HEALTH)
{
}


PlayerModel::PlayerModel(const SavedGame& save)
  : mTutorialMessages(save.mTutorialMessagesAlreadySeen)
  , mWeapon(save.mWeapon)
  , mScore(save.mScore)
  , mAmmo(save.mAmmo)
  , mHealth(MAX_HEALTH)
{
}


PlayerModel::CheckpointState PlayerModel::makeCheckpoint() const {
  return CheckpointState{mWeapon, mAmmo, mHealth};
}


void PlayerModel::restoreFromCheckpoint(const CheckpointState& state) {
  mHealth = std::max(2, state.mHealth);
  mWeapon = state.mWeapon;
  mAmmo = state.mAmmo;
}


int PlayerModel::score() const {
  return mScore;
}


void PlayerModel::giveScore(const int amount) {
  mScore = std::clamp(mScore + amount, 0, MAX_SCORE);
}


int PlayerModel::ammo() const {
  return mAmmo;
}


int PlayerModel::currentMaxAmmo() const {
  return
    mWeapon == WeaponType::FlameThrower ? MAX_AMMO_FLAME_THROWER : MAX_AMMO;
}


WeaponType PlayerModel::weapon() const {
  return mWeapon;
}


bool PlayerModel::currentWeaponConsumesAmmo() const {
  return mWeapon != WeaponType::Normal;
}


void PlayerModel::switchToWeapon(const WeaponType type) {
  mWeapon = type;
  mAmmo = currentMaxAmmo();
}


void PlayerModel::useAmmo() {
  if (currentWeaponConsumesAmmo()) {
    --mAmmo;
    if (mAmmo <= 0) {
      switchToWeapon(WeaponType::Normal);
    }
  }
}


void PlayerModel::setAmmo(int amount) {
  assert(amount >= 0 && amount <= currentMaxAmmo());
  mAmmo = amount;
}


int PlayerModel::health() const {
  return mHealth;
}


bool PlayerModel::isAtFullHealth() const {
  return mHealth == MAX_HEALTH;
}


bool PlayerModel::isDead() const {
  return mHealth <= 0;
}


void PlayerModel::takeDamage(const int amount) {
  mHealth = std::clamp(mHealth - amount, 0, MAX_HEALTH);
}


void PlayerModel::takeFatalDamage() {
  mHealth = 0;
}


void PlayerModel::giveHealth(const int amount) {
  mHealth = std::clamp(mHealth + amount, 0, MAX_HEALTH);
}


const std::vector<InventoryItemType>& PlayerModel::inventory() const {
  return mInventory;
}


bool PlayerModel::hasItem(const InventoryItemType type) const {
  using namespace std;

  return find(begin(mInventory), end(mInventory), type) != end(mInventory);
}


void PlayerModel::giveItem(InventoryItemType type) {
  mInventory.push_back(type);
}


void PlayerModel::removeItem(const InventoryItemType type) {
  using namespace std;

  auto iItem = find(begin(mInventory), end(mInventory), type);
  if (iItem != end(mInventory)) {
    mInventory.erase(iItem);
  }
}


const std::vector<CollectableLetterType>& PlayerModel::collectedLetters() const {
  return mCollectedLetters;
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


void PlayerModel::resetForNewLevel() {
  mHealth = MAX_HEALTH;
  mCollectedLetters.clear();
  mInventory.clear();
}


void PlayerModel::resetHealthAndScore() {
  mHealth = MAX_HEALTH;
  mScore = 0;
}


TutorialMessageState& PlayerModel::tutorialMessages() {
  return mTutorialMessages;
}


const TutorialMessageState& PlayerModel::tutorialMessages() const {
  return mTutorialMessages;
}

}
