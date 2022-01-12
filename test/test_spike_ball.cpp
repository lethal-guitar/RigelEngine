/* Copyright (C) 2017, Nikolai Wuttke. All rights reserved.
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

#include "utils.hpp"

#include <base/spatial_types_printing.hpp>
#include <data/game_options.hpp>
#include <data/game_traits.hpp>
#include <data/map.hpp>
#include <data/player_model.hpp>
#include <engine/base_components.hpp>
#include <engine/collision_checker.hpp>
#include <engine/entity_activation_system.hpp>
#include <engine/particle_system.hpp>
#include <engine/physical_components.hpp>
#include <engine/physics_system.hpp>
#include <engine/random_number_generator.hpp>
#include <engine/timing.hpp>
#include <game_logic/behavior_controller_system.hpp>
#include <game_logic/enemies/spike_ball.hpp>
#include <game_logic/entity_factory.hpp>
#include <game_logic/player.hpp>
#include <game_logic/player/components.hpp>

RIGEL_DISABLE_WARNINGS
#include <catch.hpp>
RIGEL_RESTORE_WARNINGS


using namespace rigel;
using namespace engine;
using namespace engine::components;
using namespace game_logic;


namespace ex = entityx;


struct MockSpriteFactory : public rigel::engine::ISpriteFactory
{
  engine::components::Sprite createSprite(data::ActorID id) override
  {
    static rigel::engine::SpriteDrawData dummyDrawData;
    return {&dummyDrawData, {}};
  }

  base::Rect<int> actorFrameRect(data::ActorID id, int frame) const override
  {
    // Bounds for spike ball
    return {{}, {3, 3}};
  }

  SpriteFrame actorFrameData(data::ActorID id, int frame) const override
  {
    return {};
  }
};


TEST_CASE("Spike ball")
{
  ex::EntityX entityx;

  data::map::Map map{300, 300, data::map::TileAttributeDict{{0x0, 0xF}}};

  // Floor
  for (int i = 0; i < 8; ++i)
  {
    map.setTileAt(0, i, 21, 1);
  }

  MockServiceProvider mockServiceProvider;
  engine::RandomNumberGenerator randomGenerator;
  MockSpriteFactory mockSpriteFactory;
  data::GameOptions options;
  EntityFactory entityFactory{
    &mockSpriteFactory,
    &entityx.entities,
    &mockServiceProvider,
    &randomGenerator,
    &options,
    data::Difficulty::Medium};

  auto spikeBall =
    entityFactory.spawnActor(data::ActorID::Bouncing_spike_ball, {2, 20});

  CollisionChecker collisionChecker{&map, entityx.entities, entityx.events};
  PhysicsSystem physicsSystem{&collisionChecker, &map, &entityx.events};

  auto playerEntity = entityx.entities.create();
  playerEntity.assign<WorldPosition>(6, 100);
  playerEntity.assign<Sprite>();
  assignPlayerComponents(playerEntity, Orientation::Left);

  data::PlayerModel playerModel;
  Player player(
    playerEntity,
    data::Difficulty::Medium,
    &playerModel,
    &mockServiceProvider,
    &options,
    &collisionChecker,
    &map,
    &entityFactory,
    &entityx.events,
    &randomGenerator);

  base::Vec2 cameraPosition{0, 0};
  engine::ParticleSystem particleSystem{&randomGenerator, nullptr};
  BehaviorControllerSystem behaviorControllerSystem{
    GlobalDependencies{
      &collisionChecker,
      &particleSystem,
      &randomGenerator,
      &entityFactory,
      &mockServiceProvider,
      &entityx.entities,
      &entityx.events},
    &player,
    &cameraPosition,
    &map};

  auto& ballPosition = *spikeBall.component<WorldPosition>();

  const auto& viewportSize = data::GameTraits::mapViewportSize;
  PerFrameState perFrameState;
  perFrameState.mCurrentViewportSize = viewportSize;

  auto runOneFrame = [&]() {
    engine::markActiveEntities(entityx.entities, {0, 0}, viewportSize);
    behaviorControllerSystem.update(entityx.entities, perFrameState);
    physicsSystem.update(entityx.entities);
    perFrameState.mIsOddFrame = !perFrameState.mIsOddFrame;
  };

  auto runFramesAndCollect = [&](std::size_t numFrames) {
    std::vector<base::Vec2> positions;
    for (std::size_t i = 0; i < numFrames; ++i)
    {
      runOneFrame();
      positions.push_back(ballPosition);
    }
    return positions;
  };


  SECTION("Bouncing without obstacle")
  {
    // clang-format off
    const auto expectedPositions = std::vector<base::Vec2>{
      {2, 18},
      {2, 16},
      {2, 15},
      {2, 14},
      {2, 13},
      {2, 13},
      {2, 14},
      {2, 15},
      {2, 17},
      {2, 19},
      {2, 20},

      {2, 18},
      {2, 16},
      {2, 15},
      {2, 14},
      {2, 13},
      {2, 13},
      {2, 14},
      {2, 15},
      {2, 17},
      {2, 19}
    };
    // clang-format on

    const auto actualPositions = runFramesAndCollect(expectedPositions.size());

    CHECK(actualPositions == expectedPositions);
  }

  SECTION("Bouncing against ceiling")
  {
    // 15 ----------
    // 16
    // 17
    // 18  OOO
    // 19  OOO
    // 20  OOO
    // 21 ----------

    // Ceiling
    for (int i = 0; i < 8; ++i)
    {
      map.setTileAt(0, i, 15, 1);
    }

    // clang-format off
    const auto expectedPositions = std::vector<base::Vec2>{
      {2, 18},
      {2, 18},
      {2, 18},
      {2, 19},
      {2, 20},
      {2, 18}
    };
    // clang-format on

    const auto actualPositions = runFramesAndCollect(expectedPositions.size());

    CHECK(actualPositions == expectedPositions);
  }
}
