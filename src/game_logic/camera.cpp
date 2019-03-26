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

#include "camera.hpp"

#include "base/math_tools.hpp"
#include "data/game_traits.hpp"
#include "data/map.hpp"
#include "engine/physical_components.hpp"
#include "game_logic/player.hpp"
#include "global_level_events.hpp"

namespace ex = entityx;


namespace rigel { namespace game_logic {

namespace {

using namespace engine::components;
using namespace game_logic::components;


struct VerticalDeadZone {
  int mStart;
  int mEnd;
};


constexpr auto MANUAL_SCROLL_ADJUST = 2;
constexpr auto MAX_ADJUST_X = 2;
constexpr auto MAX_ADJUST_UP = 2;
constexpr auto MAX_ADJUST_DOWN = 2;
constexpr auto MAX_ADJUST_DOWN_ELEVATOR = 3;

constexpr auto DEAD_ZONE_START_X = 11;
constexpr auto DEAD_ZONE_END_X = 21;

constexpr auto DEFAULT_VERTICAL_DEAD_ZONE = VerticalDeadZone{2, 19};
constexpr auto TIGHT_VERTICAL_DEAD_ZONE = VerticalDeadZone{7, 13};

constexpr auto MANUAL_SROLL_COOLDOWN_AFTER_SHOOTING = 4;


bool shouldUseTightDeadZone(const Player& player) {
  return
    player.stateIs<ClimbingLadder>() ||
    player.stateIs<PushedByFan>() ||
    player.isRidingElevator();
}


const VerticalDeadZone& deadZoneForStateOf(const Player& player) {
  return shouldUseTightDeadZone(player)
    ? TIGHT_VERTICAL_DEAD_ZONE
    : DEFAULT_VERTICAL_DEAD_ZONE;
}


base::Rect<int> deadZoneRect(const Player& player) {
  const auto& verticalDeadZone = deadZoneForStateOf(player);

  return base::makeRect<int>(
    {DEAD_ZONE_START_X, verticalDeadZone.mStart},
    {DEAD_ZONE_END_X, verticalDeadZone.mEnd});
}


base::Vector offsetToDeadZone(
  const Player& player,
  const base::Vector& cameraPosition
) {
  const auto playerBounds = player.worldSpaceCollisionBox();

  auto worldSpaceDeadZone = deadZoneRect(player);
  worldSpaceDeadZone.topLeft += cameraPosition;

  // horizontal
  const auto offsetLeft =
    std::max(0, worldSpaceDeadZone.topLeft.x -  playerBounds.topLeft.x);
  const auto offsetRight =
    std::min(0,
      worldSpaceDeadZone.bottomRight().x - playerBounds.bottomRight().x);
  const auto offsetX = -offsetLeft - offsetRight;

  // vertical
  const auto offsetTop =
    std::max(0, worldSpaceDeadZone.top() - playerBounds.top());
  const auto offsetBottom =
    std::min(0,
      worldSpaceDeadZone.bottom() - playerBounds.bottom());
  const auto offsetY = -offsetTop - offsetBottom;

  return {offsetX, offsetY};
}

}


Camera::Camera(
  const Player* pPlayer,
  const data::map::Map& map,
  entityx::EventManager& eventManager
)
  : mpPlayer(pPlayer)
  , mMaxPosition(base::Extents{
    static_cast<int>(map.width() - data::GameTraits::mapViewPortWidthTiles),
    static_cast<int>(map.height() - data::GameTraits::mapViewPortHeightTiles)})
{
  eventManager.subscribe<rigel::events::PlayerFiredShot>(*this);
}


void Camera::update(const PlayerInput& input) {
  updateManualScrolling(input);
  updateAutomaticScrolling();
}


void Camera::updateManualScrolling(const PlayerInput& input) {
  if (mManualScrollCooldown > 0) {
    const auto isApplicable =
      (mpPlayer->stateIs<OnGround>() && input.mDown) ||
      (mpPlayer->stateIs<OnPipe>() && input.mUp);
    if (isApplicable) {
      --mManualScrollCooldown;
      return;
    }
  }

  if (mpPlayer->stateIs<OnGround>() || mpPlayer->stateIs<OnPipe>()) {
    if (input.mDown) {
      mPosition.y += MANUAL_SCROLL_ADJUST;
    }
    if (input.mUp) {
      mPosition.y -= MANUAL_SCROLL_ADJUST;
    }
  }
}


void Camera::updateAutomaticScrolling() {
  const auto [offsetX, offsetY] = offsetToDeadZone(*mpPlayer, mPosition);

  const auto maxAdjustDown = mpPlayer->isRidingElevator()
    ? MAX_ADJUST_DOWN_ELEVATOR
    : MAX_ADJUST_DOWN;
  const auto adjustment = base::Vector{
    std::clamp(offsetX, -MAX_ADJUST_X, MAX_ADJUST_X),
    std::clamp(offsetY, -MAX_ADJUST_UP, maxAdjustDown)
  };

  setPosition(mPosition + adjustment);
}


void Camera::setPosition(const base::Vector position) {
  mPosition.x = base::clamp(position.x, 0, mMaxPosition.width);
  mPosition.y = base::clamp(position.y, 0, mMaxPosition.height);
}


void Camera::centerViewOnPlayer() {
  setPosition(offsetToDeadZone(*mpPlayer, {}));
}


void Camera::receive(const rigel::events::PlayerFiredShot& event) {
  mManualScrollCooldown = MANUAL_SROLL_COOLDOWN_AFTER_SHOOTING;
}

}}
