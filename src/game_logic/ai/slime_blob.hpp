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
#include "base/spatial_types.hpp"
#include "base/warnings.hpp"
#include "engine/base_components.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

namespace rigel { namespace engine { class CollisionChecker; }}
namespace rigel { namespace engine { class RandomNumberGenerator; }}
namespace rigel { namespace game_logic { class EntityFactory; }}


namespace rigel { namespace game_logic { namespace ai {

namespace components {

namespace detail {

struct OnGround {
  bool mIsOddUpdate = false;
};

struct Idle {
  int mFramesElapsed = 0;
};

struct Ascending {};

struct Descending {};

struct OnCeiling {
  bool mIsOddUpdate = false;
};


using StateT = boost::variant<
  detail::OnGround,
  detail::OnCeiling,
  detail::Idle,
  detail::Ascending,
  detail::Descending>;

} // namespace detail


struct SlimeContainer {
  int mBreakAnimationStep = 0;
};


struct SlimeBlob {
  detail::StateT mState = detail::Idle{};
  engine::components::Orientation mOrientation =
    engine::components::Orientation::Left;
};

} // namespace components


void configureSlimeContainer(entityx::Entity entity);


class SlimeBlobSystem {
public:
  SlimeBlobSystem(
    entityx::Entity player,
    engine::CollisionChecker* pCollisionChecker,
    EntityFactory* pEntityFactory,
    engine::RandomNumberGenerator* pRandomGenerator);

  void update(entityx::EntityManager& es);
  void onEntityHit(entityx::Entity entity);

private:
  base::Vector adjustedPlayerPosition() const;

private:
  entityx::Entity mPlayer;
  engine::CollisionChecker* mpCollisionChecker;
  EntityFactory* mpEntityFactory;
  engine::RandomNumberGenerator* mpRandomGenerator;
};

}}}
