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

  static const auto PressKeyToUse =
    "Press a key to use..";

  static const auto KeyAlreadyUsed =
    "THAT KEY IS ALREADY IN USE!\n   Select another key.";

  static const auto CalibrationTopLeft =
    "Move the joystick towards the\nUPPER LEFT and press a button.";

  static const auto CalibrationBottomRight =
    "Move the joystick towards the\nLOWER RIGHT and press a button.";

  static const auto CalibrationButtons =
    "Select fire button.  The other\nbutton is used for jumping.";

  static const auto EarthQuake =
    "E A R T H Q U A K E ! ! !*WAIT, THIS IS NOT EARTH.";

  static const auto DestroyedEverything =
    "DUKE... YOU HAVE DESTROYED*EVERYTHING.  EXCELLENT...";

  static const auto LettersCollectedWrongOrder =
    "OH, WELL... TEN THOUSAND POINTS IS*BETTER THAN NOTHING.";

  static const auto NeedCloak =
    "OUCH, YOU NEED TO FIND THE CLOAKING*DEVICE TO DISABLE THIS FORCEFIELD.";

  static const auto CanUseAccessCard =
    "USE THE ACCESS CARD TO DISABLE*THIS FORCE FIELD.";

  static const auto AccessDenied =
    "ACCESS DENIED.";

  static const auto AccessGranted = // NOFLAGS
    "ACCESS GRANTED.";

  static const auto CanUseBlueKey =
    "USE A KEY TO OPEN THIS DOOR.";

  static const auto NeedBlueKey =
    "YOU NEED A KEY TO OPEN*THE DOOR."

  static const auto OpeningDoor = // NOFLAGS
    "OPENING DOOR.";

  static const auto FoundSpaceShip =
    "THE SHIP. USE THIS TO GET OUT OF THIS*LEVEL.  YOU CAN FLY ANYWHERE,"
    "AND*YOUR WEAPON IS VERY POWERFUL.";

  static const auto FoundRegularWeapon =
    "THIS IS YOUR REGULAR WEAPON!";

  static const auto FoundLaser =
    "THIS WEAPON CAN SHOOT*THROUGH ANYTHING!";

  static const auto FoundFlameThrower =
    "THERE ARE MANY SECRETS WITH*THIS WEAPON. USE IT TO*YOUR ADVANTAGE!";

  static const auto FoundRocketLauncher =
    "A ROCKET LAUNCHER IS VERY*DEADLY. IT IS THE STRONGEST OF*ANY WEAPON!";

  static const auto FoundSoda =
    "THE CAFFEINE IN SODAS PROVIDES*ONE UNIT OF HEALTH.";

  static const auto FoundHealthMolecule =
    "THIS ITEM WILL GIVE YOU ONE UNIT*OF HEALTH.";

  static const auto FoundLetterN =
    "GOT THE N.  WHAT IS NEXT?";

  static const auto FoundLetterU =
    "GOT THE U.";

  static const auto FoundLetterK =
    "GOT THE K.";

  static const auto FoundLetterE =
    "GOT THE E.";

  static const auto LettersCollectedRightOrder =
    "EXCELLENT!  ONE HUNDRED THOUSAND*POINTS!!!!!!!!!!!";

  static const auto FoundCloak = // NOFLAGS?
    "YOU ARE NOW INVINCIBLE FOR A SHORT*PERIOD OF TIME...**USE THIS TO ALSO DISABLE*THE FORCE FIELD...";

  static const auto FoundBlueKey =
    "FIND THE DOOR THAT*THIS KEY OPENS.";

  static const auto FoundAccessCard =
    "USE THE CARD TO TURN OFF*FORCE FIELDS.";

  static const auto FoundRapidFire =
    "HOLD DOWN YOUR FIRE BUTTON FOR*RAPID FIRE.";

  static const auto NeedBlueHintGlobe =
    "THIS DEVICE WILL GIVE SPECIFIC HINTS.*FIND THE SPECIAL BLUE GLOBE AND*"
    "BRING IT BACK HERE.";

  static const auto FoundSpecialHintGlobe =
    "BRING THIS SPECIAL CRYSTAL GLOBE*BACK TO THE PEDESTAL TO RECEIVE A*"
    "SPECIAL HINT FOR THIS LEVEL.";

  static const auto TeleporterHelp =
    "PRESS UP OR ENTER TO USE*THE TRANSPORTER.";

  static const auto CloakTimingOut = // NOFLAGS
    "*CLOAK IS DISABLING...";

  static const auto RapidFireTimingOut = // NOFLAGS
    "RAPID FIRE IS DISABLING...";

  static const auto FoundRespawnBeacon = // NOFLAGS
    "SECTOR SECURE!!!";

  static const auto NeedToDestroyRadars =
    " WAIT!!!!!!!!      *YOU NEED TO DESTROY ALL THE RADAR*"
    "DISHES FIRST BEFORE YOU CAN COMPLETE*THE LEVEL...";

  static const auto ForceFieldDestroyed = // NOFLAGS
    "FORCE FIELD DESTROYED... *GOOD WORK...";

  static const auto TurboLiftHelp =
    "PRESS UP OR DOWN TO USE THE*TURBO LIFT.";

  static const auto FindAllRadars = // NOFLAGS
    "DUKE, FIND AND DESTROY ALL THE*RADAR DISHES ON THIS LEVEL.";

  static const auto WelcomeToDukeNukem2 = // NOFLAGS
    "WELCOME TO DUKE NUKEM II!";
}}
