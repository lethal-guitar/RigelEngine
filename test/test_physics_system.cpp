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

#include <catch.hpp>

#include <base/warnings.hpp>

#include <data/map.hpp>
#include <engine/physical_components.hpp>
#include <engine/physics_system.hpp>
#include <engine/timing.hpp>


using namespace rigel;
using namespace engine;
using namespace engine::components;


namespace ex = entityx;


TEST_CASE("Physics system works as expected") {
  ex::EntityX entityx;
  auto& entities = entityx.entities;

  data::map::Map map{100, 100, data::map::TileAttributes{{0x0, 0xF}}};

  PhysicsSystem physicsSystem{&map};

  auto physicalObject = entities.create();
  physicalObject.assign<BoundingBox>(BoundingBox{{0, 0}, {2, 2}});
  physicalObject.assign<Physical>(Physical{{0.0f, 0.0f}, true});
  physicalObject.assign<WorldPosition>(WorldPosition{0, 4});

  auto& physical = *physicalObject.component<Physical>();
  auto& position = *physicalObject.component<WorldPosition>();

  const auto update = [&physicsSystem, &entityx](const TimeDelta dt) {
    physicsSystem.update(entityx.entities, entityx.events, dt);
  };
  const auto runOneFrame = [&update]() {
    update(gameFramesToTime(1));
  };


  SECTION("Objects move according to their velocity") {
    physical.mGravityAffected = false;

    SECTION("No movement when velocity is 0") {
      const auto previousPosition = position;

      physical.mVelocity.x = 0.0f;
      runOneFrame();
      CHECK(position == previousPosition);
    }

    physical.mVelocity.x = 4.0f;

    SECTION("Object's position changes") {
      SECTION("To the right") {
        runOneFrame();
        CHECK(position.x == 4);
      }

      position.x = 4;

      SECTION("To the left") {
        physical.mVelocity.x = -1.0f;
        runOneFrame();
        CHECK(position.x == 3);
      }

      physical.mVelocity.x = 0.0f;

      SECTION("Movement stops when setting velocity to 0") {
        runOneFrame();
        CHECK(position.x == 4);
      }

      SECTION("Upwards") {
        position.y = 10;
        physical.mVelocity.y = -2.0f;
        runOneFrame();
        CHECK(position.x == 4);
        CHECK(position.y == 8);
      }

      SECTION("Downwards") {
        position.y = 5;
        physical.mVelocity.y = 1.0f;
        runOneFrame();
        CHECK(position.x == 4);
        CHECK(position.y == 6);
      }
    }

    SECTION("Movement happens only if enough time has elapsed") {
      const auto timeForOneFrame = gameFramesToTime(1);

      update(0.0);
      CHECK(position.x == 0);

      update(timeForOneFrame / 2.0);
      CHECK(position.x == 0);

      update(timeForOneFrame / 2.0 + 0.0001);
      CHECK(position.x == 4);
    }
  }


  SECTION("Object's are pulled down by gravity") {
    position.x = 10;
    position.y = 5;

    SECTION("Non-moving object") {
      physical.mVelocity = base::Point<float>{0.0f, 0.0f};
      runOneFrame();
      CHECK(position.y > 5);
      CHECK(physical.mVelocity.y > 0.0f);

      SECTION("Falling speed increases until terminal velocity reached") {
        const auto lastPosition = position.y;
        const auto lastVelocity = physical.mVelocity.y;

        runOneFrame();
        CHECK(position.y > lastPosition);
        CHECK(physical.mVelocity.y > lastVelocity);

        for (int i = 0; i < 10; ++i) {
          runOneFrame();
        }

        // Yes, in the world of Duke Nukem II, 'terminal velocity' has a
        // value of 2...
        CHECK(physical.mVelocity.y <= 2.0f);
      }
    }

    SECTION("Moving object") {
      physical.mVelocity.x = 2;
      runOneFrame();
      CHECK(position.x == 12);

      CHECK(position.y > 5);
      CHECK(physical.mVelocity.y > 0.0f);
    }
  }


  SECTION("Physical objects collide with solid bodies") {
    auto solidBody = entities.create();
    solidBody.assign<BoundingBox>(BoundingBox{{0, 0}, {4, 3}});
    solidBody.assign<WorldPosition>(0, 8);
    solidBody.assign<SolidBody>();

    SECTION("Downard") {
      physical.mVelocity.y = 2.0f;

      runOneFrame();
      CHECK(physical.mVelocity.y == 0.0f);
      CHECK(position.y == 5);
      CHECK(physicalObject.has_component<CollidedWithWorld>());

      runOneFrame();
      CHECK(position.y == 5);
    }

    SECTION("Downward with offset") {
      physical.mVelocity.y = 2.0f;
      physical.mGravityAffected = true;
      physicalObject.component<BoundingBox>()->size = {3,5};
      position = {7, 88};

      solidBody.component<BoundingBox>()->topLeft.y = 3;
      solidBody.component<BoundingBox>()->size.height = 6;
      *solidBody.component<WorldPosition>() = {7,96};

      runOneFrame();
      CHECK(position.y == 90);
      CHECK(!physicalObject.has_component<CollidedWithWorld>());

      runOneFrame();
      CHECK(position.y == 92);
      CHECK(!physicalObject.has_component<CollidedWithWorld>());

      runOneFrame();
      CHECK(position.y == 93);
      CHECK(physical.mVelocity.y == 0.0f);
      CHECK(physicalObject.has_component<CollidedWithWorld>());
    }

    SECTION("Upward") {
      position.y = 11;
      physical.mVelocity.y = -2.0f;
      physical.mGravityAffected = false;

      runOneFrame();
      CHECK(physical.mVelocity.y == 0.0f);
      CHECK(position.y == 10);
      CHECK(physicalObject.has_component<CollidedWithWorld>());

      runOneFrame();
      CHECK(position.y == 10);
    }

    SECTION("Object continues falling after solidbody removed") {
      physical.mVelocity.y = 2.0f;
      runOneFrame();
      CHECK(position.y == 5);

      solidBody.destroy();
      physical.mVelocity.y = 2.0f;
      runOneFrame();
      CHECK(position.y == 7);
    }

    SECTION("SolidBody doesn't collide with itself") {
      solidBody.assign<Physical>(base::Point<float>{0, 2.0f}, false);
      runOneFrame();
      CHECK(solidBody.component<WorldPosition>()->y == 10);
    }
  }
}
