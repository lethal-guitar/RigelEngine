/* Copyright (C) 2019, Nikolai Wuttke. All rights reserved.
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

#include "base/spatial_types.hpp"
#include "base/warnings.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <variant>


namespace rigel::engine::events
{
struct CollidedWithWorld;
}
namespace rigel::game_logic
{
struct GlobalDependencies;
struct GlobalState;
} // namespace rigel::game_logic


namespace rigel::game_logic::behaviors
{

struct BossEpisode2
{
  struct WarmingUp
  {
    int mFramesElapsed = 0;
  };

  struct FlyingRight
  {
    int mFramesElapsed = 0;
  };

  struct FlyingLeft
  {
    int mFramesElapsed = 0;
  };

  struct FallingDown
  {
  };

  struct JumpingRight
  {
    int mFramesElapsed = 0;
    int mJumpsCompleted = 0;
  };

  struct JumpingLeft
  {
    int mFramesElapsed = 0;
    int mJumpsCompleted = 0;
  };

  struct RisingUp
  {
  };

  using State = std::variant<
    WarmingUp,
    FlyingRight,
    FlyingLeft,
    FallingDown,
    JumpingRight,
    JumpingLeft,
    RisingUp>;

  void update(
    GlobalDependencies& dependencies,
    GlobalState& state,
    bool isOnScreen,
    entityx::Entity entity);

  void onCollision(
    GlobalDependencies& d,
    GlobalState&,
    const engine::events::CollidedWithWorld&,
    entityx::Entity entity);

  void onKilled(
    GlobalDependencies& dependencies,
    GlobalState& state,
    const base::Vec2f& inflictorVelocity,
    entityx::Entity entity);

  State mState = WarmingUp{};
  int mStartingHeight = 0;
  int mCoolDownFrames = 0;
  bool mDestructionPending = false;
};

} // namespace rigel::game_logic::behaviors
