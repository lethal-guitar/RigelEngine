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

#include "base/warnings.hpp"
#include "base/spatial_types.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <memory>
#include <vector>

namespace rigel { struct IGameServiceProvider; }
namespace rigel::engine { class CollisionChecker; }
namespace rigel::game_logic { struct GlobalState; }
namespace rigel::game_logic::events {
  struct ShootableKilled;
}


namespace rigel::game_logic {

class EntityFactory;
struct GlobalDependencies;


/** Provides type erasure for component classes */
class ComponentHolder {
public:
  template<typename T>
  explicit ComponentHolder(T component_)
    : mpSelf(std::make_unique<Model<T>>(std::move(component_)))
  {
  }

  ComponentHolder(const ComponentHolder& other)
    : mpSelf(other.mpSelf->clone())
  {
  }

  ComponentHolder& operator=(const ComponentHolder& other) {
    auto copy = other;
    std::swap(mpSelf, copy.mpSelf);
    return *this;
  }

  ComponentHolder(ComponentHolder&&) = default;
  ComponentHolder& operator=(ComponentHolder&&) = default;

  void assignToEntity(entityx::Entity entity) const {
    mpSelf->assignToEntity(entity);
  }

private:
  struct Concept {
    virtual ~Concept() = default;
    virtual std::unique_ptr<Concept> clone() const = 0;
    virtual void assignToEntity(entityx::Entity entity) const = 0;
  };

  template<typename T>
  struct Model : public Concept {
    explicit Model(T data_)
      : mData(std::move(data_))
    {
    }

    std::unique_ptr<Concept> clone() const override {
      return std::make_unique<Model>(mData);
    }

    void assignToEntity(entityx::Entity entity) const override {
      entity.assign<T>(mData);
    }

    T mData;
  };

  std::unique_ptr<Concept> mpSelf;
};


namespace components {

struct ItemContainer {
  enum class ReleaseStyle : std::uint8_t {
    Default,
    ItemBox,
    ItemBoxNoBounce,
    NuclearWasteBarrel
  };

  std::vector<ComponentHolder> mContainedComponents;
  ReleaseStyle mStyle = ReleaseStyle::Default;
  std::int8_t mFramesElapsed = 0;
  bool mHasBeenShot = false;

  template<typename TComponent, typename... TArgs>
  void assign(TArgs&&... components) {
    mContainedComponents.emplace_back(
      TComponent{std::forward<TArgs>(components)...});
  }
};

}

class ItemContainerSystem : public entityx::Receiver<ItemContainerSystem> {
public:
  ItemContainerSystem(
    entityx::EntityManager* pEntityManager,
    const engine::CollisionChecker* pCollisionChecker,
    entityx::EventManager& events);

  void update(entityx::EntityManager& es);
  void updateItemBounce(entityx::EntityManager& es);
  void receive(const events::ShootableKilled& event);

private:
  entityx::EntityManager* mpEntityManager;
  const engine::CollisionChecker* mpCollisionChecker;
};


namespace behaviors {

class NapalmBomb {
public:
  void update(
    GlobalDependencies& dependencies,
    GlobalState& state,
    bool isOnScreen,
    entityx::Entity entity);

  void onKilled(
    GlobalDependencies& dependencies,
    GlobalState& state,
    const base::Point<float>& inflictorVelocity,
    entityx::Entity entity);

  enum class State {
    Ticking,
    SpawningFires
  };

  State mState = State::Ticking;
  int mFramesElapsed = 0;
  bool mCanSpawnLeft = true;
  bool mCanSpawnRight = true;

private:
  void explode(GlobalDependencies& dependencies, entityx::Entity entity);
  void spawnFires(
    GlobalDependencies& d, const base::Vector& bombPosition, int step);
};

}

}
