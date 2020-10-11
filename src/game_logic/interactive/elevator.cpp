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

#include "elevator.hpp"

#include "common/game_service_provider.hpp"
#include "common/global.hpp"
#include "engine/collision_checker.hpp"
#include "engine/entity_tools.hpp"
#include "engine/physical_components.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/actor_tag.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/player.hpp"

namespace ex = entityx;


namespace rigel::game_logic::behaviors {

void Elevator::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool,
  ex::Entity entity
) {
  using engine::components::BoundingBox;
  using engine::components::MovingBody;
  using engine::components::Sprite;
  using engine::components::WorldPosition;
  using game_logic::components::ActorTag;

  const auto& position = *entity.component<WorldPosition>();
  const auto& bbox = *entity.component<BoundingBox>();
  auto& sprite = *entity.component<Sprite>();

  auto isPlayerInRange = [&]() {
    const auto& playerBbox = s.mpPlayer->worldSpaceCollisionBox();
    const auto elevatorBbox = engine::toWorldSpace(bbox, position);

    return
      playerBbox.bottom() + 1 == elevatorBbox.top() &&
      playerBbox.left() >= elevatorBbox.left() &&
      playerBbox.right() <= elevatorBbox.right();
  };

  auto attach = [&]() {
    entity.component<MovingBody>()->mGravityAffected = false;
    entity.assign<ActorTag>(ActorTag::Type::ActiveElevator);
    mState = State{position.y};

    d.mpEvents->emit(rigel::events::TutorialMessage{
      data::TutorialMessageId::FoundTurboLift});
    d.mpEvents->emit(events::ElevatorAttachmentChanged{
      entity,
      events::ElevatorAttachmentChanged::ChangeType::Attach});
  };

  auto detach = [&]() {
    sprite.mFramesToRender.back() = engine::IGNORE_RENDER_SLOT;
    entity.component<MovingBody>()->mVelocity.y = 2.0f;
    entity.component<MovingBody>()->mGravityAffected = true;
    entity.remove<ActorTag>();
    mState.reset();

    d.mpEvents->emit(events::ElevatorAttachmentChanged{
      entity,
      events::ElevatorAttachmentChanged::ChangeType::Detach});
  };


  const auto playerInRange = isPlayerInRange();
  if (playerInRange && !mState) {
    attach();
  }

  if (!playerInRange && mState) {
    detach();
  }

  if (!mState) {
    // We are not attached to the player, nothing to do.
    return;
  }

  const auto movement = position.y - mState->mPreviousPosY;
  mState->mPreviousPosY = position.y;

  const auto offset = s.mpPerFrameState->mIsOddFrame ? 1 : 0;
  if (movement < 0) {
    if (s.mpPerFrameState->mIsOddFrame) {
      d.mpServiceProvider->playSound(data::SoundId::FlameThrowerShot);
    }

    sprite.mFramesToRender.back() = 1 + offset;
  } else if (movement > 0) {
    sprite.mFramesToRender.back() = engine::IGNORE_RENDER_SLOT;
  } else {
    if (!d.mpCollisionChecker->isOnSolidGround(position, bbox)) {
      sprite.mFramesToRender.back() = 3 + offset;
    }
  }
}

}
