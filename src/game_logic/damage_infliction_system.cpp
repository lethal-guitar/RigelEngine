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

#include "damage_infliction_system.hpp"

#include "data/player_model.hpp"
#include "engine/base_components.hpp"
#include "engine/physical_components.hpp"
#include "engine/visual_components.hpp"
#include "frontend/game_service_provider.hpp"


namespace rigel::game_logic
{

namespace ex = entityx;

using engine::components::Active;
using engine::components::BoundingBox;
using engine::components::CollidedWithWorld;
using engine::components::MovingBody;
using engine::components::Sprite;
using engine::components::WorldPosition;
using game_logic::components::CustomDamageApplication;
using game_logic::components::DamageInflicting;
using game_logic::components::Shootable;


namespace
{

auto extractVelocity(entityx::Entity entity)
{
  return entity.has_component<MovingBody>()
    ? entity.component<MovingBody>()->mVelocity
    : base::Vec2f{};
}

} // namespace


DamageInflictionSystem::DamageInflictionSystem(
  data::PlayerModel* pPlayerModel,
  IGameServiceProvider* pServiceProvider,
  entityx::EventManager* pEvents)
  : mpPlayerModel(pPlayerModel)
  , mpServiceProvider(pServiceProvider)
  , mpEvents(pEvents)
{
}


void DamageInflictionSystem::update(ex::EntityManager& es)
{
  es.each<Shootable, WorldPosition, BoundingBox>(
    [this, &es](
      ex::Entity shootableEntity,
      Shootable& shootable,
      const WorldPosition& shootablePos,
      const BoundingBox& shootableBboxLocal) {
      const auto shootableBbox =
        engine::toWorldSpace(shootableBboxLocal, shootablePos);

      ex::ComponentHandle<DamageInflicting> damage;
      ex::ComponentHandle<WorldPosition> inflictorPosition;
      ex::ComponentHandle<BoundingBox> inflictorBboxLocal;
      for (auto inflictorEntity : es.entities_with_components(
             damage, inflictorPosition, inflictorBboxLocal))
      {
        const auto inflictorBbox =
          engine::toWorldSpace(*inflictorBboxLocal, *inflictorPosition);

        const auto shootableOnScreen =
          shootableEntity.has_component<Active>() &&
          shootableEntity.component<Active>()->mIsOnScreen;

        // clang-format off
        if (
          shootableBbox.intersects(inflictorBbox) &&
          !shootable.mInvincible &&
          (shootableOnScreen || shootable.mCanBeHitWhenOffscreen))
        // clang-format on
        {
          inflictDamage(inflictorEntity, *damage, shootableEntity, shootable);
          break;
        }
      }
    });
}


void DamageInflictionSystem::inflictDamage(
  entityx::Entity inflictorEntity,
  DamageInflicting& damage,
  entityx::Entity shootableEntity,
  Shootable& shootable)
{
  const auto inflictorVelocity = extractVelocity(inflictorEntity);
  if (damage.mDestroyOnContact || shootable.mAlwaysConsumeInflictor)
  {
    inflictorEntity.destroy();
  }
  else
  {
    damage.mHasCausedDamage = true;
  }

  if (shootableEntity.has_component<CustomDamageApplication>())
  {
    mpEvents->emit(events::ShootableDamaged{shootableEntity, inflictorEntity});
  }
  else
  {
    shootable.mHealth -= damage.mAmount;

    if (shootable.mHealth > 0)
    {
      mpEvents->emit(
        events::ShootableDamaged{shootableEntity, inflictorEntity});
    }
  }

  if (shootable.mHealth <= 0)
  {
    mpEvents->emit(events::ShootableKilled{shootableEntity, inflictorVelocity});
    // Event listeners mustn't remove the shootable component
    assert(shootableEntity.has_component<Shootable>());

    mpPlayerModel->giveScore(shootable.mGivenScore);

    if (shootable.mDestroyWhenKilled)
    {
      shootableEntity.destroy();
    }
    else
    {
      shootableEntity.remove<Shootable>();
    }
  }
  else
  {
    if (shootable.mEnableHitFeedback)
    {
      mpServiceProvider->playSound(data::SoundId::EnemyHit);

      if (shootableEntity.has_component<Sprite>())
      {
        shootableEntity.component<Sprite>()->flashWhite();
      }
    }
  }
}

} // namespace rigel::game_logic
