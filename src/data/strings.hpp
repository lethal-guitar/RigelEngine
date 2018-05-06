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

namespace rigel { namespace data {

struct Messages {

  static constexpr auto PressKeyToUse =
    "Press a key to use..";

  static constexpr auto KeyAlreadyUsed =
    "THAT KEY IS ALREADY IN USE!\n   Select another key.";

  static constexpr auto CalibrationTopLeft =
    "Move the joystick towards the\nUPPER LEFT and press a button.";

  static constexpr auto CalibrationBottomRight =
    "Move the joystick towards the\nLOWER RIGHT and press a button.";

  static constexpr auto CalibrationButtons =
    "Select fire button.  The other\nbutton is used for jumping.";

  static constexpr auto EarthQuake =
    "E A R T H Q U A K E ! ! !*WAIT, THIS IS NOT EARTH.";

  static constexpr auto DestroyedEverything =
    "DUKE... YOU HAVE DESTROYED*EVERYTHING.  EXCELLENT...";

  static constexpr auto LettersCollectedWrongOrder =
    "OH, WELL... TEN THOUSAND POINTS IS*BETTER THAN NOTHING.";

  static constexpr auto NeedCloak =
    "OUCH, YOU NEED TO FIND THE CLOAKING*DEVICE TO DISABLE THIS FORCEFIELD.";

  static constexpr auto CanUseAccessCard =
    "USE THE ACCESS CARD TO DISABLE*THIS FORCE FIELD.";

  static constexpr auto AccessDenied =
    "ACCESS DENIED.";

  static constexpr auto AccessGranted = // NOFLAGS
    "ACCESS GRANTED.";

  static constexpr auto CanUseBlueKey =
    "USE A KEY TO OPEN THIS DOOR.";

  static constexpr auto NeedBlueKey =
    "YOU NEED A KEY TO OPEN*THE DOOR.";

  static constexpr auto OpeningDoor = // NOFLAGS
    "OPENING DOOR.";

  static constexpr auto FoundSpaceShip =
    "THE SHIP. USE THIS TO GET OUT OF THIS*LEVEL.  YOU CAN FLY ANYWHERE,"
    "AND*YOUR WEAPON IS VERY POWERFUL.";

  static constexpr auto FoundRegularWeapon =
    "THIS IS YOUR REGULAR WEAPON!";

  static constexpr auto FoundLaser =
    "THIS WEAPON CAN SHOOT*THROUGH ANYTHING!";

  static constexpr auto FoundFlameThrower =
    "THERE ARE MANY SECRETS WITH*THIS WEAPON. USE IT TO*YOUR ADVANTAGE!";

  static constexpr auto FoundRocketLauncher =
    "A ROCKET LAUNCHER IS VERY*DEADLY. IT IS THE STRONGEST OF*ANY WEAPON!";

  static constexpr auto FoundSoda =
    "THE CAFFEINE IN SODAS PROVIDES*ONE UNIT OF HEALTH.";

  static constexpr auto FoundHealthMolecule =
    "THIS ITEM WILL GIVE YOU ONE UNIT*OF HEALTH.";

  static constexpr auto FoundLetterN =
    "GOT THE N.  WHAT IS NEXT?";

  static constexpr auto FoundLetterU =
    "GOT THE U.";

  static constexpr auto FoundLetterK =
    "GOT THE K.";

  static constexpr auto FoundLetterE =
    "GOT THE E.";

  static constexpr auto LettersCollectedRightOrder =
    "EXCELLENT!  ONE HUNDRED THOUSAND*POINTS!!!!!!!!!!!";

  static constexpr auto FoundCloak = // NOFLAGS?
    "YOU ARE NOW INVINCIBLE FOR A SHORT*PERIOD OF TIME...**USE THIS TO ALSO DISABLE*THE FORCE FIELD...";

  static constexpr auto FoundBlueKey =
    "FIND THE DOOR THAT*THIS KEY OPENS.";

  static constexpr auto FoundAccessCard =
    "USE THE CARD TO TURN OFF*FORCE FIELDS.";

  static constexpr auto FoundRapidFire =
    "HOLD DOWN YOUR FIRE BUTTON FOR*RAPID FIRE.";

  static constexpr auto NeedBlueHintGlobe =
    "THIS DEVICE WILL GIVE SPECIFIC HINTS.*FIND THE SPECIAL BLUE GLOBE AND*"
    "BRING IT BACK HERE.";

  static constexpr auto FoundSpecialHintGlobe =
    "BRING THIS SPECIAL CRYSTAL GLOBE*BACK TO THE PEDESTAL TO RECEIVE A*"
    "SPECIAL HINT FOR THIS LEVEL.";

  static constexpr auto TeleporterHelp =
    "PRESS UP OR ENTER TO USE*THE TRANSPORTER.";

  static constexpr auto CloakTimingOut = // NOFLAGS
    "*CLOAK IS DISABLING...";

  static constexpr auto RapidFireTimingOut = // NOFLAGS
    "RAPID FIRE IS DISABLING...";

  static constexpr auto FoundRespawnBeacon = // NOFLAGS
    "SECTOR SECURE!!!";

  static constexpr auto NeedToDestroyRadars =
    " WAIT!!!!!!!!      *YOU NEED TO DESTROY ALL THE RADAR*"
    "DISHES FIRST BEFORE YOU CAN COMPLETE*THE LEVEL...";

  static constexpr auto ForceFieldDestroyed = // NOFLAGS
    "FORCE FIELD DESTROYED... *GOOD WORK...";

  static constexpr auto TurboLiftHelp =
    "PRESS UP OR DOWN TO USE THE*TURBO LIFT.";

  static constexpr auto FindAllRadars = // NOFLAGS
    "DUKE, FIND AND DESTROY ALL THE*RADAR DISHES ON THIS LEVEL.";

  static constexpr auto WelcomeToDukeNukem2 = // NOFLAGS
    "WELCOME TO DUKE NUKEM II!";
};

}}
