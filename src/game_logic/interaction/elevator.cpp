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

#include "engine/collision_checker.hpp"
#include "engine/entity_tools.hpp"
#include "engine/physical_components.hpp"
#include "engine/visual_components.hpp"
#include "game_service_provider.hpp"

#include "global_level_events.hpp"

namespace ex = entityx;


namespace rigel { namespace game_logic { namespace interaction {

using engine::components::BoundingBox;
using engine::components::MovingBody;
using engine::components::SolidBody;
using engine::components::Sprite;
using engine::components::WorldPosition;


void configureElevator(entityx::Entity entity) {
  using namespace engine::components::parameter_aliases;
  using engine::components::ActivationSettings;

  entity.assign<components::Elevator>();
  entity.assign<BoundingBox>(BoundingBox{{0, 0}, {4, 3}});
  entity.assign<MovingBody>(Velocity{0.0f, 0.0f}, GravityAffected{true});
  entity.assign<ActivationSettings>(ActivationSettings::Policy::Always);
  entity.assign<SolidBody>();

  entity.component<Sprite>()->mFramesToRender.push_back(
    engine::IGNORE_RENDER_SLOT);
}


ElevatorSystem::ElevatorSystem(
  entityx::Entity player,
  IGameServiceProvider* pServiceProvider,
  engine::CollisionChecker* pCollisionChecker,
  entityx::EventManager* pEvents
)
  : mPlayer(player)
  , mpServiceProvider(pServiceProvider)
  , mpCollisionChecker(pCollisionChecker)
  , mpEvents(pEvents)
{
}


void ElevatorSystem::update(entityx::EntityManager& es) {
  auto attachableElevator = findAttachableElevator(es);

  const auto attachmentChanged =
    attachableElevator.valid() != mAttachedElevator.valid() ||
    attachableElevator != mAttachedElevator;

  if (attachmentChanged) {
    if (mAttachedElevator) {
      // detach old
      mAttachedElevator.component<Sprite>()->mFramesToRender.back() =
        engine::IGNORE_RENDER_SLOT;
      mAttachedElevator.component<MovingBody>()->mVelocity.y = 2.0f;
      mAttachedElevator.component<MovingBody>()->mGravityAffected = true;
      mAttachedElevator.invalidate();
    }

    mAttachedElevator = attachableElevator;

    if (attachableElevator) {
      mpEvents->emit(rigel::events::TutorialMessage{
        data::TutorialMessageId::FoundTurboLift});

      // attach new
      mAttachedElevator.component<MovingBody>()->mGravityAffected = false;
      mActiveElevatorPreviousPosition =
        mAttachedElevator.component<WorldPosition>()->y;
    }

    mpEvents->emit<events::ElevatorAttachmentChanged>(attachableElevator);
  }

  // Animation and sound
  if (mAttachedElevator) {
    auto& sprite = *mAttachedElevator.component<Sprite>();

    const auto newPosition = mAttachedElevator.component<WorldPosition>()->y;
    const auto movement = newPosition - mActiveElevatorPreviousPosition;
    mActiveElevatorPreviousPosition = newPosition;

    const auto offset = mIsOddFrame ? 1 : 0;
    if (movement < 0) {
      if (mIsOddFrame) {
        mpServiceProvider->playSound(data::SoundId::FlameThrowerShot);
      }

      sprite.mFramesToRender.back() = 1 + offset;
    } else if (movement > 0) {
      sprite.mFramesToRender.back() = engine::IGNORE_RENDER_SLOT;
    } else {
      const auto& position = *mAttachedElevator.component<WorldPosition>();
      const auto& bbox = *mAttachedElevator.component<BoundingBox>();
      if (!mpCollisionChecker->isOnSolidGround(position, bbox)) {
        sprite.mFramesToRender.back() = 3 + offset;
      }
    }
  }

  mIsOddFrame = !mIsOddFrame;
}


ex::Entity ElevatorSystem::findAttachableElevator(ex::EntityManager& es) const {
  ex::Entity attachableElevator;

  const auto& playerPos = *mPlayer.component<const WorldPosition>();
  const auto& playerBox = *mPlayer.component<const BoundingBox>();
  const auto leftAttachPoint = playerPos.x;
  const auto rightAttachPoint = playerPos.x + playerBox.size.width - 1;

  // TODO: This can be simplified now that the Player takes care of moving
  // the elevator

  // Note: We don't use the elvator's bounding box to check if we can attach,
  // but hardcoded values. This is because the bounding box will be modified
  // during elevator movement, which would throw off the attachment check.
  // Making the attachment check independent of the actual bounding box means
  // we can be more straightforward when updating the elevator's state.
  ex::ComponentHandle<WorldPosition> position;
  ex::ComponentHandle<components::Elevator> tag;
  for (auto entity : es.entities_with_components(position, tag)) {
    const auto top = position->y - 2;
    const auto left = position->x;
    const auto right = left + 3;
    if (
      playerPos.y + 1 == top &&
      leftAttachPoint >= left &&
      rightAttachPoint <= right
    ) {
      attachableElevator = entity;
      break;
    }
  }

  return attachableElevator;
}

}}}
