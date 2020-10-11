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

#include "common/game_service_provider.hpp"
#include "common/global.hpp"
#include "engine/random_number_generator.hpp"


namespace rigel::game_logic {

EarthQuakeEffect::EarthQuakeEffect(
  IGameServiceProvider* pServiceProvider,
  engine::RandomNumberGenerator* pRandomGenerator,
  entityx::EventManager* pEvents
)
  : mCountdown(0)
  , mThreshold(0)
  , mpServiceProvider(pServiceProvider)
  , mpRandomGenerator(pRandomGenerator)
  , mpEvents(pEvents)
{
}


void EarthQuakeEffect::synchronizeTo(const EarthQuakeEffect& other) {
  mCountdown = other.mCountdown;
  mThreshold = other.mThreshold;
}


void EarthQuakeEffect::update() {
  if (mCountdown <= 0) {
    // Once the countdown reaches 0, determine a new countdown and threshold
    mCountdown = mpRandomGenerator->gen() - 1;
    mThreshold = mpRandomGenerator->gen() % 50;
  } else {
    if (mCountdown < mThreshold) {
      // Shake the screen or play the sound
      const auto randomNumber = mpRandomGenerator->gen() % 4;
      if (randomNumber == 0) {
        mpServiceProvider->playSound(data::SoundId::EarthQuake);
      } else {
        mpEvents->emit(events::ScreenShake{randomNumber});
      }
    }

    --mCountdown;
  }
}


bool EarthQuakeEffect::isQuaking() const {
  return mCountdown < mThreshold && mCountdown != 0;
}

}
