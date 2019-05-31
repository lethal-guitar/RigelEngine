/* Copyright (C) 2018, Nikolai Wuttke. All rights reserved.
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

#include "locked_door.hpp"

#include "engine/visual_components.hpp"
#include "game_logic/actor_tag.hpp"
#include "game_logic/player/components.hpp"

namespace ex = entityx;


namespace rigel { namespace game_logic { namespace interaction {

using namespace engine::components;
using namespace game_logic::components;

namespace {

constexpr int KEYHOLE_ANIMATION[] = {0, 1, 2, 3, 4, 3, 2, 1};
constexpr int KEYHOLE_UNLOCKED_FRAME = 5;

}


void configureLockedDoor(
  entityx::Entity entity,
  const int spawnIndex,
  const BoundingBox& boundingBox
) {
  entity.assign<ActorTag>(ActorTag::Type::Door, spawnIndex);
  entity.assign<BoundingBox>(boundingBox);
}


void configureKeyHole(
  entityx::Entity entity,
  const BoundingBox& boundingBox
) {
  entity.assign<Interactable>(InteractableType::KeyHole);
  entity.assign<BoundingBox>(boundingBox);
  entity.assign<AnimationSequence>(KEYHOLE_ANIMATION, 0, true);
}


void disableKeyHole(entityx::Entity entity) {
  entity.remove<Interactable>();
  entity.remove<BoundingBox>();
  entity.remove<AnimationSequence>();

  entity.component<Sprite>()->mFramesToRender[0] = KEYHOLE_UNLOCKED_FRAME;
}

}}}
