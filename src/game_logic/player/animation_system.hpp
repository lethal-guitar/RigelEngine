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
#include "game_logic/player/components.hpp"
#include "engine/timing.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <array>

namespace rigel { struct IGameServiceProvider; }
namespace rigel { namespace game_logic { class EntityFactory; }}


namespace rigel { namespace game_logic { namespace player {

class AnimationSystem : public entityx::System<AnimationSystem> {
public:
  explicit AnimationSystem(
    entityx::Entity player,
    IGameServiceProvider* pServiceProvider,
    EntityFactory* pFactory);

  void update(
    entityx::EntityManager& es,
    entityx::EventManager& events,
    entityx::TimeDelta dt
  ) override;

private:
  int determineAnimationFrame(
    components::PlayerControlled& state,
    engine::TimeDelta dt,
    int currentAnimationFrame);

  int movementAnimationFrame(
    components::PlayerControlled& state,
    int currentAnimationFrame);

  int attackAnimationFrame(
    components::PlayerControlled& state,
    engine::TimeDelta dt,
    int currentAnimationFrame);

private:
  entityx::Entity mPlayer;
  IGameServiceProvider* mpServiceProvider;
  EntityFactory* mpEntityFactory;

  boost::optional<engine::TimeDelta> mElapsedForShotAnimation;
  entityx::Entity mMuzzleFlashEntity;

  PlayerState mPreviousState;
};

}}}
