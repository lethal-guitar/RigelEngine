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
#include "game_logic/player/components.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS


namespace rigel {
  struct IGameServiceProvider;

  namespace data {
    struct PlayerModel;
  }

  namespace game_logic { namespace events {
    struct PlayerInteraction;
  }}
}


namespace rigel { namespace game_logic {

class PlayerInteractionSystem :
  public entityx::System<PlayerInteractionSystem>,
  public entityx::Receiver<PlayerInteractionSystem>
{
public:
  PlayerInteractionSystem(
    entityx::Entity player,
    data::PlayerModel* pPlayerModel,
    IGameServiceProvider* pServices);

  void update(
    entityx::EntityManager& es,
    entityx::EventManager& events,
    entityx::TimeDelta) override;

private:
  void performInteraction(
    entityx::EntityManager& es,
    entityx::Entity interactable,
    components::InteractableType type
  );

private:
  entityx::Entity mPlayer;
  bool mNeedFadeIn = false;
  data::PlayerModel* mpPlayerModel;
  IGameServiceProvider* mpServiceProvider;
};

}}
