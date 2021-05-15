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

#include "data/player_model.hpp"
#include "data/tutorial_messages.hpp"

#include <optional>
#include <variant>


namespace rigel::game_logic
{

namespace components
{

struct CollectableItem
{
  std::optional<int> mGivenScore;
  std::optional<int> mGivenScoreAtFullHealth;
  std::optional<int> mGivenHealth;
  std::optional<data::InventoryItemType> mGivenItem;
  std::optional<data::WeaponType> mGivenWeapon;
  std::optional<data::CollectableLetterType> mGivenCollectableLetter;
  std::optional<data::TutorialMessageId> mShownTutorialMessage;
  bool mSpawnScoreNumbers = true;
};


struct CollectableItemForCheat
{
  std::variant<data::InventoryItemType, data::WeaponType> mGivenItem;
};

} // namespace components


inline std::optional<int> givenScore(
  const components::CollectableItem& collectable,
  const bool playerAtFullHealth)
{
  if (collectable.mGivenScore && collectable.mGivenScoreAtFullHealth)
  {
    const auto score = playerAtFullHealth ? *collectable.mGivenScoreAtFullHealth
                                          : *collectable.mGivenScore;
    return score;
  }

  return collectable.mGivenScore;
}

} // namespace rigel::game_logic
