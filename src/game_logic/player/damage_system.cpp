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

#include "damage_system.hpp"

#include "engine/base_components.hpp"
#include "engine/physical_components.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/player/components.hpp"


// Duke damage mercy frames:
//   Easy mode:
//     40 in total. First 30 are blinking (one frame show, one frame hide),
//     rest is flashing white at same frequency.
//
//   Medium mode:
//     30 in total. First 23/24 blinking?? (needs more capture analysis)
//
//   Hard mode:
//     20 in total. First 12 blinking?? (needs more analysis)
//
// Death animation: 42 frames total till fade-out.
//   17 frames for animation
//   22 explosion/particles
//   rest is waiting for fade-out


namespace rigel { namespace game_logic { namespace player {

namespace {

// TODO: Possibly tweak this value
const auto DEATH_JUMP_IMPULSE = -2.6f;


int mercyFramesForDifficulty(const data::Difficulty difficulty) {
  using data::Difficulty;

  switch (difficulty) {
    case Difficulty::Easy:
      return 40;

    case Difficulty::Medium:
      return 30;

    case Difficulty::Hard:
      return 20;
  }

  assert(false);
  return 40;
}

}


using engine::components::BoundingBox;
using engine::components::Physical;
using engine::components::WorldPosition;
using engine::toWorldSpace;
using game_logic::components::PlayerControlled;
using game_logic::components::PlayerDamaging;


DamageSystem::DamageSystem(
  entityx::Entity player,
  data::PlayerModel* pPlayerModel,
  IGameServiceProvider* pServiceProvider,
  const data::Difficulty difficulty
)
  : mPlayer(player)
  , mpPlayerModel(pPlayerModel)
  , mpServiceProvider(pServiceProvider)
  , mNumMercyFrames(mercyFramesForDifficulty(difficulty))
{
}


void DamageSystem::update(
  entityx::EntityManager& es,
  entityx::EventManager& events,
  entityx::TimeDelta dt
) {
  if (mpPlayerModel->mHealth <= 0) {
    return;
  }

  assert(mPlayer.has_component<BoundingBox>());
  assert(mPlayer.has_component<Physical>());
  assert(mPlayer.has_component<PlayerControlled>());
  assert(mPlayer.has_component<WorldPosition>());

  const auto& playerPosition = *mPlayer.component<WorldPosition>().get();
  const auto playerBBox = toWorldSpace(
    *mPlayer.component<BoundingBox>().get(), playerPosition);
  auto& playerState = *mPlayer.component<PlayerControlled>().get();

  const auto inMercyFrames =
    playerState.mMercyFramesTimeElapsed != boost::none;
  if (inMercyFrames) {
    auto& mercyTimeElapsed = *playerState.mMercyFramesTimeElapsed;
    mercyTimeElapsed += dt;

    const auto mercyFramesElapsed = static_cast<int>(
      engine::timeToGameFrames(mercyTimeElapsed));
    if (mercyFramesElapsed >= mNumMercyFrames) {
      playerState.mMercyFramesTimeElapsed = boost::none;
    }
  }

  es.each<PlayerDamaging, BoundingBox, WorldPosition>(
    [this, &playerBBox, &playerState, inMercyFrames](
      entityx::Entity,
      const PlayerDamaging& damage,
      const BoundingBox& boundingBox,
      const WorldPosition& position
    ) {
      const auto bbox = toWorldSpace(boundingBox, position);
      const auto hasCollision = bbox.intersects(playerBBox);
      const auto canTakeDamage = !inMercyFrames || damage.mIgnoreMercyFrames;

      if (hasCollision && canTakeDamage) {
        mpPlayerModel->mHealth =
          std::max(0, mpPlayerModel->mHealth - damage.mAmount);

        if (mpPlayerModel->mHealth > 0) {
          playerState.mMercyFramesTimeElapsed = 0.0;
          mpServiceProvider->playSound(data::SoundId::DukePain);
        } else {
          auto& physical = *mPlayer.component<Physical>().get();
          physical.mVelocity = {0.0f, DEATH_JUMP_IMPULSE};

          playerState.mState = PlayerState::Dieing;
          mpServiceProvider->playSound(data::SoundId::DukeDeath);
        }
      }
    });
}

}}}
