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

#include "base/spatial_types.hpp"
#include "base/warnings.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/Entity.h>
RIGEL_RESTORE_WARNINGS


namespace rigel::game_logic
{

namespace components
{

namespace parameter_aliases
{

using Damage = int;
using Health = int;
using GivenScore = int;
using IsFatal = bool;
using DestroyOnContact = bool;

} // namespace parameter_aliases


struct PlayerDamaging
{
  explicit PlayerDamaging(
    const int amount,
    const bool isFatal = false,
    const bool destroyOnContact = false)
    : mAmount(amount)
    , mIsFatal(isFatal)
    , mDestroyOnContact(destroyOnContact)
  {
  }

  int mAmount;
  bool mIsFatal;
  bool mDestroyOnContact;
};


struct Shootable
{
  explicit Shootable(const int health, const int givenScore = 0)
    : mHealth(health)
    , mGivenScore(givenScore)
  {
  }

  int mHealth;
  int mGivenScore;
  bool mInvincible = false;
  bool mEnableHitFeedback = true;
  bool mDestroyWhenKilled = true;

  /** When set, the shootable will immediately destroy any
   * inflictor that hits it, even if that inflictor has 'destroy on contact'
   * set to false.
   */
  bool mAlwaysConsumeInflictor = false;

  bool mCanBeHitWhenOffscreen = false;
};


struct DamageInflicting
{
  explicit DamageInflicting(
    const int amount,
    const bool destroyOnContact = true)
    : mAmount(amount)
    , mDestroyOnContact(destroyOnContact)
  {
  }

  int mAmount;
  bool mDestroyOnContact;
  bool mHasCausedDamage = false;
};


struct CustomDamageApplication
{
};


struct PlayerProjectile
{
  enum class Type
  {
    Normal,
    Laser,
    Rocket,
    Flame,
    ShipLaser,
    ReactorDebris
  };

  explicit PlayerProjectile(const Type type)
    : mType(type)
  {
  }

  Type mType;
};

} // namespace components


namespace events
{

struct ShootableDamaged
{
  entityx::Entity mEntity;
  entityx::Entity mInflictorEntity;
};


struct ShootableKilled
{
  entityx::Entity mEntity;
  base::Vec2f mInflictorVelocity;
};

} // namespace events

} // namespace rigel::game_logic
