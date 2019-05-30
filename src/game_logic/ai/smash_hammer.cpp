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

#include "smash_hammer.hpp"

#include "base/match.hpp"
#include "common/game_service_provider.hpp"
#include "data/game_traits.hpp"
#include "data/unit_conversions.hpp"
#include "engine/movement.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/behavior_controller.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/entity_factory.hpp"
#include "renderer/renderer.hpp"


namespace rigel::game_logic::behaviors {

void SmashHammer::update(
  GlobalDependencies& d,
  GlobalState& s,
  const bool isOnScreen,
  entityx::Entity entity
) {
  using namespace smash_hammer;

  auto& position = *entity.component<engine::components::WorldPosition>();

  base::match(mState,
    [&, this](Waiting& state) {
      if (state.mFramesElapsed == 0 && !isOnScreen) {
        return;
      }

      ++state.mFramesElapsed;
      if (state.mFramesElapsed == 19) {
        mState = PushingDown{};
      }
    },

    [&, this](const PushingDown&) {
      if (mExtensionStep == 0) {
        entity.assign<components::PlayerDamaging>(1);
      }

      const auto result =
        engine::moveVertically(*d.mpCollisionChecker, entity, 1);
      if (result != engine::MovementResult::Completed) {
        d.mpServiceProvider->playSound(data::SoundId::HammerSmash);
        spawnOneShotSprite(
          *d.mpEntityFactory, 84, position + base::Vector{0, 4});
        mState = PullingUp{};
      } else {
        ++mExtensionStep;
      }
    },

    [&, this](const PullingUp&) {
      --position.y;
      --mExtensionStep;
      if (mExtensionStep == 0) {
        entity.remove<components::PlayerDamaging>();
        mState = Waiting{};
      }
    }
  );
}


void SmashHammer::render(
  renderer::Renderer* pRenderer,
  entityx::Entity entity,
  const engine::components::Sprite& sprite,
  const base::Vector& positionInScreenSpace
) {
  using namespace smash_hammer;

  const auto& state =
    entity.component<components::BehaviorController>()->get<SmashHammer>();
  const auto pDrawData = sprite.mpDrawData;

  // Hammer
  engine::drawSpriteFrame(
    pDrawData->mFrames[0], positionInScreenSpace, pRenderer);

  // Mounting pole
  for (int i = 0; i < state.mExtensionStep; ++i) {
    engine::drawSpriteFrame(
      pDrawData->mFrames[1],
      positionInScreenSpace - base::Vector{0, i},
      pRenderer);
  }
}

}
