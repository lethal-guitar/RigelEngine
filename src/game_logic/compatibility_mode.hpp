/* Copyright (C) 2020, Nikolai Wuttke. All rights reserved.
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

#include "base/warnings.hpp"
#include "game_logic/behavior_controller.hpp"
#include "game_logic/damage_components.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <optional>
#include <vector>


namespace rigel::game_logic {

namespace components {

struct SlotIndex {
  enum class Group {
    Actors,
    Projectiles,
    Effects
  };

  SlotIndex(const int index, const Group group)
    : mIndex(index)
    , mGroup(group)
  {
  }

  friend bool operator<(const SlotIndex& lhs, const SlotIndex& rhs) {
    return std::tie(lhs.mIndex, lhs.mGroup) < std::tie(rhs.mIndex, rhs.mGroup);
  }

  int mIndex;
  Group mGroup;
};

}


template <typename C>
auto collectSortedEntities(entityx::EntityManager& es) {
  namespace c = game_logic::components;

  std::vector<std::tuple<entityx::Entity, c::SlotIndex>> result;

  es.each<C, c::SlotIndex>(
    [&result](
      entityx::Entity e,
      const C&,
      const c::SlotIndex& slotIndex
    ) {
      result.push_back({e, slotIndex});
    });

  std::sort(std::begin(result), std::end(result),
    [](const auto& lhs, const auto& rhs) {
      return std::get<1>(lhs) < std::get<1>(rhs);
    });

  return result;
}


class SlotIndexPool {
public:
  explicit SlotIndexPool(int numSlots);

  std::optional<int> acquireSlot();
  void releaseSlot(int index);

  bool hasFreeSlots() const;

private:
  std::vector<int> mFreeSlots;
};


class UpdateOrderManager : public entityx::Receiver<UpdateOrderManager> {
public:
  explicit UpdateOrderManager(entityx::EventManager& eventManager);

  void receive(
    const entityx::ComponentAddedEvent<
      game_logic::components::PlayerProjectile>& event);
  void receive(
    const entityx::ComponentAddedEvent<
      game_logic::components::BehaviorController>& event);
  void receive(
    const entityx::ComponentRemovedEvent<
      game_logic::components::SlotIndex>& event);

  bool canSpawnActor() const {
    return mActorSlots.hasFreeSlots();
  }

  bool canSpawnProjectile() const {
    return mProjectileSlots.hasFreeSlots();
  }

  bool canSpawnEffect() const {
    return mEffectSlots.hasFreeSlots();
  }

private:
  SlotIndexPool mActorSlots{448};
  SlotIndexPool mProjectileSlots{6};
  SlotIndexPool mEffectSlots{18};
};

}
