/* Copyright (C) 2017, Nikolai Wuttke. All rights reserved.
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

#include <variant>

namespace rigel { namespace engine {
  class CollisionChecker;
  namespace events { struct CollidedWithWorld; }
}}


namespace rigel { namespace game_logic { namespace ai {

namespace components {

namespace detail {

struct Flying {};

struct Hovering {
  int mFramesElapsed = 0;
};

struct PlungingDown {
  int mInitialHeight;
};

struct RisingUp {
  explicit RisingUp(const int initialHeight)
    : mInitialHeight(initialHeight)
  {
  }

  int mInitialHeight;
  bool mBackAtOriginalHeight = false;
};

}

using StateT = std::variant<
  detail::Flying,
  detail::Hovering,
  detail::PlungingDown,
  detail::RisingUp>;


struct RedBird {
  StateT mState;
};

}


void configureRedBird(entityx::Entity entity);


class RedBirdSystem : public entityx::Receiver<RedBirdSystem> {
public:
  explicit RedBirdSystem(entityx::Entity player, entityx::EventManager& events);

  void update(entityx::EntityManager& es);

  void receive(const engine::events::CollidedWithWorld& event);

private:
  entityx::Entity mPlayer;
  bool mIsOddFrame = false;
};

}}}
