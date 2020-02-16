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

#include "lava_fountain.hpp"

#include "base/match.hpp"
#include "common/game_service_provider.hpp"
#include "data/sound_ids.hpp"
#include "engine/physical_components.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/behavior_controller.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/global_dependencies.hpp"
#include "renderer/renderer.hpp"

RIGEL_DISABLE_WARNINGS
#include <boost/container/static_vector.hpp>
RIGEL_RESTORE_WARNINGS


namespace rigel::game_logic::behaviors {

namespace {

struct SequenceElement {
  int mFrame;
  int mOffsetY;
};

using ElementList = boost::container::static_vector<SequenceElement, 4>;

const auto ERUPTION_SEQUENCE = std::array<ElementList, 11>{{
  {{3, 0}},
  {{4, -3}, {1, 1}},
  {{5, -6}, {2, -2}, {0, 2}},
  {{3, -8}, {0, -4}, {1, 0}},
  {{4, -9}, {1, -5}, {2, -1}, {0, 3}},
  {{5, -10}, {2, -6}, {0, -2}, {1, 2}},
  {{3, -9}, {0, -5}, {1, -1}, {2, 3}},
  {{3, -8}, {0, -4}, {1, 0}},
  {{4, -6}, {1, -2}, {2, 2}},
  {{5, -3}, {2, 1}},
  {{3, 0}}
}};

}


void LavaFountain::update(
  GlobalDependencies& d,
  GlobalState& s,
  const bool isOnScreen,
  entityx::Entity entity
) {
  using engine::components::ActivationSettings;

  const auto& position = *entity.component<engine::components::WorldPosition>();
  auto& bbox = *entity.component<engine::components::BoundingBox>();

  auto updateBbox = [&](const ElementList& elements) {
    bbox.topLeft.y = elements.back().mOffsetY;
    bbox.size.height = 1 + static_cast<int>(elements.size() - 1) * 4;
  };

  auto resetBbox = [&]() {
    bbox.topLeft.y = 0;
    bbox.size.height = 4;
  };


  base::match(mState,
    [&, this](Waiting& state) {
      ++state.mFramesElapsed;
      if (state.mFramesElapsed == 15) {
        mState = Erupting{};
      }
    },

    [&, this](Erupting& state) {
      if (state.mSequenceIndex == 0) {
        entity.assign<game_logic::components::PlayerDamaging>(1);
      }
      if (state.mSequenceIndex == static_cast<int>(ERUPTION_SEQUENCE.size())) {
        resetBbox();
        entity.remove<game_logic::components::PlayerDamaging>();

        const auto stillOnScreen =
          isBboxOnScreen(s, engine::toWorldSpace(bbox, position));
        if (!stillOnScreen) {
          entity.component<ActivationSettings>()->mHasBeenActivated = false;
        }

        const auto initialElapsedFrames = stillOnScreen ? 1 : 0;
        mState = Waiting{initialElapsedFrames};
        return;
      }

      if (state.mSequenceIndex < 5) {
        d.mpServiceProvider->playSound(data::SoundId::LavaFountain);
      }

      updateBbox(ERUPTION_SEQUENCE[state.mSequenceIndex]);
      ++state.mSequenceIndex;
    });
}


void LavaFountain::render(
  renderer::Renderer* pRenderer,
  entityx::Entity entity,
  const engine::components::Sprite& sprite,
  const base::Vector& positionInScreenSpace
) {
  const auto& controller =
    entity.component<components::BehaviorController>()->get<LavaFountain>();

  if (
    auto pState = std::get_if<Erupting>(&controller.mState);
    pState && pState->mSequenceIndex > 0
  ) {
    const auto indexForDrawing = pState->mSequenceIndex - 1;
    for (const auto& element : ERUPTION_SEQUENCE[indexForDrawing]) {
      engine::drawSpriteFrame(
        sprite.mpDrawData->mFrames[element.mFrame],
        positionInScreenSpace + base::Vector{0, element.mOffsetY},
        pRenderer);
    }
  }
}

}
