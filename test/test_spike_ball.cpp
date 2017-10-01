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

#include <catch.hpp>
#include "utils.hpp"

#include <base/spatial_types_printing.hpp>
#include <data/map.hpp>
#include <game_logic/ai/spike_ball.hpp>
#include <engine/base_components.hpp>
#include <engine/collision_checker.hpp>
#include <engine/physical_components.hpp>
#include <engine/physics_system.hpp>
#include <engine/timing.hpp>


using namespace rigel;
using namespace engine;
using namespace engine::components;
using namespace game_logic;


namespace ex = entityx;


TEST_CASE("Spike ball") {
  ex::EntityX entityx;
  auto spikeBall = entityx.entities.create();
  spikeBall.assign<Active>();
  spikeBall.assign<WorldPosition>(2, 20);
  spikeBall.assign<BoundingBox>(BoundingBox{{0, 0}, {3, 3}});
  ai::configureSpikeBall(spikeBall);

  data::map::Map map{300, 300, data::map::TileAttributes{{0x0, 0xF}}};

  // Floor
  for (int i = 0; i < 8; ++i) {
    map.setTileAt(0, i, 21, 1);
  }

  MockServiceProvider mockServiceProvicer;

  CollisionChecker collisionChecker{&map, entityx.entities, entityx.events};
  PhysicsSystem physicsSystem{&collisionChecker};
  ai::SpikeBallSystem spikeBallSystem{&collisionChecker, &mockServiceProvicer};

  physicsSystem.entityCollidedSignal().connect(
    [&spikeBallSystem](auto e, auto l, auto r, auto t, auto b) {
      spikeBallSystem.onEntityCollided(e, l, r, t, b);
    });

  auto& ballPosition = *spikeBall.component<WorldPosition>();

  auto runOneFrame = [&]() {
    spikeBallSystem.update(entityx.entities);
    physicsSystem.update(entityx.entities);
  };

  auto runFramesAndCollect = [&](std::size_t numFrames) {
    std::vector<base::Vector> positions;
    for (std::size_t i = 0; i < numFrames; ++i) {
      runOneFrame();
      positions.push_back(ballPosition);
    }
    return positions;
  };


  SECTION("Bouncing without obstacle") {
    const auto expectedPositions = std::vector<base::Vector>{
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
    const auto actualPositions = runFramesAndCollect(expectedPositions.size());

    CHECK(actualPositions == expectedPositions);
  }

  SECTION("Bouncing against ceiling") {
    // 15 ----------
    // 16
    // 17
    // 18  OOO
    // 19  OOO
    // 20  OOO
    // 21 ----------

    // Ceiling
    for (int i = 0; i < 8; ++i) {
      map.setTileAt(0, i, 15, 1);
    }

    const auto expectedPositions = std::vector<base::Vector>{
      {2, 18},
      {2, 18},
      {2, 19},
      {2, 20},
      {2, 18}
    };
    const auto actualPositions = runFramesAndCollect(expectedPositions.size());

    CHECK(actualPositions == expectedPositions);
  }
}
