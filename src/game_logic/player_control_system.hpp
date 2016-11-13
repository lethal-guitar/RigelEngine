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

#include <base/spatial_types.hpp>
#include <base/grid.hpp>
#include <base/warnings.hpp>
#include <engine/base_components.hpp>
#include <engine/physics_system.hpp>
#include <engine/timing.hpp>

RIGEL_DISABLE_WARNINGS
#include <boost/optional.hpp>
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS


namespace rigel { namespace data { namespace map {
  class Map;
  class TileAttributes;
}}}

namespace rigel { namespace engine { namespace components {
  struct Physical;
  struct Sprite;
}}}


namespace rigel { namespace game_logic {

namespace detail {

enum class Orientation {
  None,
  Left,
  Right
};


enum class PlayerState {
  Standing,
  Walking,
  Crouching,
  LookingUp,
  ClimbingLadder,
  Airborne
};

}

struct PlayerInputState {
  bool mMovingLeft = false;
  bool mMovingRight = false;
  bool mMovingUp = false;
  bool mMovingDown = false;
  bool mJumping = false;
  bool mShooting = false;
};


namespace components {

struct PlayerControlled {
  detail::Orientation mOrientation = detail::Orientation::Left;
  detail::PlayerState mState = detail::PlayerState::Standing;

  bool mIsLookingUp = false;
  bool mIsLookingDown = false;

  bool mPerformedInteraction = false;
  bool mPerformedJump = false;
  bool mPerformedShot = false;
};


enum class InteractableType {
  Teleporter
};

struct Interactable {
  InteractableType mType;
};

}

namespace events {

struct PlayerInteraction {
  PlayerInteraction(entityx::Entity e, const components::InteractableType t)
    : mInteractedEntity(e)
    , mType(t)
  {
  }

  entityx::Entity mInteractedEntity;
  const components::InteractableType mType;
};


}


void initializePlayerEntity(entityx::Entity player, bool isFacingRight);


/** Takes inputs from player (e.g. keypresses, gamepad etc.) and controls the
 * avatar (Duke) accordingly.
 *
 */
class PlayerControlSystem : public entityx::System<PlayerControlSystem> {
public:
  PlayerControlSystem(
    entityx::Entity player,
    const PlayerInputState* pInputs,
    const data::map::Map& map,
    const data::map::TileAttributes& tileAttributes);

  void update(
    entityx::EntityManager& es,
    entityx::EventManager& events,
    entityx::TimeDelta dt
  ) override;

  void updateScrollOffset();

private:
  void updateAnimationStateAndBoundingBox(
    const components::PlayerControlled& state,
    engine::components::Sprite& sprite,
    engine::components::Physical& physical);

  boost::optional<base::Vector> findLadderTouchPoint(
    const engine::BoundingBox& worldSpacePlayerBounds
  ) const;

  bool canClimbUp(const engine::BoundingBox& worldSpacePlayerBounds) const;
  bool canClimbDown(const engine::BoundingBox& worldSpacePlayerBounds) const;

private:
  engine::TimeStepper mTimeStepper;
  const PlayerInputState* mpPlayerControlInput;
  entityx::Entity mPlayer;

  base::Grid<std::uint8_t> mLadderFlags;
};


class MapScrollSystem : public entityx::System<MapScrollSystem> {
public:
  MapScrollSystem(
    base::Vector* pScrollOffset,
    entityx::Entity player,
    const data::map::Map& map);

  void update(
    entityx::EntityManager& es,
    entityx::EventManager& events,
    entityx::TimeDelta dt
  ) override;

private:
  void updateScrollOffset(
    const components::PlayerControlled& state,
    const engine::components::WorldPosition& position,
    const engine::components::Physical& physical,
    entityx::TimeDelta dt);

private:
  engine::TimeStepper mTimeStepper;
  entityx::Entity mPlayer;
  base::Vector* mpScrollOffset;
  base::Extents mMaxScrollOffset;
};

}}
