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

#include "data/tutorial_messages.hpp"

#include <vector>


namespace rigel::game_logic
{
class GameWorld_Classic;
}


namespace rigel::data
{

struct SavedGame;


enum class InventoryItemType
{
  CircuitBoard,
  BlueKey,
  RapidFire,
  SpecialHintGlobe,
  CloakingDevice
};


enum class CollectableLetterType
{
  N,
  U,
  K,
  E,
  M
};


enum class WeaponType
{
  Normal = 0,
  Laser = 1,
  Rocket = 2,
  FlameThrower = 3
};


constexpr auto MAX_SCORE = 9999999;
constexpr auto MAX_AMMO = 32;
constexpr auto MAX_AMMO_FLAME_THROWER = 64;
constexpr auto MAX_HEALTH = 9;


class PlayerModel
{
public:
  struct CheckpointState
  {
    WeaponType mWeapon;
    int mAmmo;
    int mHealth;
  };

  enum class LetterCollectionState
  {
    Incomplete,
    WrongOrder,
    InOrder
  };

  PlayerModel();
  explicit PlayerModel(const SavedGame& save);

  CheckpointState makeCheckpoint() const;
  void restoreFromCheckpoint(const CheckpointState& state);

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
  void takeDamage(int amount);

  /** Immediately set health to 0 */
  void takeFatalDamage();
  void giveHealth(int amount);

  const std::vector<InventoryItemType>& inventory() const;
  bool hasItem(const InventoryItemType type) const;
  void giveItem(InventoryItemType type);
  void removeItem(const InventoryItemType type);

  const std::vector<CollectableLetterType>& collectedLetters() const;
  LetterCollectionState addLetter(CollectableLetterType type);

  void resetForNewLevel();

  /** Restore full health, reset score to 0
   *
   * This implements the "eat" cheat code.
   */
  void resetHealthAndScore();

  TutorialMessageState& tutorialMessages();
  const TutorialMessageState& tutorialMessages() const;

  friend class rigel::game_logic::GameWorld_Classic;

private:
  std::vector<CollectableLetterType> mCollectedLetters;
  std::vector<InventoryItemType> mInventory;
  TutorialMessageState mTutorialMessages;
  WeaponType mWeapon;
  int mScore;
  int mAmmo;
  int mHealth;
};

} // namespace rigel::data
