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

#include "tutorial_messages.hpp"

#include <array>


namespace rigel { namespace data {

namespace {

constexpr std::array<const char*, NUM_TUTORIAL_MESSAGES> MESSAGE_TEXTS = {
  "HOLD DOWN YOUR FIRE BUTTON FOR*RAPID FIRE.",
  "THIS ITEM WILL GIVE YOU ONE UNIT*OF HEALTH.",
  "THIS IS YOUR REGULAR WEAPON!",
  "THIS WEAPON CAN SHOOT*THROUGH ANYTHING!",
  "THERE ARE MANY SECRETS WITH*THIS WEAPON. USE IT TO*YOUR ADVANTAGE!",
  "A ROCKET LAUNCHER IS VERY*DEADLY. IT IS THE STRONGEST OF*ANY WEAPON!",
  "E A R T H Q U A K E ! ! !*WAIT, THIS IS NOT EARTH.",
  "FIND THE DOOR THAT*THIS KEY OPENS.",
  "USE THE CARD TO TURN OFF*FORCE FIELDS.",
  "THE SHIP. USE THIS TO GET OUT OF THIS*LEVEL.  YOU CAN FLY ANYWHERE,"
  "AND*YOUR WEAPON IS VERY POWERFUL.",
  "GOT THE N.  WHAT IS NEXT?",
  "GOT THE U.",
  "GOT THE K.",
  "GOT THE E.",
  "YOU NEED A KEY TO OPEN*THE DOOR.",
  "ACCESS DENIED.",
  "OUCH, YOU NEED TO FIND THE CLOAKING*DEVICE TO DISABLE THIS FORCEFIELD.",
  "WAIT!!!!!!!!      *YOU NEED TO DESTROY ALL THE RADAR*"
  "DISHES FIRST BEFORE YOU CAN COMPLETE*THE LEVEL...",
  "THIS DEVICE WILL GIVE SPECIFIC HINTS.*FIND THE SPECIAL BLUE GLOBE AND*"
  "BRING IT BACK HERE.",
  "PRESS UP OR DOWN TO USE THE*TURBO LIFT.",
  "PRESS UP OR ENTER TO USE*THE TRANSPORTER.*",
  "EXCELLENT!  ONE HUNDRED THOUSAND*POINTS!!!!!!!!!!!",
  "THE CAFFEINE IN SODAS PROVIDES*ONE UNIT OF HEALTH.",
  "USE THE ACCESS CARD TO DISABLE*THIS FORCE FIELD.",
  "USE A KEY TO OPEN THIS DOOR."
};


int toIndex(const TutorialMessageId message) {
  return static_cast<int>(message);
}

}


const char* messageText(TutorialMessageId id) {
  return MESSAGE_TEXTS[toIndex(id)];
}


void TutorialMessageState::markAsShown(TutorialMessageId id) {
  mMessagesShownMask.set(toIndex(id));
}


bool TutorialMessageState::hasBeenShown(TutorialMessageId id) const {
  return mMessagesShownMask.test(toIndex(id));
}

}}
