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

#include "compatibility_mode.hpp"

#include <algorithm>
#include <numeric>


namespace rigel::game_logic {

namespace c = game_logic::components;


SlotIndexPool::SlotIndexPool(const int numSlots)
  : mFreeSlots(static_cast<size_t>(numSlots))
{
  std::iota(begin(mFreeSlots), end(mFreeSlots), 0);
  std::make_heap(begin(mFreeSlots), end(mFreeSlots), std::greater<>{});
}


std::optional<int> SlotIndexPool::acquireSlot() {
  if (!hasFreeSlots()) {
    return {};
  }

  std::pop_heap(begin(mFreeSlots), end(mFreeSlots), std::greater<>{});
  const auto result = mFreeSlots.back();
  mFreeSlots.pop_back();

  return result;
}


void SlotIndexPool::releaseSlot(int index) {
  mFreeSlots.push_back(index);
  std::push_heap(begin(mFreeSlots), end(mFreeSlots), std::greater<>{});
}


bool SlotIndexPool::hasFreeSlots() const {
  return !mFreeSlots.empty();
}


UpdateOrderManager::UpdateOrderManager(entityx::EventManager& eventManager) {
  eventManager.subscribe<entityx::ComponentAddedEvent<c::PlayerProjectile>>(*this);
  eventManager.subscribe<entityx::ComponentAddedEvent<c::BehaviorController>>(*this);
  eventManager.subscribe<entityx::ComponentRemovedEvent<c::SlotIndex>>(*this);
}


void UpdateOrderManager::receive(
  const entityx::ComponentAddedEvent<c::PlayerProjectile>& event
) {
  auto entity = event.entity;
  entity.assign<c::SlotIndex>(
    *mProjectileSlots.acquireSlot(),
    c::SlotIndex::Group::Projectiles);
}


void UpdateOrderManager::receive(
  const entityx::ComponentAddedEvent<c::BehaviorController>& event
) {
  auto entity = event.entity;
  entity.assign<c::SlotIndex>(
    *mActorSlots.acquireSlot(),
    c::SlotIndex::Group::Actors);
}


void UpdateOrderManager::receive(
  const entityx::ComponentRemovedEvent<c::SlotIndex>& event
) {
  using G = c::SlotIndex::Group;

  switch (event.component->mGroup) {
    case G::Actors:
      mActorSlots.releaseSlot(event.component->mIndex);
      break;

    case G::Projectiles:
      mProjectileSlots.releaseSlot(event.component->mIndex);
      break;

    case G::Effects:
      mEffectSlots.releaseSlot(event.component->mIndex);
      break;
  }
}

}
