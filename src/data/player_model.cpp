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

#include "base/math_tools.hpp"

#include <cassert>


namespace rigel { namespace data {

namespace {

const auto MAX_SCORE = 9999999;
const auto MAX_AMMO = 32;
const auto MAX_AMMO_FLAME_THROWER = 64;
const auto MAX_HEALTH = 9;

const auto TEMPORARY_ITEM_EXPIRATION_TIME = 700;

}


PlayerModel::PlayerModel()
  : mWeapon(WeaponType::Normal)
  , mScore(0)
  , mAmmo(MAX_AMMO)
  , mHealth(MAX_HEALTH)
  , mFramesElapsedHavingRapidFire(0)
  , mFramesElapsedHavingCloak(0)
{
}


int PlayerModel::score() const {
  return mScore;
}


void PlayerModel::giveScore(const int amount) {
  mScore = base::clamp(mScore + amount, 0, MAX_SCORE);
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


void PlayerModel::takeHealth(const int amount) {
  mHealth = base::clamp(mHealth - amount, 0, MAX_HEALTH);
}


void PlayerModel::giveHealth(const int amount) {
  mHealth = base::clamp(mHealth + amount, 0, MAX_HEALTH);
}


const std::unordered_set<InventoryItemType>& PlayerModel::inventory() const {
  return mInventory;
}


bool PlayerModel::hasItem(const InventoryItemType type) const {
  return mInventory.count(type) != 0;
}


void PlayerModel::giveItem(InventoryItemType type) {
  mInventory.insert(type);
}


void PlayerModel::removeItem(const InventoryItemType type) {
  mInventory.erase(type);
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
  mFramesElapsedHavingRapidFire = mFramesElapsedHavingCloak = 0;
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


}}
