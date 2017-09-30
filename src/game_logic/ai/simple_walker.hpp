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
#include "engine/base_components.hpp"

RIGEL_DISABLE_WARNINGS
#include <boost/optional.hpp>
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

namespace rigel { namespace engine { class CollisionChecker; }}


namespace rigel { namespace game_logic { namespace ai {

namespace components {

struct SimpleWalker {
  struct Configuration {
    int mAnimationSteps = 0;
    int mAnimationDelay = 0;
    bool mWalkAtFullSpeed = false;
  };

  SimpleWalker(const Configuration* pConfig)
    : mpConfig(pConfig)
  {
  }

  const Configuration* mpConfig;
  boost::optional<engine::components::Orientation> mOrientation;
};

}


class SimpleWalkerSystem {
public:
  SimpleWalkerSystem(
    entityx::Entity player,
    engine::CollisionChecker* pCollisionChecker);

  void update(entityx::EntityManager& es);

private:
  entityx::Entity mPlayer;
  engine::CollisionChecker* mpCollisionChecker;
  bool mIsOddFrame = false;
};

}}}
