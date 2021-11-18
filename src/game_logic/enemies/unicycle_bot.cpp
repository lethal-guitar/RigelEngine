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

#include "unicycle_bot.hpp"

#include "base/match.hpp"
#include "engine/base_components.hpp"
#include "engine/movement.hpp"
#include "engine/random_number_generator.hpp"
#include "engine/visual_components.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/ientity_factory.hpp"
#include "game_logic/player.hpp"


namespace rigel::game_logic::behaviors
{

void UnicycleBot::update(
  GlobalDependencies& d,
  GlobalState& s,
  bool isOnScreen,
  entityx::Entity entity)
{
  using engine::components::Orientation;

  const auto& position = *entity.component<engine::components::WorldPosition>();
  auto& animationFrame =
    entity.component<engine::components::Sprite>()->mFramesToRender[0];
  auto& orientation = *entity.component<Orientation>();

  auto spawnSmokePuff = [&]() {
    const auto isFacingLeft = orientation == Orientation::Left;
    const auto xOffset = isFacingLeft ? 1 : 0;
    const auto movementType = isFacingLeft ? SpriteMovement::FlyUpperRight
                                           : SpriteMovement::FlyUpperLeft;
    spawnMovingEffectSprite(
      *d.mpEntityFactory,
      data::ActorID::Smoke_puff_FX,
      movementType,
      position + base::Vec2{xOffset, 0});
  };


  base::match(
    mState,
    [&, this](Waiting& state) {
      ++state.mFramesElapsed;
      if (state.mFramesElapsed == 15)
      {
        orientation = position.x < s.mpPlayer->orientedPosition().x
          ? Orientation::Right
          : Orientation::Left;
        mFramesUntilNextTurn = d.mpRandomGenerator->gen() % 32 + 15;
        mState = Accelerating{};
      }

      animationFrame = d.mpRandomGenerator->gen() % 2;
    },

    [&, this](Accelerating& state) {
      animationFrame = s.mpPerFrameState->mIsOddFrame ? 2 : 1;

      if (s.mpPerFrameState->mIsOddFrame)
      {
        spawnSmokePuff();
      }

      --mFramesUntilNextTurn;
      ++state.mFramesElapsed;
      if (state.mFramesElapsed == 10)
      {
        mState = Moving{};
      }
    },

    [&, this](Moving& state) {
      animationFrame = s.mpPerFrameState->mIsOddFrame ? 2 : 1;

      --mFramesUntilNextTurn;

      const auto result = engine::moveHorizontallyWithStairStepping(
        *d.mpCollisionChecker,
        entity,
        engine::orientation::toMovement(orientation));
      if (
        result != engine::MovementResult::Completed ||
        mFramesUntilNextTurn <= 0)
      {
        mState = Waiting{};
      }
    });
}

} // namespace rigel::game_logic::behaviors
