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

#include "base/warnings.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS


namespace rigel
{
struct IGameServiceProvider;
}
namespace rigel::engine
{
class RandomNumberGenerator;
}


namespace rigel::game_logic
{

class EarthQuakeEffect
{
public:
  EarthQuakeEffect(
    IGameServiceProvider* pServiceProvider,
    engine::RandomNumberGenerator* pRandomGenerator,
    entityx::EventManager* pEvents);

  void synchronizeTo(const EarthQuakeEffect& other);

  void update();
  bool isQuaking() const;

private:
  int mCountdown;
  int mThreshold;

  IGameServiceProvider* mpServiceProvider;
  engine::RandomNumberGenerator* mpRandomGenerator;
  entityx::EventManager* mpEvents;
};

} // namespace rigel::game_logic
