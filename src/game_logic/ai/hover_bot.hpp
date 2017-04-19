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

#include "base/boost_variant.hpp"
#include "base/warnings.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

namespace rigel { namespace engine { class CollisionChecker; }}
namespace rigel { namespace engine { namespace components { struct Sprite; }}}
namespace rigel { namespace game_logic { class EntityFactory; }}


namespace rigel { namespace game_logic { namespace ai {

namespace components {

struct HoverBotSpawnMachine {
  int mSpawnsRemaining = 30;
  int mNextSpawnCountdown = 0;
};


namespace detail {

enum class Orientation {
  Left,
  Right
};


struct TeleportingIn {
  int mFramesElapsed = 0;
};


struct Moving {
  Moving(const Orientation orientation)
    : mOrientation(orientation)
  {
  }

  Orientation mOrientation;
};


struct Reorientation {
  Reorientation(const Orientation targetOrientation)
    : mTargetOrientation(targetOrientation)
  {
  }

  Orientation mTargetOrientation;
  int mStep = 0;
};

} // namespace detail


using HoverBot =
  boost::variant<detail::TeleportingIn, detail::Moving, detail::Reorientation>;

}


class HoverBotSystem {
public:
  HoverBotSystem(
    entityx::Entity player,
    engine::CollisionChecker* pCollisionChecker,
    EntityFactory* pEntityFactory);

  void update(entityx::EntityManager& es);

private:
  void updateReorientation(
    components::detail::Reorientation& state,
    components::HoverBot& robotState,
    engine::components::Sprite& sprite);

private:
  entityx::Entity mPlayer;
  engine::CollisionChecker* mpCollisionChecker;
  EntityFactory* mpEntityFactory;
  bool mIsOddFrame = false;
};

}}}
