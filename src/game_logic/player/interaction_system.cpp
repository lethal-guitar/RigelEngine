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

#include "interaction_system.hpp"

#include "assets/duke_script_loader.hpp"
#include "assets/resource_loader.hpp"
#include "data/strings.hpp"
#include "engine/physics_system.hpp"
#include "engine/visual_components.hpp"
#include "frontend/game_service_provider.hpp"
#include "game_logic/actor_tag.hpp"
#include "game_logic/collectable_components.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/ientity_factory.hpp"
#include "game_logic/interactive/force_field.hpp"
#include "game_logic/interactive/locked_door.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic
{

using data::PlayerModel;
using engine::components::AnimationLoop;
using engine::components::BoundingBox;
using engine::components::Sprite;
using engine::components::WorldPosition;

using namespace game_logic::components;

namespace ex = entityx;

namespace
{

constexpr auto BASIC_LETTER_COLLECTION_SCORE = 10100;
constexpr auto CORRECT_LETTER_COLLECTION_SCORE = 100000;
constexpr auto HINT_MACHINE_ACTIVATION_SCORE = 50000;

constexpr auto PLAYER_TO_TELEPORTER_OFFSET = base::Vec2{1, 0};
constexpr auto HINT_MACHINE_GLOBE_OFFSET = base::Vec2{1, -4};


void spawnScoreNumbers(
  const base::Vec2& position,
  int score,
  IEntityFactory& entityFactory)
{
  std::vector<ScoreNumberType> numbers;
  for (const auto numberType : ScoreNumberType_Items)
  {
    const auto value = scoreNumberValue(numberType);
    for (; score >= value; score -= value)
    {
      numbers.push_back(numberType);
    }
  }

  if (numbers.empty())
  {
    return;
  }

  auto yOffset = static_cast<int>(numbers.size() - 1);
  for (const auto numberType : numbers)
  {
    const auto offset = base::Vec2{0, yOffset};
    --yOffset;
    spawnFloatingScoreNumber(entityFactory, numberType, position - offset);
  }
}


void spawnScoreNumbersForLetterCollectionBonus(
  IEntityFactory& factory,
  const base::Vec2& position)
{
  constexpr int X_OFFSETS[] = {-3, 0, 3, 0};

  for (int i = 0; i < 10; ++i)
  {
    const auto offset = base::Vec2{X_OFFSETS[i % 4], -i};
    spawnFloatingScoreNumber(
      factory, ScoreNumberType::S10000, position + offset);
  }
}

data::TutorialMessageId tutorialFor(const components::InteractableType type)
{
  using TM = data::TutorialMessageId;
  switch (type)
  {
    case InteractableType::Teleporter:
      return TM::FoundTeleporter;

    case InteractableType::ForceFieldCardReader:
      return TM::FoundForceField;

    case InteractableType::KeyHole:
      return TM::FoundDoor;

    case InteractableType::HintMachine:
      return TM::HintGlobeNeeded;
  }

  assert(false);
  std::terminate();
}


std::optional<base::Vec2> findTeleporterTargetPosition(
  ex::EntityManager& es,
  ex::Entity sourceTeleporter)
{
  ex::ComponentHandle<components::Interactable> interactable;
  ex::ComponentHandle<WorldPosition> position;

  ex::Entity targetTeleporter;
  for (auto entity : es.entities_with_components(interactable, position))
  {
    if (
      interactable->mType == components::InteractableType::Teleporter &&
      entity != sourceTeleporter)
    {
      targetTeleporter = entity;
    }
  }

  if (!targetTeleporter)
  {
    return std::nullopt;
  }

  const auto targetTeleporterPosition =
    *targetTeleporter.component<WorldPosition>();
  return targetTeleporterPosition + PLAYER_TO_TELEPORTER_OFFSET;
}


ex::Entity currentlyTouchedInteractable(
  entityx::EntityManager& es,
  const Player* pPlayer)
{
  const auto worldSpacePlayerBounds = pPlayer->worldSpaceHitBox();

  auto isInRange = [&](
                     const BoundingBox& objectBounds,
                     const WorldPosition& position,
                     const components::InteractableType type) {
    if (!worldSpacePlayerBounds.intersects(objectBounds))
    {
      return false;
    }

    const auto& playerPos = pPlayer->orientedPosition();

    switch (type)
    {
      case InteractableType::Teleporter:
        // clang-format off
        return
          objectBounds.left() <= playerPos.x &&
          objectBounds.left() + 3 >= playerPos.x &&
          objectBounds.bottom() == playerPos.y &&
          pPlayer->isInRegularState();
        // clang-format on

      case InteractableType::KeyHole:
      case InteractableType::ForceFieldCardReader:
        return playerPos.y - 2 == position.y;

      default:
        return true;
    }
  };


  ex::ComponentHandle<Interactable> interactable;
  ex::ComponentHandle<WorldPosition> pos;
  ex::ComponentHandle<BoundingBox> bbox;
  for (auto entity : es.entities_with_components(interactable, pos, bbox))
  {
    const auto objectBounds = engine::toWorldSpace(*bbox, *pos);
    if (isInRange(objectBounds, *pos, interactable->mType))
    {
      return entity;
    }
  }

  return {};
}


data::LevelHints loadHints(const assets::ResourceLoader& resources)
{
  const auto text = resources.fileAsText("HELP.MNI");
  return assets::loadHintMessages(text);
}

} // namespace


PlayerInteractionSystem::PlayerInteractionSystem(
  const data::GameSessionId& sessionId,
  Player* pPlayer,
  PlayerModel* pPlayerModel,
  IGameServiceProvider* pServices,
  IEntityFactory* pEntityFactory,
  entityx::EventManager* pEvents,
  const assets::ResourceLoader& resources)
  : mpPlayer(pPlayer)
  , mpPlayerModel(pPlayerModel)
  , mpServiceProvider(pServices)
  , mpEntityFactory(pEntityFactory)
  , mpEvents(pEvents)
  , mLevelHints(loadHints(resources))
  , mSessionId(sessionId)
{
}


void PlayerInteractionSystem::updatePlayerInteraction(
  const PlayerInput& input,
  entityx::EntityManager& es)
{
  if (mpPlayer->isDead())
  {
    return;
  }

  if (auto entity = currentlyTouchedInteractable(es, mpPlayer))
  {
    const auto type = entity.component<Interactable>()->mType;
    const auto isHintMachine = type == InteractableType::HintMachine;

    showTutorialMessage(tutorialFor(type));

    // The hint machine activates on touch, all other interactables require
    // pressing the interact button/key.
    if (input.mInteract.mWasTriggered || isHintMachine)
    {
      performInteraction(es, entity, type);
    }
  }
}


void PlayerInteractionSystem::updateItemCollection(entityx::EntityManager& es)
{
  if (mpPlayer->isDead())
  {
    return;
  }

  es.each<CollectableItem, WorldPosition, BoundingBox>(
    [this, &es](
      ex::Entity entity,
      const CollectableItem& collectable,
      const WorldPosition& pos,
      const BoundingBox& collisionRect) {
      using namespace data;

      auto worldSpaceBbox = collisionRect;
      worldSpaceBbox.topLeft +=
        base::Vec2{pos.x, pos.y - (worldSpaceBbox.size.height - 1)};

      auto playerBBox = mpPlayer->worldSpaceHitBox();
      if (worldSpaceBbox.intersects(playerBBox))
      {
        std::optional<data::SoundId> soundToPlay;

        const auto playerAtFullHealth = mpPlayerModel->isAtFullHealth();
        if (auto maybeScore = givenScore(collectable, playerAtFullHealth))
        {
          const auto score = *maybeScore;
          assert(score > 0);
          mpPlayerModel->giveScore(score);

          soundToPlay = SoundId::ItemPickup;

          if (collectable.mSpawnScoreNumbers)
          {
            spawnScoreNumbers(pos, score, *mpEntityFactory);
          }
        }

        if (collectable.mGivenHealth)
        {
          assert(*collectable.mGivenHealth > 0);
          mpPlayerModel->giveHealth(*collectable.mGivenHealth);
          soundToPlay = SoundId::HealthPickup;
        }

        if (collectable.mGivenWeapon)
        {
          mpPlayerModel->switchToWeapon(*collectable.mGivenWeapon);
          soundToPlay = SoundId::WeaponPickup;
        }

        if (collectable.mGivenItem)
        {
          const auto itemType = *collectable.mGivenItem;
          mpPlayerModel->giveItem(itemType);

          soundToPlay = itemType == InventoryItemType::RapidFire
            ? SoundId::WeaponPickup
            : SoundId::ItemPickup;

          if (itemType == InventoryItemType::SpecialHintGlobe)
          {
            showMessage(data::Messages::FoundSpecialHintGlobe);
          }

          if (itemType == InventoryItemType::CloakingDevice)
          {
            showMessage(data::Messages::FoundCloak);
            mpEvents->emit(rigel::events::CloakPickedUp{pos});
          }

          if (itemType == InventoryItemType::RapidFire)
          {
            mpEvents->emit(rigel::events::RapidFirePickedUp{});
          }
        }

        if (collectable.mShownTutorialMessage)
        {
          showTutorialMessage(*collectable.mShownTutorialMessage);
        }

        if (collectable.mGivenCollectableLetter)
        {
          collectLetter(*collectable.mGivenCollectableLetter, pos);
        }

        if (soundToPlay)
        {
          mpServiceProvider->playSound(*soundToPlay);
        }

        es.destroy(entity.id());
      }
    });
}


void PlayerInteractionSystem::showMessage(const std::string& text)
{
  mpEvents->emit(rigel::events::PlayerMessage{text});
}


void PlayerInteractionSystem::showTutorialMessage(
  const data::TutorialMessageId id)
{
  mpEvents->emit(rigel::events::TutorialMessage{id});
}


void PlayerInteractionSystem::performInteraction(
  entityx::EntityManager& es,
  entityx::Entity interactable,
  const components::InteractableType type)
{
  switch (type)
  {
    case InteractableType::Teleporter:
      activateTeleporter(es, interactable);
      break;

    case InteractableType::ForceFieldCardReader:
      activateCardReader(es, interactable);
      break;

    case InteractableType::KeyHole:
      activateKeyHole(es, interactable);
      break;

    case InteractableType::HintMachine:
      activateHintMachine(interactable);
      break;
  }
}


void PlayerInteractionSystem::activateTeleporter(
  entityx::EntityManager& es,
  entityx::Entity interactable)
{
  mpServiceProvider->playSound(data::SoundId::Teleport);

  const auto maybeTargetPosition =
    findTeleporterTargetPosition(es, interactable);
  if (maybeTargetPosition)
  {
    mpEvents->emit(rigel::events::PlayerTeleported{*maybeTargetPosition});
  }
  else
  {
    // If there is only one teleporter in the level, using it exits the level.
    // This is used in N7, for example.
    // Probably an oversight, but this does NOT check for radar dishes in the
    // original.
    mpEvents->emit(rigel::events::ExitReached{false});
  }
}


void PlayerInteractionSystem::activateCardReader(
  entityx::EntityManager& es,
  entityx::Entity interactable)
{
  const auto hasKey =
    mpPlayerModel->hasItem(data::InventoryItemType::CircuitBoard);

  if (hasKey)
  {
    mpPlayerModel->removeItem(data::InventoryItemType::CircuitBoard);
    interaction::disableKeyCardSlot(interactable);
    interaction::disableNextForceField(es);

    mpPlayer->doInteractionAnimation();
    showMessage(data::Messages::AccessGranted);
  }
  else
  {
    showTutorialMessage(data::TutorialMessageId::AccessCardNeeded);
  }
}


void PlayerInteractionSystem::activateKeyHole(
  entityx::EntityManager& es,
  entityx::Entity interactable)
{
  if (mpPlayerModel->hasItem(data::InventoryItemType::BlueKey))
  {
    mpPlayerModel->removeItem(data::InventoryItemType::BlueKey);
    interaction::disableKeyHole(interactable);

    auto door = findFirstMatchInSpawnOrder(es, ActorTag::Type::Door);
    if (door)
    {
      mpEvents->emit(rigel::events::DoorOpened{door});
    }

    mpPlayer->doInteractionAnimation();
    showMessage(data::Messages::OpeningDoor);
  }
  else
  {
    showTutorialMessage(data::TutorialMessageId::KeyNeeded);
  }
}


void PlayerInteractionSystem::activateHintMachine(entityx::Entity entity)
{
  if (!mpPlayerModel->hasItem(data::InventoryItemType::SpecialHintGlobe))
  {
    return;
  }

  const auto machinePosition = *entity.component<WorldPosition>();
  mpPlayerModel->removeItem(data::InventoryItemType::SpecialHintGlobe);
  mpPlayerModel->giveScore(HINT_MACHINE_ACTIVATION_SCORE);

  mpServiceProvider->playSound(data::SoundId::ItemPickup);
  spawnScoreNumbers(
    machinePosition, HINT_MACHINE_ACTIVATION_SCORE, *mpEntityFactory);

  const auto maybeHint =
    mLevelHints.getHint(mSessionId.mEpisode, mSessionId.mLevel);
  if (maybeHint)
  {
    mpEvents->emit(rigel::events::HintMachineMessage{*maybeHint});
  }

  entity.remove<Interactable>();
  entity.remove<BoundingBox>();
  mpEntityFactory->spawnSprite(
    data::ActorID::Special_hint_globe_icon,
    machinePosition + HINT_MACHINE_GLOBE_OFFSET);
}


void PlayerInteractionSystem::collectLetter(
  const data::CollectableLetterType type,
  const base::Vec2& position)
{
  using S = data::PlayerModel::LetterCollectionState;

  const auto collectionState = mpPlayerModel->addLetter(type);
  if (collectionState == S::InOrder)
  {
    mpServiceProvider->playSound(data::SoundId::LettersCollectedCorrectly);
    mpPlayerModel->giveScore(CORRECT_LETTER_COLLECTION_SCORE);
    spawnScoreNumbersForLetterCollectionBonus(*mpEntityFactory, position);
    showTutorialMessage(data::TutorialMessageId::LettersCollectedRightOrder);
  }
  else
  {
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

    if (collectionState == S::WrongOrder)
    {
      showMessage(data::Messages::LettersCollectedWrongOrder);
    }
  }
}

} // namespace rigel::game_logic
