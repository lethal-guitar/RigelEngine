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

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <memory>


namespace rigel { namespace game_logic {

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
};

}


namespace item_containers {

void onEntityHit(entityx::Entity entity, entityx::EntityManager& es);

}

}}
