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

#include "item_container.hpp"

#include "common/game_service_provider.hpp"
#include "data/sound_ids.hpp"
#include "engine/base_components.hpp"
#include "engine/collision_checker.hpp"
#include "engine/life_time_components.hpp"
#include "engine/motion_smoothing.hpp"
#include "engine/sprite_tools.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/actor_tag.hpp"
#include "game_logic/damage_components.hpp"
#include "game_logic/effect_components.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/ientity_factory.hpp"


namespace rigel::game_logic
{

namespace
{

using engine::components::Active;
using engine::components::BoundingBox;
using engine::components::MovingBody;
using engine::components::Sprite;
using engine::components::WorldPosition;
using game_logic::components::ItemBounceEffect;
using game_logic::components::ItemContainer;

constexpr int ITEM_BOUNCE_SEQUENCE[] = {-3, -2, -1, 0, 1, 2, 3, -1, 1};

} // namespace


ItemContainerSystem::ItemContainerSystem(
  entityx::EntityManager* pEntityManager,
  const engine::CollisionChecker* pCollisionChecker,
  entityx::EventManager& events)
  : mpEntityManager(pEntityManager)
  , mpCollisionChecker(pCollisionChecker)
{
  events.subscribe<events::ShootableKilled>(*this);
}


void ItemContainerSystem::update(entityx::EntityManager& es)
{
  using RS = ItemContainer::ReleaseStyle;

  auto releaseItem =
    [this](
      entityx::Entity entity,
      const std::vector<ComponentHolder>& containedComponents) {
      auto contents = mpEntityManager->create();
      for (auto& component : containedComponents)
      {
        component.assignToEntity(contents);
      }

      contents.assign<WorldPosition>(*entity.component<WorldPosition>());
      engine::discardInterpolation(contents);

      return contents;
    };

  es.each<ItemContainer>([&](entityx::Entity entity, ItemContainer& container) {
    if (!container.mHasBeenShot)
    {
      return;
    }

    switch (container.mStyle)
    {
      case RS::Default:
        releaseItem(entity, container.mContainedComponents);
        entity.destroy();
        break;

      case RS::ItemBox:
      case RS::ItemBoxNoBounce:
        ++container.mFramesElapsed;

        if (container.mFramesElapsed == 1)
        {
          entity.component<Sprite>()->flashWhite();
        }
        else if (container.mFramesElapsed == 2)
        {
          auto item = releaseItem(entity, container.mContainedComponents);

          if (container.mStyle != RS::ItemBoxNoBounce)
          {
            const auto fallVelocity =
              entity.component<MovingBody>()->mVelocity.y;
            item.assign<ItemBounceEffect>(fallVelocity);
            item.component<WorldPosition>()->y += ITEM_BOUNCE_SEQUENCE[0];
          }

          entity.destroy();
        }
        break;

      case RS::NuclearWasteBarrel:
        ++container.mFramesElapsed;

        if (container.mFramesElapsed == 1)
        {
          entity.component<Sprite>()->flashWhite();
        }
        else if (container.mFramesElapsed == 2)
        {
          // Switch to "bulging" state
          ++entity.component<Sprite>()->mFramesToRender[0];
        }
        else if (container.mFramesElapsed == 3)
        {
          // At this point, the destruction effects take over
          entity.component<Sprite>()->mShow = false;
        }
        else if (container.mFramesElapsed == 4)
        {
          releaseItem(entity, container.mContainedComponents);
          entity.destroy();
        }
        break;
    }
  });
}


void ItemContainerSystem::updateItemBounce(entityx::EntityManager& es)
{
  es.each<WorldPosition, BoundingBox, MovingBody, ItemBounceEffect>(
    [this](
      entityx::Entity entity,
      WorldPosition& position,
      const BoundingBox& bbox,
      MovingBody& body,
      ItemBounceEffect& state) {
      position.y += ITEM_BOUNCE_SEQUENCE[state.mFramesElapsed];

      ++state.mFramesElapsed;

      if (const auto hasLanded =
            mpCollisionChecker->isOnSolidGround(position, bbox);
          (state.mFramesElapsed == 6 && !hasLanded) ||
          state.mFramesElapsed == 9)
      {
        body.mGravityAffected = true;
        body.mVelocity.y = state.mFallVelocity;
      }

      if (state.mFramesElapsed == 9)
      {
        entity.remove<ItemBounceEffect>();
      }
    });
}


void ItemContainerSystem::receive(const events::ShootableKilled& event)
{
  auto entity = event.mEntity;
  if (entity.has_component<ItemContainer>())
  {
    // We can't open up the item container immediately, but have to do it
    // in our update() function. This is because the container's contents
    // might be shootable, and this could cause them to be hit by the
    // same projectile as the one that opened the container. By deferring
    // opening the container to our update, the damage infliction update
    // will be finished, so this problem can't occur.
    entity.component<components::Shootable>()->mDestroyWhenKilled = false;
    entity.component<ItemContainer>()->mHasBeenShot = true;
  }
}


namespace behaviors
{

void NapalmBomb::update(
  GlobalDependencies& d,
  GlobalState&,
  const bool,
  entityx::Entity entity)
{
  using components::DestructionEffects;
  using State = NapalmBomb::State;

  ++mFramesElapsed;

  const auto& position = *entity.component<WorldPosition>();
  auto& sprite = *entity.component<Sprite>();

  switch (mState)
  {
    case State::Ticking:
      if (mFramesElapsed >= 25 && mFramesElapsed % 2 == 1)
      {
        sprite.flashWhite();
      }

      if (mFramesElapsed >= 31)
      {
        explode(d, entity);

        // Remove the shootable to prevent explode() being called twice
        // in case the timeout happens on the same frame as the bomb being hit
        // by a shot.
        entity.remove<game_logic::components::Shootable>();
      }
      break;

    case State::SpawningFires:
      if (mFramesElapsed > 10)
      {
        entity.destroy();
        return;
      }

      if (mFramesElapsed % 2 == 0)
      {
        const auto step = mFramesElapsed / 2;
        spawnFires(d, position, step);
      }
      break;
  }
}


void NapalmBomb::onKilled(
  GlobalDependencies& d,
  GlobalState&,
  const base::Vec2T<float>&,
  entityx::Entity entity)
{
  explode(d, entity);
}


void NapalmBomb::explode(GlobalDependencies& d, entityx::Entity entity)
{
  const auto& position = *entity.component<WorldPosition>();

  triggerEffects(entity, *d.mpEntityManager);

  d.mpServiceProvider->playSound(data::SoundId::Explosion);
  spawnFires(d, position, 0);

  mState = NapalmBomb::State::SpawningFires;
  mFramesElapsed = 0;
  entity.component<Sprite>()->mShow = false;
  entity.remove<MovingBody>();
  entity.remove<game_logic::components::AppearsOnRadar>();

  // Once the bomb explodes, it counts towards bonus 6. This means we need to
  // remove the actor tag (which is used to count remaining fire bombs in the
  // level when determining which bonuses have been achieved) here.
  entity.remove<components::ActorTag>();
}


void NapalmBomb::spawnFires(
  GlobalDependencies& d,
  const base::Vector& bombPosition,
  const int step)
{
  using namespace game_logic::components::parameter_aliases;

  auto spawnOneFire = [&, this](const base::Vector& position) {
    const auto canSpawn =
      d.mpCollisionChecker->isOnSolidGround(position, BoundingBox{{}, {2, 1}});

    if (canSpawn)
    {
      auto fire = spawnOneShotSprite(
        *d.mpEntityFactory, data::ActorID::Fire_bomb_fire, position);
      fire.assign<components::PlayerDamaging>(Damage{1});
      fire.assign<components::DamageInflicting>(
        Damage{1}, DestroyOnContact{false});
    }
    return canSpawn;
  };

  const auto basePosition = WorldPosition{step + 1, 0};
  if (mCanSpawnLeft)
  {
    mCanSpawnLeft = spawnOneFire(bombPosition + basePosition * -2);
  }

  if (mCanSpawnRight)
  {
    mCanSpawnRight = spawnOneFire(bombPosition + basePosition * 2);
  }
}

} // namespace behaviors
} // namespace rigel::game_logic
