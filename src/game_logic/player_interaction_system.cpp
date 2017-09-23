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

#include "player_interaction_system.hpp"

#include "data/player_data.hpp"
#include "engine/base_components.hpp"
#include "engine/physics_system.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/collectable_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/interaction/force_field.hpp"
#include "game_logic/interaction/teleporter.hpp"
#include "game_mode.hpp"


namespace rigel { namespace game_logic {

using data::PlayerModel;
using engine::components::AnimationLoop;
using engine::components::BoundingBox;
using engine::components::Sprite;
using engine::components::WorldPosition;
using player::PlayerState;

using namespace game_logic::components;

namespace ex = entityx;


PlayerInteractionSystem::PlayerInteractionSystem(
  ex::Entity player,
  PlayerModel* pPlayerModel,
  IGameServiceProvider* pServices,
  TeleportCallback teleportCallback
)
  : mPlayer(player)
  , mpPlayerModel(pPlayerModel)
  , mpServiceProvider(pServices)
  , mTeleportCallback(teleportCallback)
{
}


void PlayerInteractionSystem::update(
  entityx::EntityManager& es,
  entityx::EventManager& events,
  entityx::TimeDelta
) {
  assert(mPlayer.has_component<PlayerControlled>());
  auto& state = *mPlayer.component<PlayerControlled>();
  if (state.isPlayerDead()) {
    return;
  }

  if (state.mState != PlayerState::LookingUp) {
    state.mPerformedInteraction = false;
  }

  if (!state.mPerformedInteraction && state.mState == PlayerState::LookingUp) {
    const auto& playerBox = *mPlayer.component<BoundingBox>();
    const auto& playerPos = *mPlayer.component<WorldPosition>();
    const auto worldSpacePlayerBounds =
      engine::toWorldSpace(playerBox, playerPos);
    es.each<Interactable, WorldPosition, BoundingBox>(
      [this, &es, &state, &worldSpacePlayerBounds](
        ex::Entity entity,
        const Interactable& interactable,
        const WorldPosition& pos,
        const BoundingBox& bbox
      ) {
        if (state.mPerformedInteraction) {
          return;
        }
        const auto objectBounds = engine::toWorldSpace(bbox, pos);
        if (worldSpacePlayerBounds.intersects(objectBounds)) {
          performInteraction(es, entity, interactable.mType);
          state.mPerformedInteraction = true;
        }
      });
  }

  // ----------------------------------------------------------------------
  es.each<CollectableItem, WorldPosition, BoundingBox>(
    [this, &es](
      ex::Entity entity,
      const CollectableItem& collectable,
      const WorldPosition& pos,
      const BoundingBox& collisionRect
    ) {
      using namespace data;

      auto worldSpaceBbox = collisionRect;
      worldSpaceBbox.topLeft +=
        base::Vector{pos.x, pos.y - (worldSpaceBbox.size.height - 1)};

      const auto playerPos = *mPlayer.component<WorldPosition>();
      auto playerBBox = *mPlayer.component<BoundingBox>();
      playerBBox.topLeft +=
        base::Vector{playerPos.x, playerPos.y - (playerBBox.size.height - 1)};

      if (worldSpaceBbox.intersects(playerBBox)) {
        boost::optional<data::SoundId> soundToPlay;

        if (collectable.mGivenScore) {
          assert(*collectable.mGivenScore > 0);
          mpPlayerModel->mScore += *collectable.mGivenScore;

          soundToPlay = SoundId::ItemPickup;
        }

        if (collectable.mGivenHealth) {
          assert(*collectable.mGivenHealth > 0);
          mpPlayerModel->mHealth = std::min(
            data::MAX_HEALTH,
            mpPlayerModel->mHealth + *collectable.mGivenHealth);

          soundToPlay = SoundId::HealthPickup;
        }

        if (collectable.mGivenWeapon) {
          mpPlayerModel->switchToWeapon(*collectable.mGivenWeapon);
          soundToPlay = SoundId::WeaponPickup;
        }

        if (collectable.mGivenItem) {
          const auto itemType = *collectable.mGivenItem;
          mpPlayerModel->mInventory.insert(itemType);

          soundToPlay = itemType == InventoryItemType::RapidFire ?
            SoundId::WeaponPickup :
            SoundId::ItemPickup;
        }

        if (collectable.mGivenCollectableLetter) {
          mpPlayerModel->mCollectedLetters.insert(
            *collectable.mGivenCollectableLetter);

          soundToPlay = SoundId::ItemPickup;
        }

        if (soundToPlay) {
          mpServiceProvider->playSound(*soundToPlay);
        }

        es.destroy(entity.id());
      }
    });
}


void PlayerInteractionSystem::performInteraction(
  entityx::EntityManager& es,
  entityx::Entity interactable,
  const components::InteractableType type
) {
  switch (type) {
    case InteractableType::Teleporter:
      mTeleportCallback(interactable);
      break;

    case InteractableType::ForceFieldCardReader:
      if (interaction::disableForceField(es, interactable, mpPlayerModel)) {
        triggerPlayerInteractionAnimation();
      }
      break;
  }
}


void PlayerInteractionSystem::triggerPlayerInteractionAnimation() {
  auto& state = *mPlayer.component<PlayerControlled>();
  state.enterTimedInteractionLock();
}

}}
