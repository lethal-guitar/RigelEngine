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

#include "boss_episode_3.hpp"

#include "base/math_tools.hpp"
#include "common/game_service_provider.hpp"
#include "engine/physical_components.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/ientity_factory.hpp"
#include "game_logic/player.hpp"


namespace ec = rigel::engine::components;


namespace rigel::game_logic::behaviors
{

namespace
{

struct AttackArea
{
  base::Vec2 mBboxOffset;
  base::Vec2 mShotOffset;
  data::ActorID mActorId;
};


// clang-format off
constexpr AttackArea ATTACK_AREAS[] = {
  {{-9,  0}, {-4, -4}, data::ActorID::Enemy_rocket_left},
  {{ 9,  0}, { 8, -4}, data::ActorID::Enemy_rocket_right},
  {{ 0, -9}, { 4, -8}, data::ActorID::Enemy_rocket_2_up},
  {{ 0,  9}, { 4,  3}, data::ActorID::Enemy_rocket_2_down},
};
// clang-format on


constexpr auto PLAYER_TARGET_OFFSET = base::Vec2{3, -1};
constexpr auto BOSS_OFFSET_TO_CENTER = base::Vec2{4, -4};

} // namespace


void BossEpisode3::update(
  GlobalDependencies& d,
  GlobalState& s,
  const bool isOnScreen,
  entityx::Entity entity)
{
  if (!mHasBeenSighted)
  {
    d.mpEvents->emit(rigel::events::BossActivated{entity});
    mHasBeenSighted = true;
  }

  auto& position = *entity.component<ec::WorldPosition>();
  const auto& playerPos = s.mpPlayer->orientedPosition();

  // Move towards player
  const auto vecToPlayer =
    (playerPos + PLAYER_TARGET_OFFSET) - (position + BOSS_OFFSET_TO_CENTER);
  if (d.mpRandomGenerator->gen() % 2 != 0)
  {
    position.x += base::sgn(vecToPlayer.x);
  }
  if (s.mpPerFrameState->mIsOddFrame)
  {
    position.y += base::sgn(vecToPlayer.y);
  }

  // Fire rockets

  // clang-format off
  if (
    isOnScreen &&
    s.mpPerFrameState->mIsOddFrame &&
    d.mpRandomGenerator->gen() % 2 != 0)
  // clang-format on
  {
    const auto playerBbox = s.mpPlayer->worldSpaceHitBox();
    for (const auto& area : ATTACK_AREAS)
    {
      auto attackRangeBbox =
        engine::toWorldSpace(*entity.component<ec::BoundingBox>(), position);
      attackRangeBbox.topLeft += area.mBboxOffset;

      if (attackRangeBbox.intersects(playerBbox))
      {
        d.mpServiceProvider->playSound(data::SoundId::FlameThrowerShot);
        d.mpEntityFactory->spawnActor(
          area.mActorId, position + area.mShotOffset);
      }
    }
  }
}


void BossEpisode3::onKilled(
  GlobalDependencies& d,
  GlobalState&,
  const base::Vec2T<float>&,
  entityx::Entity entity)
{
  d.mpEvents->emit(rigel::events::BossDestroyed{entity});
}

} // namespace rigel::game_logic::behaviors
