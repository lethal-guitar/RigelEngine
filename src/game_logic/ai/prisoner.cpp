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

#include "prisoner.hpp"

#include "engine/random_number_generator.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/damage_components.hpp"


namespace rigel { namespace game_logic { namespace ai {

using engine::components::Sprite;
using engine::components::WorldPosition;
using game_logic::components::Shootable;


PrisonerSystem::PrisonerSystem(
  entityx::Entity player,
  engine::RandomNumberGenerator* pRandomGenerator
)
  : mPlayer(player)
  , mpRandomGenerator(pRandomGenerator)
{
}


void PrisonerSystem::update(
  entityx::EntityManager& es,
  entityx::EventManager& events,
  entityx::TimeDelta dt
) {
  using engine::components::Active;

  mIsOddFrame = !mIsOddFrame;

  es.each<Sprite, WorldPosition, components::Prisoner, Active>(
    [this](
      entityx::Entity entity,
      Sprite& sprite,
      const WorldPosition& position,
      components::Prisoner& state,
      const engine::components::Active&
    ) {
      if (state.mIsAggressive) {
        updateAggressivePrisoner(entity, position, state, sprite);
      } else {
        const auto shakeIronBars = (mpRandomGenerator->gen() & 4) != 0;
        // The animation has two frames, 0 is "idle" and 1 is "shaking".
        sprite.mFramesToRender[0] = int{shakeIronBars};
      }
    });
}


void PrisonerSystem::updateAggressivePrisoner(
  entityx::Entity entity,
  const WorldPosition& position,
  components::Prisoner& state,
  Sprite& sprite
) {
  using game_logic::components::PlayerDamaging;
  using State = components::Prisoner::State;

  if (state.mState == State::Dieing) {
    ++state.mDeathAnimationStep;
    if (state.mDeathAnimationStep >= 6) {
      entity.destroy();
    }

    const auto canAdvance =
      state.mDeathAnimationStep == 2 ||
      state.mDeathAnimationStep == 3;
    if (canAdvance) {
      ++sprite.mFramesToRender[0];
    }
    return;
  }

  auto& shootable = *entity.component<Shootable>();

  // See if we want to grab
  if (state.mState == State::Idle) {
    // TODO: Adjust player position according to orientation to replicate
    // original positioning?
    const auto& playerPos = *mPlayer.component<WorldPosition>();
    const auto playerInRange =
      position.x - 4 < playerPos.x &&
      position.x + 7 > playerPos.x;

    if (playerInRange) {
      const auto wantsToGrab =
        (mpRandomGenerator->gen() & 0x10) != 0 && mIsOddFrame;
      if (wantsToGrab) {
        state.mState = State::Grabbing;
        state.mGrabStep = 0;
        sprite.mFramesToRender.push_back(1);
        shootable.mInvincible = false;
        entity.assign<PlayerDamaging>(1);
      }
    }
  }

  // If we decided to grab, we immediately update accordingly on the
  // same frame (this is how it works in the original game)
  if (state.mState == State::Grabbing) {
    sprite.mFramesToRender[1] = (state.mGrabStep + 1) % 5;

    if (state.mGrabStep >= 4) {
      state.mState = State::Idle;
      sprite.mFramesToRender.pop_back();
      shootable.mInvincible = true;
      entity.remove<PlayerDamaging>();
    }

    // Do this *after* checking whether the grab sequence is finished.
    // This is required in order to get exactly the same sequence as in the
    // original game.
    if (mIsOddFrame) {
      ++state.mGrabStep;
    }
  }
}


void PrisonerSystem::onEntityHit(entityx::Entity entity) {
  if (!entity.has_component<components::Prisoner>()) {
    return;
  }

  auto& sprite = *entity.component<Sprite>();
  auto& state = *entity.component<components::Prisoner>();

  if (state.mState == components::Prisoner::State::Grabbing) {
    sprite.mFramesToRender.pop_back();
  }

  state.mState = components::Prisoner::State::Dieing;
  sprite.mFramesToRender[0] = 5;

  entity.remove<Shootable>();
}

}}}
