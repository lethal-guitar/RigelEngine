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
namespace rigel { namespace engine { class CollisionChecker; }}
namespace rigel { namespace game_logic { namespace events {
  struct ShootableKilled;
}}}


namespace rigel { namespace game_logic {

class EntityFactory;


/** Provides type erasure for component classes */
class ComponentHolder {
public:
  template<typename T>
  explicit ComponentHolder(T component_)
    : mpSelf(std::make_shared<Model<T>>(std::move(component_)))
  {
  }

  void assignToEntity(entityx::Entity entity) const {
    mpSelf->assignToEntity(entity);
  }

private:
  struct Concept {
    virtual ~Concept() = default;
    virtual void assignToEntity(entityx::Entity entity) const = 0;
  };

  template<typename T>
  struct Model : public Concept {
    explicit Model(T data_)
      : mData(std::move(data_))
    {
    }

    void assignToEntity(entityx::Entity entity) const override {
      entity.assign<T>(mData);
    }

    T mData;
  };

  std::shared_ptr<Concept> mpSelf;
};


namespace components {

struct ItemContainer {
  std::vector<ComponentHolder> mContainedComponents;

  template<typename TComponent, typename... TArgs>
  void assign(TArgs&&... components) {
    mContainedComponents.emplace_back(
      TComponent{std::forward<TArgs>(components)...});
  }
};


struct NapalmBomb {
  enum class State {
    Ticking,
    SpawningFires
  };

  State mState = State::Ticking;
  int mFramesElapsed = 0;
  bool mCanSpawnLeft = true;
  bool mCanSpawnRight = true;
};

}

class ItemContainerSystem : public entityx::Receiver<ItemContainerSystem> {
public:
  ItemContainerSystem(
    entityx::EntityManager* pEntityManager,
    entityx::EventManager& events);

  void update(entityx::EntityManager& es);
  void receive(const events::ShootableKilled& event);

private:
  entityx::EntityManager* mpEntityManager;
  std::vector<entityx::Entity> mShotContainersQueue;
};


class NapalmBombSystem : public entityx::Receiver<NapalmBombSystem> {
public:
  NapalmBombSystem(
    IGameServiceProvider* pServiceProvider,
    EntityFactory* pEntityFactory,
    engine::CollisionChecker* pCollisionChecker,
    entityx::EventManager& events);

  void update(entityx::EntityManager& es);

  void receive(const events::ShootableKilled& event);

private:
  void explode(entityx::Entity entity);
  void spawnFires(
    components::NapalmBomb& state,
    const base::Vector& position,
    int step);

  IGameServiceProvider* mpServiceProvider;
  EntityFactory* mpEntityFactory;
  engine::CollisionChecker* mpCollisionChecker;
};

}}
