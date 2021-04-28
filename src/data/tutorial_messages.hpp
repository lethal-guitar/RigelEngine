/* Copyright (C) 2018, Nikolai Wuttke. All rights reserved.
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

#include <bitset>


namespace rigel::data
{

// The values are explicitly given in order to easily see which message has
// which id. The ids are chosen to match how the original game references these
// messages.
enum class TutorialMessageId
{
  FoundRapidFire = 0,
  FoundHealthMolecule = 1,
  FoundRegularWeapon = 2,
  FoundLaser = 3,
  FoundFlameThrower = 4,
  FoundRocketLauncher = 5,
  EarthQuake = 6,
  FoundBlueKey = 7,
  FoundAccessCard = 8,
  FoundSpaceShip = 9,
  FoundLetterN = 10,
  FoundLetterU = 11,
  FoundLetterK = 12,
  FoundLetterE = 13,
  KeyNeeded = 14,
  AccessCardNeeded = 15,
  CloakNeeded = 16,
  RadarsStillFunctional = 17,
  HintGlobeNeeded = 18,
  FoundTurboLift = 19,
  FoundTeleporter = 20,
  LettersCollectedRightOrder = 21,
  FoundSoda = 22,
  FoundForceField = 23,
  FoundDoor = 24,

  LAST_ID
};


constexpr auto NUM_TUTORIAL_MESSAGES =
  static_cast<int>(TutorialMessageId::LAST_ID);


const char* messageText(TutorialMessageId id);


class TutorialMessageState
{
public:
  void markAsShown(TutorialMessageId id);
  bool hasBeenShown(TutorialMessageId id) const;

private:
  std::bitset<NUM_TUTORIAL_MESSAGES> mMessagesShownMask;
};

} // namespace rigel::data
