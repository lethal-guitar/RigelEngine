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

#include "sliding_door.hpp"

#include "common/game_service_provider.hpp"
#include "data/game_traits.hpp"
#include "data/unit_conversions.hpp"
#include "engine/entity_tools.hpp"
#include "engine/physical_components.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/behavior_controller.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/player.hpp"

#include <algorithm>


namespace rigel::game_logic::behaviors {

using engine::components::Sprite;
using engine::components::WorldPosition;


namespace {

constexpr auto HORIZONTAL_DOOR_RANGE = base::Rect<int>{{-2, -2}, {8, 9}};
constexpr auto VERTICAL_DOOR_RANGE = base::Rect<int>{{-8, -6}, {15, 7}};
constexpr auto NUM_VERTICAL_DOOR_SEGMENTS = 8;


bool playerInRange(
  const WorldPosition& playerPos,
  const WorldPosition& doorPos,
  const base::Rect<int>& doorRange
) {
  const auto worldSpaceDoorRange = doorRange + doorPos;
  return worldSpaceDoorRange.containsPoint(playerPos);
}

}


void HorizontalSlidingDoor::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool,
  entityx::Entity entity
) {
  using engine::components::BoundingBox;
  using engine::components::SolidBody;

  auto nextState = [&](const bool playerInRange) {
    auto newState = mState;

    switch (mState) {
      case State::Closed:
        if (playerInRange) {
          newState = State::HalfOpen;
        }
        break;

      case State::HalfOpen:
        newState = playerInRange ? State::Open : State::Closed;
        break;

      case State::Open:
        if (!playerInRange) {
          newState = State::HalfOpen;
        }
        break;
    }

    return newState;
  };


  const auto& position = *entity.component<WorldPosition>();
  const auto& playerPosition = s.mpPlayer->orientedPosition();
  auto& boundingBox = *entity.component<BoundingBox>();

  if (!mCollisionHelper) {
    auto collisionHelper = d.mpEntityManager->create();
    collisionHelper.assign<BoundingBox>(BoundingBox{{}, {1, 1}});
    collisionHelper.assign<WorldPosition>(position);
    collisionHelper.assign<engine::components::Active>();
    collisionHelper.assign<SolidBody>();
    mCollisionHelper = collisionHelper;
  }

  const auto inRange =
    playerInRange(playerPosition, position, HORIZONTAL_DOOR_RANGE);
  const auto previousState = mState;
  mState = nextState(inRange);

  if (mState == State::Closed) {
    boundingBox.topLeft.x = 0;
    boundingBox.size.width = 6;
  } else {
    boundingBox.topLeft.x = 5;
    boundingBox.size.width = 1;
  }

  const auto missingLeftEdgeCollision =
    previousState == State::Closed &&
    mState == State::HalfOpen;
  engine::setTag<SolidBody>(
    mCollisionHelper, !missingLeftEdgeCollision);

  auto& sprite = *entity.component<engine::components::Sprite>();
  sprite.mFramesToRender[0] = static_cast<int>(mState);

  if (inRange != mPlayerWasInRange) {
    d.mpServiceProvider->playSound(data::SoundId::SlidingDoor);
    mPlayerWasInRange = inRange;
  }
}


void VerticalSlidingDoor::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool,
  entityx::Entity entity
) {
  using engine::components::BoundingBox;

  auto updateSprite = [&]() {
    auto& additionalFrames =
      entity.component<engine::components::ExtendedFrameList>()->mFrames;

    const auto segmentsToDraw =
      NUM_VERTICAL_DOOR_SEGMENTS - std::max(0, mSlideStep - 1);

    additionalFrames.clear();
    for (int i = 0; i < segmentsToDraw; ++i) {
      const auto segmentIndex = NUM_VERTICAL_DOOR_SEGMENTS - i - mSlideStep;
      additionalFrames.push_back({
        segmentIndex,
        base::Vector{0, -(NUM_VERTICAL_DOOR_SEGMENTS - i)}});
    }
  };

  auto nextState = [&](const bool playerInRange) {
    auto newState = mState;

    switch (mState) {
      case State::Closed:
        if (playerInRange) {
          newState = State::Opening;
        }
        break;

      case State::Opening:
        if (!playerInRange) {
          newState = State::Closing;
        } else if (mSlideStep >= 7) {
          newState = State::Open;
        }
        break;

      case State::Closing:
        if (playerInRange) {
          newState = State::Opening;
        } else if (mSlideStep <= 0) {
          newState = State::Closed;
        }
        break;

      case State::Open:
        if (!playerInRange) {
          newState = State::Closing;
        }
        break;
    }

    return newState;
  };

  auto stepChange = [&]() {
    if (mState == State::Opening) {
      return 1;
    } else if (mState == State::Closing) {
      return -1;
    }

    return 0;
  };


  if (!entity.has_component<engine::components::ExtendedFrameList>()) {
    entity.assign<engine::components::ExtendedFrameList>();
    entity.component<engine::components::Sprite>()->mFramesToRender[0] =
      engine::IGNORE_RENDER_SLOT;
    updateSprite();
  }

  const auto& position = *entity.component<WorldPosition>();
  const auto& playerPosition = s.mpPlayer->orientedPosition();
  auto& boundingBox = *entity.component<BoundingBox>();

  const auto inRange =
    playerInRange(playerPosition, position, VERTICAL_DOOR_RANGE);
  mState = nextState(inRange);

  if (mState == State::Closed) {
    boundingBox.topLeft.y = 0;
    boundingBox.size.height = 8;
  } else {
    boundingBox.topLeft.y = -7;
    boundingBox.size.height = 1;
  }

  if (inRange != mPlayerWasInRange) {
    d.mpServiceProvider->playSound(data::SoundId::SlidingDoor);
    mPlayerWasInRange = inRange;
  }

  const auto previousSlideStep = mSlideStep;
  mSlideStep = std::clamp(mSlideStep + stepChange(), 0, 7);

  if (mSlideStep != previousSlideStep) {
    updateSprite();
  }
}

}
