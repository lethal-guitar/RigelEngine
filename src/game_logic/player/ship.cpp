/* Copyright (C) 2019, Nikolai Wuttke. All rights reserved.
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

#include "ship.hpp"

#include "common/game_service_provider.hpp"
#include "common/global.hpp"
#include "data/sound_ids.hpp"
#include "engine/base_components.hpp"
#include "engine/collision_checker.hpp"
#include "engine/physical_components.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic::behaviors
{

PlayerShip::PlayerShip(const bool hasJustBeenExited)
  : mPickUpCoolDownFrames(hasJustBeenExited ? 20 : 0)
{
}


void PlayerShip::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool,
  entityx::Entity entity)
{
  using engine::components::BoundingBox;
  using engine::components::WorldPosition;

  const auto& position = *entity.component<WorldPosition>();
  const auto& bbox = *entity.component<BoundingBox>();

  if (mPickUpCoolDownFrames > 0)
  {
    --mPickUpCoolDownFrames;
  }

  const auto worldSpaceBbox = engine::toWorldSpace(bbox, position);
  const auto isTouchingPlayer =
    s.mpPlayer->worldSpaceHitBox().intersects(worldSpaceBbox);

  // clang-format off
  if (
    mPickUpCoolDownFrames == 0 &&
    isTouchingPlayer &&
    d.mpCollisionChecker->isOnSolidGround(worldSpaceBbox) &&
    s.mpPlayer->stateIs<Falling>())
  // clang-format on
  {
    d.mpEvents->emit(
      rigel::events::TutorialMessage{data::TutorialMessageId::FoundSpaceShip});
    d.mpServiceProvider->playSound(data::SoundId::WeaponPickup);

    s.mpPlayer->enterShip(
      position, *entity.component<engine::components::Orientation>());
    entity.destroy();
  }
}

} // namespace rigel::game_logic::behaviors
