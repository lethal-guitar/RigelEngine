#pragma once

#include <base/warnings.hpp>

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

  void configure(
    entityx::EntityManager&,
    entityx::EventManager& events) override;

  void update(
    entityx::EntityManager& es,
    entityx::EventManager& events,
    entityx::TimeDelta) override;

  void receive(
    const game_logic::events::PlayerInteraction& interaction
  );

private:
  entityx::Entity mPlayer;
  bool mTeleportRequested = false;
  entityx::Entity mSourceTeleporter;
  bool mNeedFadeIn = false;
  data::PlayerModel* mpPlayerModel;
  IGameServiceProvider* mpServiceProvider;
};

}}
