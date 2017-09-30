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


namespace rigel { namespace game_logic { namespace components {

namespace parameter_aliases {

using Damage = int;
using Health = int;
using GivenScore = int;
using IgnoreMercyFrames = bool;
using DestroyOnContact = bool;

}


struct PlayerDamaging {
  explicit PlayerDamaging(
    const int amount,
    const bool ignoreMercyFrames = false,
    const bool destroyOnContact = false
  )
    : mAmount(amount)
    , mIgnoreMercyFrames(ignoreMercyFrames)
    , mDestroyOnContact(destroyOnContact)
  {
  }

  int mAmount;
  bool mIgnoreMercyFrames;
  bool mDestroyOnContact;
};


struct Shootable {
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
};


struct DamageInflicting {
  explicit DamageInflicting(
    const int amount,
    const bool destroyOnContact = true)
    : mAmount(amount)
    , mDestroyOnContact(destroyOnContact)
  {
  }

  int mAmount;
  bool mDestroyOnContact;
};

}}}
