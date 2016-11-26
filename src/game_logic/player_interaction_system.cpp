#include "player_interaction_system.hpp"

#include <data/player_data.hpp>
#include <engine/base_components.hpp>
#include <engine/physics_system.hpp>
#include <game_logic/collectable_components.hpp>
#include <game_logic/player_control_system.hpp>
#include <game_mode.hpp>


namespace rigel { namespace game_logic {

using data::PlayerModel;
using engine::components::BoundingBox;
using engine::components::WorldPosition;

namespace ex = entityx;


PlayerInteractionSystem::PlayerInteractionSystem(
  ex::Entity player,
  PlayerModel* pPlayerModel,
  IGameServiceProvider* pServices
)
  : mPlayer(player)
  , mpPlayerModel(pPlayerModel)
  , mpServiceProvider(pServices)
{
}


void PlayerInteractionSystem::configure(
  ex::EntityManager&,
  ex::EventManager& events
) {
  events.subscribe<game_logic::events::PlayerInteraction>(*this);
}


void PlayerInteractionSystem::update(
  entityx::EntityManager& es,
  entityx::EventManager& events,
  entityx::TimeDelta
) {
  using namespace game_logic::components;

  if (mNeedFadeIn) {
    mNeedFadeIn = false;
    mpServiceProvider->fadeInScreen();
  }

  // ----------------------------------------------------------------------
  if (mTeleportRequested) {
    mTeleportRequested = false;

    const auto sourceTeleporterPosition =
      *mSourceTeleporter.component<WorldPosition>().get();

    es.each<Interactable, WorldPosition>(
      [this, sourceTeleporterPosition](
        ex::Entity,
        const Interactable& i,
        const WorldPosition& pos
      ) {
        if (
          i.mType == InteractableType::Teleporter &&
          pos != sourceTeleporterPosition
        ) {
          mpServiceProvider->fadeOutScreen();
          auto playerPosition = mPlayer.component<WorldPosition>();
          *playerPosition.get() = pos + base::Vector{2, 0};
          mNeedFadeIn = true;
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

      const auto playerPos = *mPlayer.component<WorldPosition>().get();
      auto playerBBox = *mPlayer.component<BoundingBox>().get();
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
          mpPlayerModel->mWeapon = *collectable.mGivenWeapon;
          mpPlayerModel->mAmmo = mpPlayerModel->currentMaxAmmo();

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

void PlayerInteractionSystem::receive(
  const game_logic::events::PlayerInteraction& interaction
) {
  mTeleportRequested = true;
  mSourceTeleporter = interaction.mInteractedEntity;
}

}}
