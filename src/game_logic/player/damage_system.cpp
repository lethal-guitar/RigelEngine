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

  const auto& playerPosition = *mPlayer.component<WorldPosition>();
  const auto playerBBox = toWorldSpace(
    *mPlayer.component<BoundingBox>(), playerPosition);
  auto& playerState = *mPlayer.component<PlayerControlled>();

  es.each<PlayerDamaging, BoundingBox, WorldPosition>(
    [this, &playerBBox, &playerState](
      entityx::Entity entity,
      const PlayerDamaging& damage,
      const BoundingBox& boundingBox,
      const WorldPosition& position
    ) {
      const auto bbox = toWorldSpace(boundingBox, position);
      const auto hasCollision = bbox.intersects(playerBBox);
      const auto canTakeDamage =
        !playerState.isInMercyFrames() || damage.mIgnoreMercyFrames;

      if (hasCollision && canTakeDamage) {
        mpPlayerModel->mHealth =
          std::max(0, mpPlayerModel->mHealth - damage.mAmount);

        if (mpPlayerModel->mHealth > 0) {
          playerState.mMercyFramesRemaining = mNumMercyFrames;
          mpServiceProvider->playSound(data::SoundId::DukePain);
        } else {
          auto& physical = *mPlayer.component<Physical>();
          physical.mVelocity = {0.0f, DEATH_JUMP_IMPULSE};

          playerState.mState = PlayerState::Dieing;
          mpServiceProvider->playSound(data::SoundId::DukeDeath);
        }

        if (damage.mDestroyOnContact) {
          entity.destroy();
        }
      }
    });
}

}}}
