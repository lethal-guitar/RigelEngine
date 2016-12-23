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
#include "game_logic/player_control_system.hpp"
#include "engine/visual_components.hpp"
#include "engine/timing.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

namespace rigel { struct IGameServiceProvider; }


namespace rigel { namespace game_logic { namespace player {

class AnimationSystem : public entityx::System<AnimationSystem> {
public:
  explicit AnimationSystem(
    entityx::Entity player,
    IGameServiceProvider* pServiceProvider);

  void update(
    entityx::EntityManager& es,
    entityx::EventManager& events,
    entityx::TimeDelta dt
  ) override;

private:
  void updateAnimation(
    const components::PlayerControlled& state,
    engine::components::Sprite& sprite);

  void updateMercyFramesAnimation(
    engine::TimeDelta mercyTimeElapsed,
    engine::components::Sprite& sprite);

  void updateDeathAnimation(
    components::PlayerControlled& state,
    engine::components::Sprite& sprite,
    engine::TimeDelta dt);

private:
  entityx::Entity mPlayer;
  IGameServiceProvider* mpServiceProvider;

  Orientation mPreviousOrientation;
  PlayerState mPreviousState;
};

}}}
