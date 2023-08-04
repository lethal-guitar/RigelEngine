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

#pragma once

#include "base/warnings.hpp"
#include "data/game_session_data.hpp"
#include "data/level_hints.hpp"
#include "data/player_model.hpp"
#include "data/tutorial_messages.hpp"
#include "engine/base_components.hpp"
#include "game_logic/player/components.hpp"
#include "game_logic_common/input.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <string>

namespace rigel
{
struct IGameServiceProvider;

namespace data
{
class PersistentPlayerState;
}

namespace events
{
struct CloakExpired;
}

namespace game_logic
{
struct IEntityFactory;
}

namespace assets
{
class ResourceLoader;
}
} // namespace rigel


namespace rigel::game_logic
{

class Player;

class PlayerInteractionSystem
{
public:
  PlayerInteractionSystem(
    const data::GameSessionId& sessionId,
    Player* pPlayer,
    data::PersistentPlayerState* pPersistentPlayerState,
    IGameServiceProvider* pServices,
    IEntityFactory* pEntityFactory,
    entityx::EventManager* pEvents,
    const assets::ResourceLoader& resources);

  void updatePlayerInteraction(
    const PlayerInput& input,
    entityx::EntityManager& es);

  void updateItemCollection(entityx::EntityManager& es);

private:
  void showMessage(const std::string& text);
  void showTutorialMessage(const data::TutorialMessageId id);

  void performInteraction(
    entityx::EntityManager& es,
    entityx::Entity interactable,
    components::InteractableType type);

  void activateTeleporter(
    entityx::EntityManager& es,
    entityx::Entity interactable);
  void activateCardReader(
    entityx::EntityManager& es,
    entityx::Entity interactable);
  void
    activateKeyHole(entityx::EntityManager& es, entityx::Entity interactable);
  void activateHintMachine(entityx::Entity entity);

  void
    collectLetter(data::CollectableLetterType type, const base::Vec2& position);

private:
  Player* mpPlayer;
  data::PersistentPlayerState* mpPersistentPlayerState;
  IGameServiceProvider* mpServiceProvider;
  IEntityFactory* mpEntityFactory;
  entityx::EventManager* mpEvents;
  data::LevelHints mLevelHints;
  data::GameSessionId mSessionId;
};

} // namespace rigel::game_logic
