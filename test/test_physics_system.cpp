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
#include <engine/collision_checker.hpp>
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

  CollisionChecker collisionChecker{&map, entityx.entities, entityx.events};
  PhysicsSystem physicsSystem{&collisionChecker};

  auto physicalObject = entities.create();
  physicalObject.assign<BoundingBox>(BoundingBox{{0, 0}, {2, 2}});
  physicalObject.assign<Physical>(Physical{{0.0f, 0.0f}, true});
  physicalObject.assign<WorldPosition>(WorldPosition{0, 4});
  physicalObject.assign<Active>();

  auto& physical = *physicalObject.component<Physical>();
  auto& position = *physicalObject.component<WorldPosition>();

  const auto runOneFrame = [&physicsSystem, &entityx]() {
    physicsSystem.update(entityx.entities);
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

    SECTION("Inactive objects's don't move") {
      physicalObject.remove<Active>();
      runOneFrame();
      CHECK(position.x == 0);
    }

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

    SECTION("Object continues falling after solidbody removed") {
      physical.mVelocity.y = 2.0f;
      runOneFrame();
      CHECK(position.y == 5);

      solidBody.destroy();
      physical.mVelocity.y = 2.0f;
      runOneFrame();
      CHECK(position.y == 7);
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

    SECTION("Left") {
      position.x = 5;
      position.y = 8;
      physical.mVelocity.x = -2.0f;
      physical.mGravityAffected = false;

      runOneFrame();
      CHECK(position.x == 4);
      CHECK(physicalObject.has_component<CollidedWithWorld>());

      runOneFrame();
      CHECK(position.x == 4);
    }

    SECTION("Right") {
      solidBody.component<WorldPosition>()->x = 3;
      position.x = 0;
      position.y = 8;
      physical.mVelocity.x = 2.0f;
      physical.mGravityAffected = false;

      runOneFrame();
      CHECK(position.x == 1);
      CHECK(physicalObject.has_component<CollidedWithWorld>());

      runOneFrame();
      CHECK(position.x == 1);
    }

    // TODO: Make stair-stepping work with solid bodies

    SECTION("SolidBody doesn't collide with itself") {
      solidBody.assign<Physical>(base::Point<float>{0, 2.0f}, false);
      solidBody.assign<Active>();
      runOneFrame();
      CHECK(solidBody.component<WorldPosition>()->y == 10);
    }
  }
}
