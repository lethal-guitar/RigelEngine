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
