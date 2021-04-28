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
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <variant>


namespace rigel::game_logic
{
struct GlobalDependencies;
struct GlobalState;
} // namespace rigel::game_logic


namespace rigel::game_logic::behaviors
{

struct SlimeContainer
{
  void update(
    GlobalDependencies& dependencies,
    GlobalState& state,
    bool isOnScreen,
    entityx::Entity entity);

  void onKilled(
    GlobalDependencies& dependencies,
    GlobalState& state,
    const base::Point<float>& inflictorVelocity,
    entityx::Entity entity);

  int mBreakAnimationStep = 0;
};


struct SlimeBlob
{
  void update(
    GlobalDependencies& dependencies,
    GlobalState& state,
    bool isOnScreen,
    entityx::Entity entity);

  struct OnGround
  {
    bool mIsOddUpdate = false;
  };

  struct Idle
  {
    int mFramesElapsed = 0;
  };

  struct Ascending
  {
  };

  struct Descending
  {
  };

  struct OnCeiling
  {
    bool mIsOddUpdate = false;
  };

  using State = std::variant<OnGround, OnCeiling, Idle, Ascending, Descending>;

  State mState = Idle{};
  engine::components::Orientation mOrientation =
    engine::components::Orientation::Left;
};

} // namespace rigel::game_logic::behaviors
