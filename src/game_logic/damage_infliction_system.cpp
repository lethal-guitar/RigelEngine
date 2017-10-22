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

#include "data/player_data.hpp"
#include "engine/base_components.hpp"
#include "engine/physical_components.hpp"
#include "engine/visual_components.hpp"
#include "game_service_provider.hpp"


namespace rigel { namespace game_logic {

namespace ex = entityx;

using engine::components::Active;
using engine::components::BoundingBox;
using engine::components::CollidedWithWorld;
using engine::components::MovingBody;
using engine::components::Sprite;
using engine::components::WorldPosition;
using game_logic::components::DamageInflicting;
using game_logic::components::Shootable;


namespace {

auto extractVelocity(entityx::Entity entity) {
  return entity.has_component<MovingBody>()
    ? entity.component<MovingBody>()->mVelocity
    : base::Point<float>{};
}

}


DamageInflictionSystem::DamageInflictionSystem(
  data::PlayerModel* pPlayerModel,
  IGameServiceProvider* pServiceProvider,
  entityx::EventManager* pEvents
)
  : mpPlayerModel(pPlayerModel)
  , mpServiceProvider(pServiceProvider)
  , mpEvents(pEvents)
{
}


void DamageInflictionSystem::update(ex::EntityManager& es) {
  es.each<DamageInflicting, WorldPosition, BoundingBox>(
    [this, &es](
      ex::Entity inflictorEntity,
      DamageInflicting& damage,
      const WorldPosition& inflictorPosition,
      const BoundingBox& bbox
    ) {
      const auto inflictorBbox = engine::toWorldSpace(bbox, inflictorPosition);

      ex::ComponentHandle<Shootable> shootable;
      ex::ComponentHandle<WorldPosition> shootablePos;
      ex::ComponentHandle<BoundingBox> shootableBboxLocal;
      ex::ComponentHandle<Active> active;
      for (auto shootableEntity : es.entities_with_components(
        shootable, shootablePos, shootableBboxLocal, active)
      ) {
        const auto shootableBbox =
          engine::toWorldSpace(*shootableBboxLocal, *shootablePos);
        if (
          shootableBbox.intersects(inflictorBbox) &&
          !shootable->mInvincible &&
          active->mIsOnScreen
        ) {
          const auto destroyOnContact = damage.mDestroyOnContact;
          inflictDamage(inflictorEntity, damage, shootableEntity, *shootable);
          if (destroyOnContact) {
            break;
          }
        }
      }
    });
}


void DamageInflictionSystem::inflictDamage(
  entityx::Entity inflictorEntity,
  DamageInflicting& damage,
  entityx::Entity shootableEntity,
  Shootable& shootable
) {
  const auto inflictorVelocity = extractVelocity(inflictorEntity);
  if (damage.mDestroyOnContact || shootable.mAlwaysConsumeInflictor) {
    inflictorEntity.destroy();
  } else {
    damage.mHasCausedDamage = true;
  }

  shootable.mHealth -= damage.mAmount;
  if (shootable.mHealth <= 0) {
    mpEvents->emit(
      events::ShootableKilled{shootableEntity, inflictorVelocity});
    // Event listeners mustn't remove the shootable component
    assert(shootableEntity.has_component<Shootable>());

    mpPlayerModel->mScore += shootable.mGivenScore;

    if (shootable.mDestroyWhenKilled) {
      shootableEntity.destroy();
    } else {
      shootableEntity.remove<Shootable>();
    }
  } else {
    mpEvents->emit(
      events::ShootableDamaged{shootableEntity, inflictorVelocity});

    if (shootable.mEnableHitFeedback) {
      mpServiceProvider->playSound(data::SoundId::EnemyHit);

      if (shootableEntity.has_component<Sprite>()) {
        shootableEntity.component<Sprite>()->flashWhite();
      }
    }
  }
}

}}
