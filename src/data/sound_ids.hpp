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


namespace rigel::data {

enum class SoundId {
  DukeNormalShot = 0,
  BigExplosion = 1,
  DukePain = 2,
  DukeDeath = 3,
  Explosion = 4,
  MenuSelect = 5,
  GlassBreaking = 6,
  DukeLaserShot = 7,
  ItemPickup = 8,
  WeaponPickup = 9,
  EnemyHit = 10,
  Swoosh = 11,
  FlameThrowerShot = 12,
  DukeJumping = 13,
  LavaFountain = 14,
  DukeLanding = 15,
  DukeAttachClimbable = 16,
  IngameMessageTyping = 17,
  HammerSmash = 18,
  BlueKeyDoorOpened = 19,
  AlternateExplosion = 20,
  WaterDrop = 21,
  ForceFieldFizzle = 22,
  Unknown1 = 23,
  SlidingDoor = 24,
  MenuToggle = 25,
  FallingRock = 26,
  EnemyLaserShot = 27,
  EarthQuake = 28,
  BiologicalEnemyDestroyed = 29,
  Teleport = 30,
  Unknown3 = 31,
  HealthPickup = 32,
  LettersCollectedCorrectly = 33,

  // Intro sounds
  IntroGunShot = 34,
  IntroGunShotLow = 35,
  IntroEmptyShellsFalling = 36,
  IntroTargetMovingCloser = 37,
  IntroTargetStopsMoving = 38,
  IntroDukeSpeaks1 = 39,
  IntroDukeSpeaks2 = 40
};


namespace detail {

const auto FirstSoundId = SoundId::DukeNormalShot;
const auto LastSoundId = SoundId::IntroDukeSpeaks2;

inline SoundId nextSoundId(const SoundId id) {
  const auto nextIndex = static_cast<int>(id) + 1;
  return static_cast<SoundId>(nextIndex);
}

}


template<typename Callable>
void forEachSoundId(Callable callback) {
  using namespace data::detail;
  for (auto id = FirstSoundId; id != LastSoundId; id = nextSoundId(id)) {
    callback(id);
  }
  callback(LastSoundId);
}

}
