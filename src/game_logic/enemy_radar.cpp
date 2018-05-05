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

#include "enemy_radar.hpp"

#include "engine/visual_components.hpp"

#include <array>


namespace ex = entityx;

namespace rigel { namespace game_logic {

namespace {

constexpr auto NUM_ANIMATION_STEPS = 29;


constexpr std::array<int, NUM_ANIMATION_STEPS> DISHES_FUNCTIONAL_SEQUENCE = {
  4, 4, 4, 0, 4, 4, 4, 0, 4, 4, 4, 0,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5
};


constexpr std::array<int, NUM_ANIMATION_STEPS> DISHES_DESTROYED_SEQUENCE = {
  6, 6, 6, 0, 6, 6, 6, 0, 6, 6, 6, 0,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};


// When the animation sequence is currently showing frame 5, the number of
// functional radar dishes is additionally shown
constexpr auto SHOW_COUNT_FRAME = 5;


// The number of functional radar dishes shown on the display is represented by
// frames 8 to 16, with frame 8 being the number '1'. Therefore, by adding
// the number of functional dishes to 7, we get the right frame to show.
constexpr auto DISH_COUNT_BASE_FRAME = 7;

}


RadarDishCounter::RadarDishCounter(
  ex::EntityManager& entities,
  ex::EventManager& events)
{
  events.subscribe<ex::ComponentAddedEvent<components::RadarDish>>(*this);
  events.subscribe<ex::ComponentRemovedEvent<components::RadarDish>>(*this);
}


int RadarDishCounter::numRadarDishes() const {
  return mNumRadarDishes;
}


bool RadarDishCounter::radarDishesPresent() const {
  return mNumRadarDishes != 0;
}


void RadarDishCounter::receive(
  const ex::ComponentAddedEvent<components::RadarDish>& event
) {
  ++mNumRadarDishes;
}


void RadarDishCounter::receive(
  const ex::ComponentRemovedEvent<components::RadarDish>& event
) {
  --mNumRadarDishes;
}


RadarComputerSystem::RadarComputerSystem(const RadarDishCounter* pCounter)
  : mpCounter(pCounter)
{
}


void RadarComputerSystem::update(entityx::EntityManager& es) {
  using components::RadarComputer;
  using engine::components::Sprite;

  if (mIsOddFrame) {
    es.each<RadarComputer, Sprite>(
      [this](entityx::Entity, RadarComputer& state, Sprite& sprite) {
        ++state.mAnimationStep;
        if (state.mAnimationStep >= NUM_ANIMATION_STEPS) {
          state.mAnimationStep = 0;
        }

        const auto previousFrame = sprite.mFramesToRender[0];

        const auto& sequence = mpCounter->radarDishesPresent()
          ? DISHES_FUNCTIONAL_SEQUENCE
          : DISHES_DESTROYED_SEQUENCE;

        const auto newFrame = sequence[state.mAnimationStep];

        sprite.mFramesToRender[0] = newFrame;

        if (previousFrame != SHOW_COUNT_FRAME && newFrame == SHOW_COUNT_FRAME) {
          sprite.mFramesToRender.push_back(DISH_COUNT_BASE_FRAME);
        }

        if (previousFrame == SHOW_COUNT_FRAME && newFrame != SHOW_COUNT_FRAME) {
          sprite.mFramesToRender.pop_back();
        }

        if (newFrame == SHOW_COUNT_FRAME) {
          const auto dishCountFrame =
            DISH_COUNT_BASE_FRAME + mpCounter->numRadarDishes();
          sprite.mFramesToRender.back() = dishCountFrame;
        }
      });
  }

  mIsOddFrame = !mIsOddFrame;
}

}}
