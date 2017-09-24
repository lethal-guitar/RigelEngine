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

#include "messenger_drone.hpp"

#include "base/array_view.hpp"
#include "engine/life_time_components.hpp"
#include "engine/visual_components.hpp"

#include <array>


namespace rigel { namespace game_logic { namespace ai {

namespace {

struct MessageFrame {
  int mIndex;
  int mDuration;
};


const MessageFrame YOUR_BRAIN_IS_OURS[] = {
  {0, 11},
  {1, 11},
  {2, 11},
  {3, 14},
  {0, 11},
  {1, 11},
  {2, 11},
  {3, 15}
};

const MessageFrame BRING_BACK_THE_BRAIN[] = {
  {0, 9},
  {1, 9},
  {2, 9},
  {3, 15},
  {4, 2}, {5, 2}, {6, 2}, {7, 2}, {4, 2}, {5, 2}, {6, 2}, {7, 2},
  {4, 2}, {5, 2}, {6, 2}, {7, 2}, {4, 2}, {5, 2}, {6, 2}, {7, 2},
  {4, 2}, {5, 2}, {6, 2}, {7, 2}, {4, 2}, {5, 2}, {6, 2}, {7, 2},
  {4, 2}, {5, 2}, {6, 2}, {7, 2}, {4, 2}, {5, 2}, {6, 2}, {7, 2},
  {8, 14}
};

const MessageFrame LIVE_FROM_RIGEL[] = {
  {0, 5},
  {1, 5},
  {2, 4},
  {3, 7},
  {4, 4},
  {5, 6},
  {6, 16}
};

const MessageFrame DIE[] = {
  {0, 2},
  {1, 2},
  {2, 2},
  {3, 2},
  {4, 2},
  {5, 16},
};

const MessageFrame CANT_ESCAPE[] = {
  {0, 9},
  {1, 9},
  {2, 9},
  {3, 9},
  {4, 9},
  {5, 9},
  {6, 9},
};


const base::ArrayView<MessageFrame> MESSAGE_SEQUENCES[] = {
  YOUR_BRAIN_IS_OURS,
  BRING_BACK_THE_BRAIN,
  LIVE_FROM_RIGEL,
  DIE,
  CANT_ESCAPE
};

}


MessengerDroneSystem::MessengerDroneSystem(entityx::Entity player)
  : mPlayer(player)
{
}


void MessengerDroneSystem::update(
  entityx::EntityManager& es,
  entityx::EventManager& events,
  const entityx::TimeDelta dt
) {
  using namespace engine::components;
  using namespace engine::orientation;

  using State = components::MessengerDrone::State;

  const auto& playerPos = *mPlayer.component<WorldPosition>();

  es.each<Sprite, WorldPosition, components::MessengerDrone, Active>(
    [this, playerPos](
      entityx::Entity entity,
      Sprite& sprite,
      WorldPosition& position,
      components::MessengerDrone& state,
      const engine::components::Active&
    ) {
      const auto flyForward = [&state, &position]() {
        // The messenger drone has no collision detection, so we can move
        // directly without using physics/velocity.
        position.x += toMovement(state.mOrientation) * 2;
      };

      if (state.mState == State::AwaitActivation) {
        // Initialize on first activation to face player

        const auto playerIsLeft = playerPos.x < position.x;
        const auto exhaustStartFrame = playerIsLeft ? 8 : 6;

        state.mOrientation =
          playerIsLeft ? Orientation::Left : Orientation::Right;

        sprite.mFramesToRender = {
          0, // blank screen and frame
          playerIsLeft ? 1 : 2, // horizontal engine
          3, // vertical engine
          exhaustStartFrame // horizontal engine exhaust/flame
        };

        entity.assign<Animated>(
          1, exhaustStartFrame, exhaustStartFrame + 1, 3);

        state.mState = State::FlyIn;
      }

      if (state.mState == State::FlyIn) {
        flyForward();

        const auto playerCenterX = playerPos.x + 1;
        const auto droneCenterX = position.x + 3;

        if (std::abs(playerCenterX - droneCenterX) <= 6) {
          // Switch from horizontal engine to vertical engine (suspension in
          // mid-air instead of propulsion)
          sprite.mFramesToRender[3] = 4;
          entity.remove<Animated>();
          entity.assign<Animated>(1, 4, 5, 3);

          // Start showing message on screen, use 5th render slot (index 4)
          sprite.mFramesToRender.push_back(10);

          state.mMessageStep = 0;
          state.mElapsedFrames = 0;

          state.mState = State::ShowingMessage;
        }
      }

      if (state.mState == State::ShowingMessage) {
        const auto messageIndex = static_cast<int>(state.mMessage);
        const auto& messageSequence = MESSAGE_SEQUENCES[messageIndex];
        const auto& currentMessageFrame = messageSequence[state.mMessageStep];
        sprite.mFramesToRender[4] = 10 + currentMessageFrame.mIndex;

        ++state.mElapsedFrames;
        if (state.mElapsedFrames >= currentMessageFrame.mDuration) {
          state.mElapsedFrames = 0;
          ++state.mMessageStep;

          if (state.mMessageStep >= messageSequence.size()) {
            // Go back to blank screen
            sprite.mFramesToRender.pop_back();

            // Switch back to horizontal engine
            const auto exhaustStartFrame =
              state.mOrientation == Orientation::Left ? 8 : 6;
            sprite.mFramesToRender[3] = exhaustStartFrame;
            entity.remove<Animated>();
            entity.assign<Animated>(
              1, exhaustStartFrame, exhaustStartFrame + 1, 3);

            entity.assign<AutoDestroy>(
              AutoDestroy::Condition::OnLeavingActiveRegion);

            state.mState = State::FlyOut;
            // We want to have one frame of black screen, no motion - so return
            // early here to skip the movement
            return;
          }
        }
      }

      if (state.mState == State::FlyOut) {
        flyForward();
      }
    });
}

}}}
