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

#include "data/strings.hpp"
#include "engine/physics_system.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/collectable_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/entity_factory.hpp"
#include "game_logic/interaction/force_field.hpp"
#include "game_logic/interaction/teleporter.hpp"
#include "game_service_provider.hpp"
#include "global_level_events.hpp"


namespace rigel { namespace game_logic {

using data::PlayerModel;
using engine::components::AnimationLoop;
using engine::components::BoundingBox;
using engine::components::Sprite;
using engine::components::WorldPosition;
using player::PlayerState;

using namespace game_logic::components;

namespace ex = entityx;

namespace {

constexpr auto BASIC_LETTER_COLLECTION_SCORE = 10100;
constexpr auto CORRECT_LETTER_COLLECTION_SCORE = 100000;


void spawnScoreNumbers(
  const base::Vector& position,
  int score,
  EntityFactory& entityFactory
) {
  std::vector<ScoreNumberType> numbers;
  for (const auto numberType : ScoreNumberType_Items) {
    const auto value = scoreNumberValue(numberType);
    for (; score >= value; score -= value) {
      numbers.push_back(numberType);
    }
  }

  if (numbers.empty()) {
    return;
  }

  auto yOffset = static_cast<int>(numbers.size() - 1);
  for (const auto numberType : numbers) {
    const auto offset = base::Vector{0, yOffset};
    --yOffset;
    spawnFloatingScoreNumber(entityFactory, numberType, position - offset);
  }
}


void spawnScoreNumbersForLetterCollectionBonus(
  EntityFactory& factory,
  const base::Vector& position
) {
  constexpr int X_OFFSETS[] = {-3, 0, 3, 0};

  for (int i = 0; i < 10; ++i) {
    const auto offset = base::Vector{X_OFFSETS[i % 4], -i};
    spawnFloatingScoreNumber(
      factory, ScoreNumberType::S10000, position + offset);
  }
}

}


PlayerInteractionSystem::PlayerInteractionSystem(
  ex::Entity player,
  PlayerModel* pPlayerModel,
  IGameServiceProvider* pServices,
  EntityFactory* pEntityFactory,
  TeleportCallback teleportCallback,
  entityx::EventManager* pEvents
)
  : mPlayer(player)
  , mpPlayerModel(pPlayerModel)
  , mpServiceProvider(pServices)
  , mpEntityFactory(pEntityFactory)
  , mTeleportCallback(teleportCallback)
  , mpEvents(pEvents)
{
}


void PlayerInteractionSystem::update(entityx::EntityManager& es) {
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

        const auto playerAtFullHealth = mpPlayerModel->isAtFullHealth();
        if (auto maybeScore = givenScore(collectable, playerAtFullHealth)) {
          const auto score = *maybeScore;
          assert(score > 0);
          mpPlayerModel->giveScore(score);

          soundToPlay = SoundId::ItemPickup;

          if (collectable.mSpawnScoreNumbers) {
            spawnScoreNumbers(pos, score, *mpEntityFactory);
          }
        }

        if (collectable.mGivenHealth) {
          assert(*collectable.mGivenHealth > 0);
          mpPlayerModel->giveHealth(*collectable.mGivenHealth);
          soundToPlay = SoundId::HealthPickup;
        }

        if (collectable.mGivenWeapon) {
          mpPlayerModel->switchToWeapon(*collectable.mGivenWeapon);
          soundToPlay = SoundId::WeaponPickup;
        }

        if (collectable.mGivenItem) {
          const auto itemType = *collectable.mGivenItem;
          mpPlayerModel->giveItem(itemType);

          soundToPlay = itemType == InventoryItemType::RapidFire ?
            SoundId::WeaponPickup :
            SoundId::ItemPickup;
        }

        if (collectable.mGivenCollectableLetter) {
          collectLetter(*collectable.mGivenCollectableLetter, pos);
        }

        if (soundToPlay) {
          mpServiceProvider->playSound(*soundToPlay);
        }

        es.destroy(entity.id());
      }
    });
}


void PlayerInteractionSystem::showMessage(const std::string& text) {
  mpEvents->emit(rigel::events::PlayerMessage{text});
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


void PlayerInteractionSystem::collectLetter(
  const data::CollectableLetterType type,
  const base::Vector& position
) {
  using S = data::PlayerModel::LetterCollectionState;

  const auto collectionState = mpPlayerModel->addLetter(type);
  if (collectionState == S::InOrder) {
    mpServiceProvider->playSound(data::SoundId::LettersCollectedCorrectly);
    mpPlayerModel->giveScore(CORRECT_LETTER_COLLECTION_SCORE);
    spawnScoreNumbersForLetterCollectionBonus(*mpEntityFactory, position);
  } else {
    mpServiceProvider->playSound(data::SoundId::ItemPickup);
    mpPlayerModel->giveScore(BASIC_LETTER_COLLECTION_SCORE);

    // In the original game, bonus letters spawn a floating 100 on pickup, but
    // the player is given 10100 points. This seems like a bug. My guess is
    // that the additional 10000 points are only supposed to be given when all
    // letters were collected out of order. The game shows a hint message in
    // this case which mentions a 10000 points bonus, but the actual score
    // given is still only 10100. So it seems that this "out of order
    // collection bonus" is accidentally given for every single letter that's
    // picked up, instead of only when all letters have been collected.
    spawnFloatingScoreNumber(*mpEntityFactory, ScoreNumberType::S100, position);

    if (collectionState == S::WrongOrder) {
      showMessage(data::Messages::LettersCollectedWrongOrder);
    }
  }
}

}}
