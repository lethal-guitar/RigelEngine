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

#include "common/global.hpp"
#include "data/game_traits.hpp"
#include "data/map.hpp"
#include "engine/physical_components.hpp"
#include "game_logic/player.hpp"

#include <algorithm>

namespace ex = entityx;


namespace rigel::game_logic {

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
constexpr auto IN_SHIP_DEAD_ZONE_START_X = 12;
constexpr auto DEAD_ZONE_END_X = 21;

constexpr auto DEFAULT_VERTICAL_DEAD_ZONE = VerticalDeadZone{2, 19};
constexpr auto TIGHT_VERTICAL_DEAD_ZONE = VerticalDeadZone{7, 13};

constexpr auto INITIAL_CAMERA_OFFSET = base::Vector{15, 19};

constexpr auto MANUAL_SROLL_COOLDOWN_AFTER_SHOOTING = 4;


bool shouldUseTightDeadZone(const Player& player) {
  return
    player.stateIs<ClimbingLadder>() ||
    player.stateIs<PushedByFan>() ||
    player.stateIs<UsingJetpack>() ||
    player.stateIs<InShip>() ||
    player.isRidingElevator();
}


const VerticalDeadZone& deadZoneForStateOf(const Player& player) {
  return shouldUseTightDeadZone(player)
    ? TIGHT_VERTICAL_DEAD_ZONE
    : DEFAULT_VERTICAL_DEAD_ZONE;
}


base::Rect<int> deadZoneRect(const Player& player) {
  const auto& verticalDeadZone = deadZoneForStateOf(player);
  const auto deadZoneStartX = player.stateIs<InShip>()
    ? IN_SHIP_DEAD_ZONE_START_X : DEAD_ZONE_START_X;

  return base::makeRect<int>(
    {deadZoneStartX, verticalDeadZone.mStart},
    {DEAD_ZONE_END_X, verticalDeadZone.mEnd});
}


/** Calculate 'normalized' player bounds
 *
 * Returns player collision box in world space, adjusted to always be in the
 * center of the screen with regards to the original game's horizontal screen
 * size.
 *
 * This makes the camera code work correctly when in widescreen mode. The
 * dead zone is tailored towards normal (i.e. not widescreen) mode, which
 * would cause the player to be constrained to move inside the left half of
 * the screen when in widescreen mode. By shifting the player position, we
 * effectively move the dead zone to the center of the screen instead.
 *
 * When the view port is not wide, the result is identical with the player's
 * world space collision box.
 */
base::Rect<int> normalizedPlayerBounds(
  const Player& player,
  const base::Extents& viewPortSize
) {
  const auto extraTiles =
    viewPortSize.width - data::GameTraits::mapViewPortSize.width;
  const auto offsetToCenter = extraTiles / 2;

  auto playerBounds = player.worldSpaceCollisionBox();
  playerBounds.topLeft.x -= offsetToCenter;

  return playerBounds;
}


base::Vector offsetToDeadZone(
  const Player& player,
  const base::Vector& cameraPosition,
  const base::Extents& viewPortSize
) {
  const auto playerBounds = normalizedPlayerBounds(player, viewPortSize);
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
  , mpMap(&map)
  , mViewPortSize(data::GameTraits::mapViewPortSize)
{
  eventManager.subscribe<rigel::events::PlayerFiredShot>(*this);
}


void Camera::update(const PlayerInput& input, const base::Extents& viewPortSize) {
  mViewPortSize = viewPortSize;
  updateManualScrolling(input);

  if (!mpPlayer->stateIs<GettingSuckedIntoSpace>()) {
    updateAutomaticScrolling();
  } else {
    const auto offset = 2 *
      engine::orientation::toMovement(mpPlayer->orientation());
    setPosition(mPosition + base::Vector{offset, 0});
  }
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
  const auto [offsetX, offsetY] =
    offsetToDeadZone(*mpPlayer, mPosition, mViewPortSize);

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
  const auto maxPosition = base::Extents{
    static_cast<int>(mpMap->width() - mViewPortSize.width),
    static_cast<int>(mpMap->height() - mViewPortSize.height)};
  mPosition.x = std::clamp(position.x, 0, maxPosition.width);
  mPosition.y = std::clamp(position.y, 0, maxPosition.height);
}


void Camera::centerViewOnPlayer() {
  auto playerPos = normalizedPlayerBounds(*mpPlayer, mViewPortSize).bottomLeft();
  if (mpPlayer->orientation() == Orientation::Left) {
    playerPos.x -= 1;
  }

  setPosition(playerPos - INITIAL_CAMERA_OFFSET);
}


void Camera::receive(const rigel::events::PlayerFiredShot& event) {
  mManualScrollCooldown = MANUAL_SROLL_COOLDOWN_AFTER_SHOOTING;
}

}
