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

#include "earth_quake_effect.hpp"

#include "engine/random_number_generator.hpp"
#include "game_service_provider.hpp"


namespace rigel { namespace engine {

EarthQuakeEffect::EarthQuakeEffect(
  IGameServiceProvider* pServiceProvider,
  RandomNumberGenerator* pRandomGenerator
)
  : mCountdown(0)
  , mThreshold(0)
  , mpServiceProvider(pServiceProvider)
  , mpRandomGenerator(pRandomGenerator)
{
}


int EarthQuakeEffect::update() {
  int shakeOffset = 0;

  if (mCountdown <= 0) {
    // Once the countdown reaches 0, determine a new countdown and threshold
    mCountdown = mpRandomGenerator->gen() - 1;
    mThreshold = mpRandomGenerator->gen() % 50;
  } else {
    if (mCountdown < mThreshold) {
      // Shake the screen or play the sound
      const auto randomNumber = mpRandomGenerator->gen() & 0x3;
      if (randomNumber == 0) {
        mpServiceProvider->playSound(data::SoundId::EarthQuake);
      } else {
        shakeOffset = randomNumber;
      }
    }

    --mCountdown;
  }

  return shakeOffset;
}

}}
