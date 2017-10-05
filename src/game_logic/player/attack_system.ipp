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

#include "data/player_data.hpp"
#include "game_logic/player/attack_traits.hpp"
#include "game_mode.hpp"

#include <cassert>


namespace rigel { namespace game_logic { namespace player {

namespace {


inline ProjectileType projectileTypeForWeapon(const data::WeaponType weaponType) {
  using data::WeaponType;

  switch (weaponType) {
    case WeaponType::Normal:
      return ProjectileType::PlayerRegularShot;

    case WeaponType::Laser:
      return ProjectileType::PlayerLaserShot;

    case WeaponType::Rocket:
      return ProjectileType::PlayerRocketShot;

    case WeaponType::FlameThrower:
      return ProjectileType::PlayerFlameShot;
  }

  assert(false);
  return ProjectileType::PlayerRegularShot;
}


inline data::SoundId soundIdForWeapon(const data::WeaponType weaponType) {
  using data::SoundId;
  using data::WeaponType;

  switch (weaponType) {
    case WeaponType::Laser:
      return SoundId::DukeLaserShot;

    case WeaponType::FlameThrower:
      return SoundId::FlameThrowerShot;

    default:
      return SoundId::DukeNormalShot;
  };
}

}


template<typename EntityFactoryT>
AttackSystem<EntityFactoryT>::AttackSystem(
  entityx::Entity playerEntity,
  data::PlayerModel* pPlayerModel,
  IGameServiceProvider* pServiceProvider,
  EntityFactoryT* pEntityFactory
)
  : mPlayerEntity(playerEntity)
  , mpPlayerModel(pPlayerModel)
  , mpServiceProvider(pServiceProvider)
  , mpEntityFactory(pEntityFactory)
  , mFireButtonPressed(false)
  , mShotRequested(false)
{
}


template<typename EntityFactoryT>
bool AttackSystem<EntityFactoryT>::attackPossible() const {
  auto& playerState =
    *mPlayerEntity.component<const components::PlayerControlled>();

  return
    playerState.mState != PlayerState::ClimbingLadder &&
    playerState.mState != PlayerState::Dieing &&
    playerState.mState != PlayerState::Dead &&
    !playerState.mIsInteracting;
}


template<typename EntityFactoryT>
void AttackSystem<EntityFactoryT>::update() {
  using engine::components::WorldPosition;

  if (!attackPossible()) {
    return;
  }

  assert(mPlayerEntity.has_component<components::PlayerControlled>());
  assert(mPlayerEntity.has_component<WorldPosition>());

  auto& playerState =
    *mPlayerEntity.component<components::PlayerControlled>();
  const auto& playerPosition =
    *mPlayerEntity.component<WorldPosition>();

  const auto shouldFireViaRapidFire =
    mFireButtonPressed &&
    !playerState.mShotFired &&
    mpPlayerModel->hasItem(data::InventoryItemType::RapidFire);

  if (mShotRequested || shouldFireViaRapidFire) {
    fireShot(playerPosition, playerState);
    mShotRequested = false;
    playerState.mShotFired = true;
  } else {
    playerState.mShotFired = false;
  }
}


template<typename EntityFactoryT>
void AttackSystem<EntityFactoryT>::buttonStateChanged(
  const PlayerInputState& inputState
) {
  if (
    attackPossible() &&
    inputState.mShooting &&
    !mFireButtonPressed &&
    !mShotRequested
  ) {
    mShotRequested = true;
  }

  mFireButtonPressed = inputState.mShooting;
}


template<typename EntityFactoryT>
void AttackSystem<EntityFactoryT>::fireShot(
  const engine::components::WorldPosition& playerPosition,
  const components::PlayerControlled& playerState
) {
  using engine::components::WorldPosition;

  const auto facingRight =
    playerState.mOrientation == player::Orientation::Right;
  const auto state = playerState.mState;

  const auto shotOffsetHorizontal = state == PlayerState::LookingUp
    ? (facingRight ? 2 : 0)
    : (facingRight ? 3 : -1);
  const auto shotOffsetVertical = state != PlayerState::Crouching
    ? (state == PlayerState::LookingUp ? -5 : -2)
    : -1;

  const auto shotOffset =
    WorldPosition{shotOffsetHorizontal, shotOffsetVertical};

  mpEntityFactory->createProjectile(
    projectileTypeForWeapon(mpPlayerModel->mWeapon),
    shotOffset + playerPosition,
    shotDirection(playerState.mState, playerState.mOrientation));
  mpServiceProvider->playSound(soundIdForWeapon(mpPlayerModel->mWeapon));

  if (mpPlayerModel->currentWeaponConsumesAmmo()) {
    --mpPlayerModel->mAmmo;

    if (mpPlayerModel->mAmmo <= 0) {
      mpPlayerModel->switchToWeapon(data::WeaponType::Normal);
    }
  }
}

}}}
