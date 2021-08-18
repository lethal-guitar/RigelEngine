/* Copyright (C) 2020, Nikolai Wuttke. All rights reserved.
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

#include "menu_navigation.hpp"


namespace rigel::ui
{

namespace
{

constexpr auto ANALOG_STICK_DEADZONE = 20'000;

}


bool isNonRepeatKeyDown(const SDL_Event& event)
{
  return event.type == SDL_KEYDOWN && event.key.repeat == 0;
}


bool isButtonPress(const SDL_Event& event)
{
  return event.type == SDL_KEYDOWN || event.type == SDL_CONTROLLERBUTTONDOWN;
}


bool isConfirmButton(const SDL_Event& event)
{
  const auto enterPressed = isNonRepeatKeyDown(event) &&
    (event.key.keysym.sym == SDLK_RETURN ||
     event.key.keysym.sym == SDLK_KP_ENTER);
  const auto buttonAPressed = event.type == SDL_CONTROLLERBUTTONDOWN &&
    event.cbutton.button == SDL_CONTROLLER_BUTTON_A;

  return enterPressed || buttonAPressed;
}


bool isMenuConfirmButton(const SDL_Event& event)
{
  return isConfirmButton(event) ||
    (isNonRepeatKeyDown(event) && event.key.keysym.sym == SDLK_SPACE);
}


bool isCancelButton(const SDL_Event& event)
{
  const auto escapePressed =
    isNonRepeatKeyDown(event) && event.key.keysym.sym == SDLK_ESCAPE;
  const auto buttonBPressed = event.type == SDL_CONTROLLERBUTTONDOWN &&
    event.cbutton.button == SDL_CONTROLLER_BUTTON_B;

  return escapePressed || buttonBPressed;
}


bool isQuitConfirmButton(const SDL_Event& event)
{
  const auto yPressed =
    event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_y;
  const auto buttonAPressed = event.type == SDL_CONTROLLERBUTTONDOWN &&
    event.cbutton.button == SDL_CONTROLLER_BUTTON_A;

  return yPressed || buttonAPressed;
}


NavigationEvent MenuNavigationHelper::convert(const SDL_Event& event)
{
  auto handleAxisMotion = [&](int& value, const std::int16_t newValueRaw) {
    auto result = NavigationEvent::None;

    const auto newValue =
      base::applyThreshold(newValueRaw, ANALOG_STICK_DEADZONE);
    if (value >= 0 && newValue < 0)
    {
      result = NavigationEvent::NavigateUp;
    }
    if (value <= 0 && newValue > 0)
    {
      result = NavigationEvent::NavigateDown;
    }

    value = newValue;

    return result;
  };


  switch (event.type)
  {
    case SDL_KEYDOWN:
      // NOTE: SDL always sends SDLK_KP_n events for the numpad keys,
      // regardless of the numlock state. Therefore, we need to explicitly
      // handle the numpad arrow keys if we want them to work for navigation.
      switch (event.key.keysym.sym)
      {
        case SDLK_LEFT:
        case SDLK_UP:
        case SDLK_KP_4: // numpad left
        case SDLK_KP_8: // numpad up
          return NavigationEvent::NavigateUp;

        case SDLK_RIGHT:
        case SDLK_DOWN:
        case SDLK_KP_6: // numpad right
        case SDLK_KP_2: // numpad down
          return NavigationEvent::NavigateDown;

        case SDLK_RETURN:
        case SDLK_SPACE:
        case SDLK_KP_ENTER:
          return NavigationEvent::Confirm;

        case SDLK_ESCAPE:
          return NavigationEvent::Cancel;

        default:
          return NavigationEvent::UnassignedButtonPress;
      }

    case SDL_CONTROLLERAXISMOTION:
      switch (event.caxis.axis)
      {
        case SDL_CONTROLLER_AXIS_LEFTX:
        case SDL_CONTROLLER_AXIS_RIGHTX:
          return handleAxisMotion(mAnalogStickVector.x, event.caxis.value);

        case SDL_CONTROLLER_AXIS_LEFTY:
        case SDL_CONTROLLER_AXIS_RIGHTY:
          return handleAxisMotion(mAnalogStickVector.y, event.caxis.value);

        default:
          break;
      }
      break;

    case SDL_CONTROLLERBUTTONDOWN:
      switch (event.cbutton.button)
      {
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
          return NavigationEvent::NavigateUp;

        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
          return NavigationEvent::NavigateDown;

        case SDL_CONTROLLER_BUTTON_A:
          return NavigationEvent::Confirm;

        case SDL_CONTROLLER_BUTTON_B:
          return NavigationEvent::Cancel;

        default:
          return NavigationEvent::UnassignedButtonPress;
      }

    default:
      break;
  }

  return NavigationEvent::None;
}

} // namespace rigel::ui
