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
#include "data/player_model.hpp"
#include "data/tutorial_messages.hpp"
#include "engine/base_components.hpp"
#include "game_logic/input.hpp"
#include "game_logic/player/components.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <string>

namespace rigel {
  struct IGameServiceProvider;

  namespace data {
    class PlayerModel;
  }

  namespace game_logic {
    class EntityFactory;
  }
}


namespace rigel { namespace game_logic {

class Player;

class PlayerInteractionSystem {
public:
  PlayerInteractionSystem(
    Player* pPlayer,
    data::PlayerModel* pPlayerModel,
    IGameServiceProvider* pServices,
    EntityFactory* pEntityFactory,
    entityx::EventManager* pEvents);

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
    components::InteractableType type
  );

  void collectLetter(
    data::CollectableLetterType type,
    const base::Vector& position);

private:
  Player* mpPlayer;
  data::PlayerModel* mpPlayerModel;
  IGameServiceProvider* mpServiceProvider;
  EntityFactory* mpEntityFactory;
  entityx::EventManager* mpEvents;
};

}}
