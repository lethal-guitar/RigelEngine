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

#include "utils/enum_hash.hpp"

#include <unordered_set>


namespace rigel { namespace data {

const auto MAX_SCORE = 9999999;
const auto MAX_AMMO = 32;
const auto MAX_AMMO_FLAME_THROWER = 64;
const auto MAX_HEALTH = 9;

const auto TEMPORARY_ITEM_EXPIRATION_TIME = 700;


enum class InventoryItemType {
  CircuitBoard,
  BlueKey,
  RapidFire,
  SpecialHintGlobe,
  CloakingDevice
};


enum class CollectableLetterType {
  N,
  U,
  K,
  E,
  M
};


enum class WeaponType {
  Normal = 0,
  Laser = 1,
  Rocket = 2,
  FlameThrower = 3
};


enum class PlayerBuff {
  RapidFire,
  Cloak
};

}}

RIGEL_PROVIDE_ENUM_CLASS_HASH(rigel::data::CollectableLetterType)
RIGEL_PROVIDE_ENUM_CLASS_HASH(rigel::data::InventoryItemType)


namespace rigel { namespace data {

struct PlayerModel {
  int currentMaxAmmo() const {
    return
      mWeapon == WeaponType::FlameThrower ? MAX_AMMO_FLAME_THROWER : MAX_AMMO;
  }

  void switchToWeapon(const WeaponType type) {
    mWeapon = type;
    mAmmo = currentMaxAmmo();
  }

  bool currentWeaponConsumesAmmo() const {
    return mWeapon != WeaponType::Normal;
  }

  bool hasItem(const InventoryItemType type) const {
    return mInventory.count(type) != 0;
  }

  void removeItem(const InventoryItemType type) {
    mInventory.erase(type);
  }

  void updateTemporaryItemExpiry() {
    if (hasItem(InventoryItemType::RapidFire)) {
      ++mFramesElapsedHavingRapidFire;
      if (mFramesElapsedHavingRapidFire >= TEMPORARY_ITEM_EXPIRATION_TIME) {
        removeItem(InventoryItemType::RapidFire);
        mFramesElapsedHavingRapidFire = 0;
      }
    }
  }

  std::unordered_set<CollectableLetterType> mCollectedLetters;
  std::unordered_set<InventoryItemType> mInventory;
  WeaponType mWeapon = WeaponType::Normal;
  int mScore = 0;
  int mAmmo = MAX_AMMO;
  int mHealth = MAX_HEALTH;
  int mFramesElapsedHavingRapidFire = 0;
  int mFramesElapsedHavingCloak = 0;
};


}}
