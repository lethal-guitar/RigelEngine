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

#pragma once

#include "base/warnings.hpp"
#include "game_logic/global_dependencies.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <memory>


namespace rigel { namespace game_logic { namespace components {

template <typename T>
void updateBehaviorController(
  T& self,
  GlobalDependencies& dependencies,
  const bool isOddFrame,
  const bool isOnScreen,
  entityx::Entity entity
) {
  self.update(dependencies, isOddFrame, isOnScreen, entity);
}


class BehaviorController {
public:
  template<typename T>
  explicit BehaviorController(T controller)
    : mpSelf(std::make_shared<Model<T>>(std::move(controller)))
  {
  }

  void update(
    GlobalDependencies& dependencies,
    const bool isOddFrame,
    const bool isOnScreen,
    entityx::Entity entity
  ) {
    mpSelf->update(dependencies, isOddFrame, isOnScreen, entity);
  }

private:
  struct Concept {
    virtual ~Concept() = default;

    virtual void update(
      GlobalDependencies& dependencies,
      bool isOddFrame,
      bool isOnScreen,
      entityx::Entity entity) = 0;
  };

  template<typename T>
  struct Model : public Concept {
    explicit Model(T data_)
      : mData(std::move(data_))
    {
    }

    void update(
      GlobalDependencies& dependencies,
      bool isOddFrame,
      bool isOnScreen,
      entityx::Entity entity
    ) override {
      updateBehaviorController(
        mData,
        dependencies,
        isOddFrame,
        isOnScreen,
        entity);
    }

    T mData;
  };

  std::shared_ptr<Concept> mpSelf;
};

}}}
