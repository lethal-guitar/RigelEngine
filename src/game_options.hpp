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

#include "engine/timing.hpp"


namespace rigel {

enum class GameSpeed {
  Lowest,
  VeryLow,
  Low,
  Medium,
  High,
  VeryHigh,
  Highest
};


constexpr GameSpeed DEFAULT_GAME_SPEED = GameSpeed::Medium;


enum class SpeedProfile {
  // CPU: AMD N80L286-16/S, 16 MHz
  // Cache: N/A
  // Mainboard: OCTEK Fox II Rev. 3.2
  // Graphics card: Cirrus Logic CL-GD5420-75QC-C (ISA)
  P286_16,

  // CPU: AMD 80386DX, 40 MHz
  // Cache: 128 kB, 20 ns
  // Mainboard: TODO
  // Graphics card: TODO
  P386_DX_40,

  // CPU: Intel 80486DX-33, 33 MHz
  // Cache: 128 kB, 20 ns
  // Mainboard: Chicony CH-486-33/50 L
  // Graphics card: Cirrus Logic CL-GD5422-80-C (ISA)
  P486_DX_33,

  // CPU: Intel 80486DX2-66, 66 MHz
  // Cache: 256 kB, 20 ns
  // Mainboard: Pine Tech PT-426, UMC Chipset
  // Graphics card: Tseng Labs ET4000 (VLB)
  P486_DX2_66_ET4000,

  NO_DELAY
};


// These are based on benchmarks taken from the original game running on real
// hardware. See the comments in the SpeedProfile enum for more information
// about the hardware.
//
// To measure frame rate, the game's binary was modified to toggle a pin on the
// parallel port each frame. An external hardware device was connected to the
// computer's parallel port to measure the time between signals.
constexpr engine::TimeDelta DELAY_FOR_PROFILE[] = {
  // TODO: Refine after taking sub-millisecond frame time measurements
  engine::slowTicksToTime(7),
  engine::slowTicksToTime(3),
  engine::slowTicksToTime(2),
  engine::slowTicksToTime(1),
  0.0
};


inline engine::TimeDelta delayForProfile(const SpeedProfile profile) {
  return DELAY_FOR_PROFILE[static_cast<std::size_t>(profile)];
}


struct GameOptions {
  GameSpeed mSpeed = DEFAULT_GAME_SPEED;
  SpeedProfile mSpeedProfile = SpeedProfile::P486_DX2_66_ET4000;
};

}
