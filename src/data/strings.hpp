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

namespace rigel::data
{

struct Messages
{

  static constexpr auto PressKeyToUse = "Press a key to use..";

  static constexpr auto KeyAlreadyUsed =
    "THAT KEY IS ALREADY IN USE!\n   Select another key.";

  static constexpr auto CalibrationTopLeft =
    "Move the joystick towards the\nUPPER LEFT and press a button.";

  static constexpr auto CalibrationBottomRight =
    "Move the joystick towards the\nLOWER RIGHT and press a button.";

  static constexpr auto CalibrationButtons =
    "Select fire button.  The other\nbutton is used for jumping.";

  static constexpr auto DestroyedEverything =
    "DUKE... YOU HAVE DESTROYED*EVERYTHING.  EXCELLENT...";

  static constexpr auto LettersCollectedWrongOrder =
    "OH, WELL... TEN THOUSAND POINTS IS*BETTER THAN NOTHING.";

  static constexpr auto AccessGranted = "ACCESS GRANTED.";

  static constexpr auto OpeningDoor = "OPENING DOOR.";

  static constexpr auto FoundCloak =
    "YOU ARE NOW INVINCIBLE FOR A SHORT*PERIOD OF TIME...**"
    "USE THIS TO ALSO DISABLE*THE FORCE FIELD...";

  static constexpr auto FoundSpecialHintGlobe =
    "BRING THIS SPECIAL CRYSTAL GLOBE*BACK TO THE PEDESTAL TO RECEIVE A*"
    "SPECIAL HINT FOR THIS LEVEL.";

  static constexpr auto CloakTimingOut = "CLOAK IS DISABLING...";

  static constexpr auto RapidFireTimingOut = "RAPID FIRE IS DISABLING...";

  static constexpr auto FoundRespawnBeacon = "SECTOR SECURE!!!";

  static constexpr auto ForceFieldDestroyed =
    "FORCE FIELD DESTROYED... *GOOD WORK...";

  static constexpr auto FindAllRadars =
    "DUKE, FIND AND DESTROY ALL THE*RADAR DISHES ON THIS LEVEL.";

  static constexpr auto WelcomeToDukeNukem2 = "WELCOME TO DUKE NUKEM II!";
};

} // namespace rigel::data
