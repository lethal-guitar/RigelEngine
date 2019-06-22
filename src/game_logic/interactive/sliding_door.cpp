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
#include "engine/base_components.hpp"
#include "engine/entity_tools.hpp"
#include "engine/physical_components.hpp"

#include <algorithm>


namespace rigel::game_logic { namespace ai {

using engine::components::Sprite;
using engine::components::WorldPosition;


namespace {

const auto HORIZONTAL_DOOR_RANGE = base::Rect<int>{{-2, -2}, {8, 9}};
const auto VERTICAL_DOOR_RANGE = base::Rect<int>{{-8, -6}, {15, 7}};


bool playerInRange(
  const WorldPosition& playerPos,
  const WorldPosition& doorPos,
  const base::Rect<int>& doorRange
) {
  const auto worldSpaceDoorRange = doorRange + doorPos;
  return worldSpaceDoorRange.containsPoint(playerPos);
}

}


namespace horizontal {

namespace {

using State = components::HorizontalSlidingDoor::State;


State nextState(const State currentState, const bool playerInRange) {
  auto newState = currentState;

  switch (currentState) {
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
}

}} // namespace horizontal


namespace vertical {

namespace {

constexpr auto NUM_DOOR_SEGMENTS = 8;

using State = components::VerticalSlidingDoor::State;


State nextState(
  const components::VerticalSlidingDoor& state,
  const bool playerInRange
) {
  auto newState = state.mState;

  switch (state.mState) {
    case State::Closed:
      if (playerInRange) {
        newState = State::Opening;
      }
      break;

    case State::Opening:
      if (!playerInRange) {
        newState = State::Closing;
      } else if (state.mSlideStep >= 7) {
        newState = State::Open;
      }
      break;

    case State::Closing:
      if (playerInRange) {
        newState = State::Opening;
      } else if (state.mSlideStep <= 0) {
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
}


int stepChangeForState(const State state) {
  if (state == vertical::State::Opening) {
    return 1;
  } else if (state == vertical::State::Closing) {
    return -1;
  }

  return 0;
}

}} // namespace vertical


SlidingDoorSystem::SlidingDoorSystem(
  entityx::Entity playerEntity,
  IGameServiceProvider* pServiceProvider
)
  : mPlayerEntity(playerEntity)
  , mpServiceProvider(pServiceProvider)
{
}


void SlidingDoorSystem::update(entityx::EntityManager& es) {
  const auto& playerPosition = *mPlayerEntity.component<WorldPosition>().get();

  using engine::components::BoundingBox;
  using engine::components::SolidBody;
  using components::HorizontalSlidingDoor;
  es.each<WorldPosition, BoundingBox, Sprite, HorizontalSlidingDoor>(
    [this, &playerPosition, &es](
      entityx::Entity entity,
      const WorldPosition& position,
      BoundingBox& boundingBox,
      Sprite& sprite,
      HorizontalSlidingDoor& state
    ) {
      if (!state.mCollisionHelper) {
        auto collisionHelper = es.create();
        collisionHelper.assign<BoundingBox>(BoundingBox{{}, {1, 1}});
        collisionHelper.assign<WorldPosition>(position);
        collisionHelper.assign<engine::components::Active>();
        collisionHelper.assign<SolidBody>();
        state.mCollisionHelper = collisionHelper;
      }

      const auto inRange =
        playerInRange(playerPosition, position, HORIZONTAL_DOOR_RANGE);
      const auto previousState = state.mState;
      state.mState = horizontal::nextState(previousState, inRange);

      if (state.mState == horizontal::State::Closed) {
        boundingBox.topLeft.x = 0;
        boundingBox.size.width = 6;
      } else {
        boundingBox.topLeft.x = 5;
        boundingBox.size.width = 1;
      }

      const auto missingLeftEdgeCollision =
        previousState == horizontal::State::Closed &&
        state.mState == horizontal::State::HalfOpen;
      engine::setTag<SolidBody>(
        state.mCollisionHelper, !missingLeftEdgeCollision);

      sprite.mFramesToRender[0] = static_cast<int>(state.mState);
      updateSoundGeneration(inRange, state);
    });

  es.each<WorldPosition, BoundingBox, components::VerticalSlidingDoor>(
    [this, &playerPosition](
      entityx::Entity entity,
      const WorldPosition& position,
      BoundingBox& boundingBox,
      components::VerticalSlidingDoor& state
    ) {
      const auto inRange =
        playerInRange(playerPosition, position, VERTICAL_DOOR_RANGE);
      state.mState = vertical::nextState(state, inRange);

      const auto stepChange = vertical::stepChangeForState(state.mState);
      state.mSlideStep = std::clamp(state.mSlideStep + stepChange, 0, 7);

      if (state.mState == vertical::State::Closed) {
        boundingBox.topLeft.y = 0;
        boundingBox.size.height = 8;
      } else {
        boundingBox.topLeft.y = -7;
        boundingBox.size.height = 1;
      }

      updateSoundGeneration(inRange, state);
    });
}


template<typename StateT>
void SlidingDoorSystem::updateSoundGeneration(
  const bool inRange,
  StateT& state
) {
  if (inRange != state.mPlayerWasInRange) {
    mpServiceProvider->playSound(data::SoundId::SlidingDoor);
    state.mPlayerWasInRange = inRange;
  }
}

} // namespace ai


void renderVerticalSlidingDoor(
  renderer::Renderer* pRenderer,
  entityx::Entity entity,
  const engine::components::Sprite& sprite,
  const base::Vector& positionInScreenSpace
) {
  using namespace ai::vertical;

  const auto& state = *entity.component<ai::components::VerticalSlidingDoor>();

  const auto segmentsToDraw =
    NUM_DOOR_SEGMENTS - std::max(0, state.mSlideStep - 1);

  for (int i = 0; i < segmentsToDraw; ++i) {
    const auto segmentIndex = NUM_DOOR_SEGMENTS - i - state.mSlideStep;
    engine::drawSpriteFrame(
      sprite.mpDrawData->mFrames[segmentIndex],
      positionInScreenSpace - base::Vector{0, NUM_DOOR_SEGMENTS - i},
      pRenderer);
  }
}

}
