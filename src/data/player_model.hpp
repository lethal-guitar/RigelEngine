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
#include <vector>


namespace rigel { namespace data {

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

}}

RIGEL_PROVIDE_ENUM_CLASS_HASH(rigel::data::CollectableLetterType)
RIGEL_PROVIDE_ENUM_CLASS_HASH(rigel::data::InventoryItemType)


namespace rigel { namespace data {

class PlayerModel {
public:
  enum class LetterCollectionState {
    Incomplete,
    WrongOrder,
    InOrder
  };

  PlayerModel();

  int score() const;
  void giveScore(int amount);

  int ammo() const;
  int currentMaxAmmo() const;
  WeaponType weapon() const;
  bool currentWeaponConsumesAmmo() const;
  void switchToWeapon(const WeaponType type);
  void useAmmo();
  void setAmmo(int amount);

  int health() const;
  bool isAtFullHealth() const;
  bool isDead() const;
  void takeHealth(int amount);
  void giveHealth(int amount);

  const std::unordered_set<InventoryItemType>& inventory() const;
  bool hasItem(const InventoryItemType type) const;
  void giveItem(InventoryItemType type);
  void removeItem(const InventoryItemType type);

  const std::vector<CollectableLetterType>& collectedLetters() const;
  LetterCollectionState addLetter(CollectableLetterType type);

  void resetForNewLevel();
  void updateTemporaryItemExpiry();

private:
  std::vector<CollectableLetterType> mCollectedLetters;
  std::unordered_set<InventoryItemType> mInventory;
  WeaponType mWeapon;
  int mScore;
  int mAmmo;
  int mHealth;
  int mFramesElapsedHavingRapidFire;
  int mFramesElapsedHavingCloak;
};

}}
