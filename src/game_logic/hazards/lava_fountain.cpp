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
#include "base/static_vector.hpp"
#include "common/game_service_provider.hpp"
#include "data/sound_ids.hpp"
#include "engine/physical_components.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/behavior_controller.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/global_dependencies.hpp"
#include "renderer/renderer.hpp"


namespace rigel::game_logic::behaviors
{

namespace
{

struct SequenceElement
{
  int mFrame;
  int mOffsetY;
};

using ElementList = base::static_vector<SequenceElement, 4>;

const auto ERUPTION_SEQUENCE = std::array<ElementList, 11>{
  {{SequenceElement{3, 0}},
   {SequenceElement{4, -3}, {1, 1}},
   {SequenceElement{5, -6}, {2, -2}, {0, 2}},
   {SequenceElement{3, -8}, {0, -4}, {1, 0}},
   {SequenceElement{4, -9}, {1, -5}, {2, -1}, {0, 3}},
   {SequenceElement{5, -10}, {2, -6}, {0, -2}, {1, 2}},
   {SequenceElement{3, -9}, {0, -5}, {1, -1}, {2, 3}},
   {SequenceElement{3, -8}, {0, -4}, {1, 0}},
   {SequenceElement{4, -6}, {1, -2}, {2, 2}},
   {SequenceElement{5, -3}, {2, 1}},
   {SequenceElement{3, 0}}}};

} // namespace


void LavaFountain::update(
  GlobalDependencies& d,
  GlobalState& s,
  const bool isOnScreen,
  entityx::Entity entity)
{
  using engine::components::ActivationSettings;

  if (!entity.has_component<engine::components::ExtendedFrameList>())
  {
    entity.assign<engine::components::ExtendedFrameList>();
    entity.component<engine::components::Sprite>()->mFramesToRender[0] =
      engine::IGNORE_RENDER_SLOT;
  }

  const auto previousSequenceIndex = std::holds_alternative<Erupting>(mState)
    ? std::optional{std::get<Erupting>(mState).mSequenceIndex}
    : std::optional<int>{};

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


  base::match(
    mState,
    [&, this](Waiting& state) {
      ++state.mFramesElapsed;
      if (state.mFramesElapsed == 15)
      {
        mState = Erupting{};
      }
    },

    [&, this](Erupting& state) {
      if (state.mSequenceIndex == 0)
      {
        entity.assign<game_logic::components::PlayerDamaging>(1);
      }
      if (state.mSequenceIndex == static_cast<int>(ERUPTION_SEQUENCE.size()))
      {
        resetBbox();
        entity.remove<game_logic::components::PlayerDamaging>();

        const auto stillOnScreen =
          isBboxOnScreen(s, engine::toWorldSpace(bbox, position));
        if (!stillOnScreen)
        {
          entity.component<ActivationSettings>()->mHasBeenActivated = false;
        }

        const auto initialElapsedFrames = stillOnScreen ? 1 : 0;
        mState = Waiting{initialElapsedFrames};
        return;
      }

      if (state.mSequenceIndex < 5)
      {
        d.mpServiceProvider->playSound(data::SoundId::LavaFountain);
      }

      updateBbox(ERUPTION_SEQUENCE[state.mSequenceIndex]);
      ++state.mSequenceIndex;
    });

  const auto currentSequenceIndex = std::holds_alternative<Erupting>(mState)
    ? std::optional{std::get<Erupting>(mState).mSequenceIndex}
    : std::optional<int>{};
  if (currentSequenceIndex != previousSequenceIndex)
  {
    auto& additionalFrames =
      entity.component<engine::components::ExtendedFrameList>()->mFrames;

    additionalFrames.clear();
    if (currentSequenceIndex && *currentSequenceIndex > 0)
    {
      const auto indexForDrawing = *currentSequenceIndex - 1;
      for (const auto& element : ERUPTION_SEQUENCE[indexForDrawing])
      {
        additionalFrames.push_back(
          {element.mFrame, base::Vec2{0, element.mOffsetY}});
      }
    }
  }
}

} // namespace rigel::game_logic::behaviors
