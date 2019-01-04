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

#include "utils.hpp"

#include <base/array_view.hpp>
#include <base/spatial_types_printing.hpp>
#include <base/warnings.hpp>
#include <data/map.hpp>
#include <data/player_model.hpp>
#include <data/sound_ids.hpp>
#include <engine/base_components.hpp>
#include <engine/collision_checker.hpp>
#include <engine/sprite_tools.hpp>
#include <engine/visual_components.hpp>
#include <game_logic/ientity_factory.hpp>
#include <game_logic/player/components.hpp>
#include <game_logic/player.hpp>
#include <utils/container_tools.hpp>

#include <global_level_events.hpp>

RIGEL_DISABLE_WARNINGS
#include <catch.hpp>
RIGEL_RESTORE_WARNINGS


using namespace std;

using namespace rigel;
using namespace game_logic;

using engine::CollisionChecker;
using engine::components::BoundingBox;
using engine::components::Orientation;
using engine::components::Sprite;
using engine::components::WorldPosition;

namespace ex = entityx;


namespace rigel { namespace data {

static std::ostream& operator<<(std::ostream& os, const SoundId id) {
  os << static_cast<int>(id);
  return os;
}

}}


namespace {

struct MockEventListener : public ex::Receiver<MockEventListener> {
  int mCallCount = 0;

  void receive(const rigel::events::PlayerDied&) {
    ++mCallCount;
  }
};


PlayerInput operator&(
  const PlayerInput& lhs,
  const PlayerInput& rhs
) {
  PlayerInput merged;
  merged.mLeft = lhs.mLeft || rhs.mLeft;
  merged.mRight = lhs.mRight || rhs.mRight;
  merged.mUp = lhs.mUp || rhs.mUp;
  merged.mDown = lhs.mDown || rhs.mDown;

  merged.mJump.mWasTriggered =
    lhs.mJump.mWasTriggered || rhs.mJump.mWasTriggered;
  merged.mFire.mWasTriggered =
    lhs.mFire.mWasTriggered || rhs.mFire.mWasTriggered;
  merged.mJump.mIsPressed =
    lhs.mJump.mIsPressed || rhs.mJump.mIsPressed;
  merged.mFire.mIsPressed =
    lhs.mFire.mIsPressed || rhs.mFire.mIsPressed;
  return merged;
}


void makeWall(data::map::Map& map, const int x, const int yStart, const int yEnd) {
  for (int y = yStart; y <= yEnd; ++y) {
    map.setTileAt(0, x, y, 1);
  }
}


void makeFloor(data::map::Map& map, const int y, const int xStart, const int xEnd) {
  for (int x = xStart; x <= xEnd; ++x) {
    map.setTileAt(0, x, y, 1);
  }
}


struct StateChange {
  base::Vector move;
  int frame;

  bool operator==(const StateChange& other) const {
    return std::tie(move, frame) == std::tie(other.move, other.frame);
  }
};


std::ostream& operator<<(std::ostream& stream, const StateChange& change) {
  stream << "[Move: " << change.move << ", anim: " << change.frame << ']';
  return stream;
}


struct MoveSpec {
  MoveSpec(
    const PlayerInput input,
    const base::Vector expectedMove,
    const int expectedAnimationFrame
  )
    : givenInput(input)
    , expectedStateChange({expectedMove, expectedAnimationFrame})
  {
  }

  PlayerInput givenInput;
  StateChange expectedStateChange;
};


void testMovementSequence(Player& player, const std::vector<MoveSpec>& spec) {
  std::vector<StateChange> actualStateChanges;

  auto previousPosition = player.position();
  for (const auto& frame : spec) {
    player.update(frame.givenInput);

    actualStateChanges.push_back(StateChange{
      player.position() - previousPosition,
      player.animationFrame()});

    previousPosition = player.position();
  }

  const auto expectedStateChanges = utils::transformed(spec,
    [](const MoveSpec& frameSpec) {
      return frameSpec.expectedStateChange;
    });
  CHECK(actualStateChanges == expectedStateChanges);
}


struct FireShotParameters {
  ProjectileType type;
  WorldPosition position;
  ProjectileDirection direction;
};


struct MockEntityFactory : public IEntityFactory {
  std::vector<FireShotParameters> mCreateProjectileCalls;

  explicit MockEntityFactory(ex::EntityManager* pEntityManager)
    : mpEntityManager(pEntityManager)
  {
  }

  entityx::Entity createProjectile(
    ProjectileType type,
    const WorldPosition& pos,
    ProjectileDirection direction
  ) override {
    mCreateProjectileCalls.push_back(FireShotParameters{type, pos, direction});
    return createMockSpriteEntity();
  }

  entityx::Entity createEntitiesForLevel(
    const data::map::ActorDescriptionList& actors
  ) override {
    return {};
  }

  entityx::Entity createSprite(
    data::ActorID actorID,
    bool assignBoundingBox = false
  ) override {
    return createMockSpriteEntity();
  }

  entityx::Entity createSprite(
    data::ActorID actorID,
    const base::Vector& position,
    bool assignBoundingBox = false
  ) override {
    return createMockSpriteEntity();
  }

  entityx::Entity createActor(
    data::ActorID actorID,
    const base::Vector& position
  ) override {
    return createMockSpriteEntity();
  }

private:
  entityx::Entity createMockSpriteEntity() {
    static rigel::engine::SpriteDrawData dummyDrawData;

    auto entity = mpEntityManager->create();
    Sprite sprite{&dummyDrawData, {}};
    entity.assign<Sprite>(sprite);
    return entity;
  }

  ex::EntityManager* mpEntityManager;
};

}


TEST_CASE("Player movement") {
  ex::EntityX entityx;

  // ---------------------------------------------------------------------------
  // Map
  data::map::Map map{100, 100, data::map::TileAttributes{{
    0x0,    // index 0: empty
    0xF,    // index 1: solid
    0x4000, // index 2: ladder
    0x80    // index 3: climbable
  }}};

  makeFloor(map, 17, 0, 32);
  const auto initialMap = map;

  // ---------------------------------------------------------------------------
  // Player dependencies

  CollisionChecker collisionChecker{&map, entityx.entities, entityx.events};

  data::PlayerModel playerModel;
  MockEntityFactory mockEntityFactory{&entityx.entities};
  MockServiceProvider mockServiceProvider;

  // ---------------------------------------------------------------------------
  // Player entity

  auto playerEntity = entityx.entities.create();
  playerEntity.assign<WorldPosition>(8, 16);
  playerEntity.assign<Sprite>();
  assignPlayerComponents(playerEntity, Orientation::Left);

  Player player(
    playerEntity,
    data::Difficulty::Medium,
    &playerModel,
    &mockServiceProvider,
    &collisionChecker,
    &map,
    &mockEntityFactory,
    &entityx.events);

  auto resetOrientation = [&](const Orientation newOrientation) {
    *playerEntity.component<Orientation>() = newOrientation;
  };


  auto drainMercyFrames = [&]() {
    while (player.isInMercyFrames()) {
      player.update({});
    }
  };

  auto& position = *playerEntity.component<WorldPosition>();
  auto& animationFrame = playerEntity.component<Sprite>()->mFramesToRender[0];
  auto& bbox = *playerEntity.component<BoundingBox>();

  PlayerInput pressingLeft;
  pressingLeft.mLeft = true;

  PlayerInput pressingRight;
  pressingRight.mRight = true;

  PlayerInput pressingUp;
  pressingUp.mUp = true;

  PlayerInput pressingDown;
  pressingDown.mDown = true;

  PlayerInput pressingJump;
  pressingJump.mJump.mIsPressed = true;

  PlayerInput pressingFire;
  pressingFire.mFire.mIsPressed = true;

  PlayerInput jumpButtonTriggered;
  jumpButtonTriggered.mJump.mWasTriggered = true;

  PlayerInput fireButtonTriggered;
  fireButtonTriggered.mFire.mWasTriggered = true;

  // ---------------------------------------------------------------------------

  SECTION("Facing left") {
    SECTION("Doesn't move when no key pressed") {
      const auto previousPosition = position;

      player.update(PlayerInput{});

      CHECK(position == previousPosition);
    }

    SECTION("Doesn't move when both keys pressed") {
      const auto previousPosition = position;

      PlayerInput input;
      input.mLeft = true;
      input.mRight = true;
      player.update(input);

      CHECK(position == previousPosition);
    }

    SECTION("Moves left when left key pressed") {
      const auto expectedPosition = position + base::Vector{-1, 0};

      PlayerInput input;
      input.mLeft = true;
      player.update(input);

      CHECK(position == expectedPosition);

      SECTION("Stops moving when key released") {
        input.mLeft = false;
        player.update(input);

        CHECK(position == expectedPosition);
      }
    }

    SECTION("Changes orientation when right key pressed") {
      const auto expectedPosition = position;

      PlayerInput input;
      input.mRight = true;
      player.update(input);

      CHECK(position == expectedPosition);
      CHECK(player.orientation() == Orientation::Right);
      CHECK(animationFrame == 0);
    }

    SECTION("Doesn't move when up against wall") {
      const auto previousPosition = position;
      makeWall(map, position.x - 1, 0, position.y + 1);

      PlayerInput input;
      input.mLeft = true;
      player.update(input);

      CHECK(position == previousPosition);
    }

    SECTION("Doesn't move when up key pressed at the same time") {
      const auto previousPosition = position;

      PlayerInput input;
      input.mUp = true;
      input.mLeft = true;
      player.update(input);

      CHECK(position == previousPosition);
    }

    SECTION("Doesn't move when down key pressed at the same time") {
      const auto previousPosition = position;

      PlayerInput input;
      input.mDown = true;
      input.mLeft = true;
      player.update(input);

      CHECK(position == previousPosition);
    }

    SECTION("Ignores up/down keys when both pressed at the same time") {
      const auto expectedPosition = position + base::Vector{-1, 0};

      PlayerInput input;
      input.mLeft = true;
      input.mUp = true;
      input.mDown = true;
      player.update(input);

      CHECK(position == expectedPosition);
    }

    SECTION("Aims up when up key pressed") {
      PlayerInput input;
      input.mUp = true;

      player.update(input);

      CHECK(animationFrame == 16);
      CHECK(player.isLookingUp());

      SECTION("isLookingUp() works correctly when recoil animation shown") {
        animationFrame = 19;
        CHECK(player.isLookingUp());
      }

      SECTION("Can change orientation while looking up") {
        const auto previousOrientation = player.orientation();

        player.update(pressingUp & pressingRight);

        CHECK(player.isLookingUp());
        CHECK(player.orientation() != previousOrientation);
      }

      SECTION("Stops aiming up when key released") {
        input.mUp = false;
        player.update(input);

        CHECK(animationFrame == 0);
        CHECK(!player.isLookingUp());
      }
    }

    SECTION("Crouches when down key pressed") {
      PlayerInput input;
      input.mDown = true;
      player.update(input);

      CHECK(animationFrame == 17);
      CHECK(player.isCrouching());
      CHECK(
        player.worldSpaceHitBox().size.height == PLAYER_HITBOX_HEIGHT_CROUCHED);
      CHECK(bbox.size.height == PLAYER_HEIGHT_CROUCHED);

      SECTION("isCrouching() works correctly when recoil animation shown") {
        animationFrame = 34;
        CHECK(player.isCrouching());
      }

      SECTION("Can change orientation while crouching") {
        const auto previousOrientation = player.orientation();

        player.update(pressingDown & pressingRight);

        CHECK(player.isCrouching());
        CHECK(player.orientation() != previousOrientation);
      }

      SECTION("Stops crouching when key released") {
        input.mDown = false;
        player.update(input);

        CHECK(animationFrame == 0);
        CHECK(!player.isCrouching());
        CHECK(bbox.size.height == PLAYER_HEIGHT);
      }
    }

    SECTION("Walks up a stair step") {
      map.setTileAt(0, position.x - 1, position.y, 1);
      const auto expectedPosition = position + base::Vector{-1, -1};

      PlayerInput input;
      input.mLeft = true;
      player.update(input);

      CHECK(position == expectedPosition);
    }

    SECTION("Falling") {
      // Make a hole in the floor
      map.setTileAt(0, position.x + 1, position.y + 1, 0);
      map.setTileAt(0, position.x + 0, position.y + 1, 0);
      map.setTileAt(0, position.x - 1, position.y + 1, 0);
      map.setTileAt(0, position.x - 2, position.y + 1, 0);
      map.setTileAt(0, position.x - 3, position.y + 1, 0);

      // New floor, further down
      makeFloor(map, 24, 0, 32);

      SECTION("Falls down when walking off ledge") {
        testMovementSequence(player, {
          {pressingLeft, {-1, +1}, 7},
          {{}, {0, 1}, 7},
          {{}, {0, 1}, 7},
          {{}, {0, 2}, 8},
          {{}, {0, 2}, 8}, // landing here
          {{}, {0, 0}, 5},
          {{}, {0, 0}, 0},
        });
      }

      SECTION("Falls down when ground disappears") {
        map.setTileAt(0, position.x + 2, position.y + 1, 0);

        testMovementSequence(player, {
          {{}, {0, 1}, 7},
          {{}, {0, 1}, 7},
          {{}, {0, 1}, 7},
          {{}, {0, 2}, 8},
          {{}, {0, 2}, 8}, // landing here
          {{}, {0, 0}, 5},
          {{}, {0, 0}, 0},
        });
      }

      SECTION("Has one recovery frame when falling at full speed") {
        map.setTileAt(0, position.x + 2, position.y + 1, 0);

        testMovementSequence(player, {
          {{}, {0, 1}, 7},
          {{}, {0, 1}, 7},
          {{}, {0, 1}, 7},
          {{}, {0, 2}, 8},
          {{}, {0, 2}, 8}, // landing here
          {{}, {0, 0}, 5},
          {pressingLeft, {0, 0}, 0}, // recovery frame - movement ignored here
          {pressingLeft, {-1, 0}, 1}, // now moving again
        });
      }

      SECTION("No recovery frame when landing before reaching full speed") {
        makeFloor(map, 19, 0, 32);
        map.setTileAt(0, position.x + 2, position.y + 1, 0);

        testMovementSequence(player, {
          {{}, {0, 1}, 7},
          {{}, {0, 1}, 7}, // landing here
          {{}, {0, 0}, 0},
          {pressingLeft, {-1, 0}, 1},
          {pressingLeft, {-1, 0}, 1},
        });
      }

      SECTION("Horizontal movement is possible while falling (air control)") {
        map.setTileAt(0, position.x + 2, position.y + 1, 0);

        testMovementSequence(player, {
          {pressingLeft, {-1, 1}, 7},
          {pressingLeft, {-1, 1}, 7},
          {pressingRight, {0, 1}, 7}, // changing orientation here
          {pressingRight, {1, 2}, 8},
          {pressingRight, {1, 2}, 8}, // landing here
          {pressingRight, {1, 0}, 5},
          {pressingRight, {0, 0}, 0}, // recovery frame - movement ignored here
          {pressingRight, {1, 0}, 1}, // now moving again
        });
      }
    }

    SECTION("Jumping") {
      SECTION("Doesn't jump when already touching ceiling") {
        makeFloor(map, position.y - PLAYER_HEIGHT, 0, 32);

        testMovementSequence(player, {
          {jumpButtonTriggered, {0, 0}, 0},
          {{}, {0, 0}, 0},
        });
      }

      SECTION("Low jump") {
        testMovementSequence(player, {
          {jumpButtonTriggered, {0, 0}, 5},
          {{}, {0, -2}, 6},
          {{}, {0, -2}, 6},
          {{}, {0, -1}, 6},
          {{}, {0, 0}, 6},
          {{}, {0, 0}, 6},
          {{}, {0, 1}, 7}, // falling again
          {{}, {0, 1}, 7},
          {{}, {0, 1}, 7},
          {{}, {0, 2}, 8},
          {{}, {0, 0}, 5},
          {{}, {0, 0}, 0},
        });
      }

      SECTION("High jump") {
        testMovementSequence(player, {
          {jumpButtonTriggered, {0, 0}, 5},
          {{}, {0, -2}, 6},
          {{}, {0, -2}, 6},
          {pressingJump, {0, -1}, 6},
          {{}, {0, -1}, 6},
          {{}, {0, -1}, 6},
          {{}, {0, 0}, 6},
          {{}, {0, 0}, 6},
          {{}, {0, 0}, 6},
          {{}, {0, 1}, 7}, // falling again
          {{}, {0, 1}, 7},
          {{}, {0, 1}, 7},
          {{}, {0, 2}, 8},
          {{}, {0, 2}, 8},
          {{}, {0, 0}, 5},
          {{}, {0, 0}, 0},
        });
      }

      SECTION("Collision after 1 step") {
        makeFloor(map, position.y - (PLAYER_HEIGHT + 1), 0, 32);

        testMovementSequence(player, {
          {jumpButtonTriggered, {0, 0}, 5},
          {{}, {0, -1}, 6},
          {{}, {0, 1}, 7},
          {{}, {0, 0}, 0},
        });
      }

      SECTION("Collision after 2 steps") {
        makeFloor(map, position.y - (PLAYER_HEIGHT + 2), 0, 32);

        testMovementSequence(player, {
          {jumpButtonTriggered, {0, 0}, 5},
          {{}, {0, -2}, 6},
          {{}, {0, 1}, 7},
          {{}, {0, 1}, 7},
          {{}, {0, 0}, 0},
        });
      }

      SECTION("Collision after 3 steps") {
        makeFloor(map, position.y - (PLAYER_HEIGHT + 3), 0, 32);

        testMovementSequence(player, {
          {jumpButtonTriggered, {0, 0}, 5},
          {{}, {0, -2}, 6},
          {{}, {0, -1}, 6},
          {{}, {0, 1}, 7},
          {{}, {0, 1}, 7},
          {{}, {0, 1}, 7},
          {{}, {0, 0}, 5},
          {{}, {0, 0}, 0},
        });
      }

      SECTION("Collision after 4 steps") {
        makeFloor(map, position.y - (PLAYER_HEIGHT + 4), 0, 32);

        testMovementSequence(player, {
          {jumpButtonTriggered, {0, 0}, 5},
          {{}, {0, -2}, 6},
          {{}, {0, -2}, 6},
          {{}, {0, 1}, 7},
          {{}, {0, 1}, 7},
          {{}, {0, 1}, 7},
          {{}, {0, 1}, 5},
          {{}, {0, 0}, 0},
        });
      }

      SECTION("Doesn't fall down immediately when collision at apex of low jump") {
        makeFloor(map, position.y - (PLAYER_HEIGHT + 5), 0, 32);

        testMovementSequence(player, {
          {jumpButtonTriggered, {0, 0}, 5},
          {{}, {0, -2}, 6},
          {{}, {0, -2}, 6},
          {{}, {0, -1}, 6},
          {{}, {0, 0}, 6},
          {{}, {0, 0}, 6},
          {{}, {0, 1}, 7}, // falling again
          {{}, {0, 1}, 7},
          {{}, {0, 1}, 7},
          {{}, {0, 2}, 8},
          {{}, {0, 0}, 5},
          {{}, {0, 0}, 0},
        });
      }

      SECTION("Doesn't fall down immediately when collision at apex of high jump") {
        makeFloor(map, position.y - (PLAYER_HEIGHT + 7), 0, 32);

        testMovementSequence(player, {
          {jumpButtonTriggered, {0, 0}, 5},
          {{}, {0, -2}, 6},
          {{}, {0, -2}, 6},
          {pressingJump, {0, -1}, 6},
          {{}, {0, -1}, 6},
          {{}, {0, -1}, 6},
          {{}, {0, 0}, 6},
          {{}, {0, 0}, 6},
          {{}, {0, 0}, 6},
          {{}, {0, 1}, 7}, // falling again
          {{}, {0, 1}, 7},
          {{}, {0, 1}, 7},
          {{}, {0, 2}, 8},
          {{}, {0, 2}, 8},
          {{}, {0, 0}, 5},
          {{}, {0, 0}, 0},
        });
      }

      SECTION("Can move horizontally while jumping") {
        testMovementSequence(player, {
          {jumpButtonTriggered & pressingLeft, {-1, 0}, 5},
          {pressingLeft, {0, -2}, 6}, // no movement on the first frame of jumping
          {pressingLeft, {-1, -2}, 6},
          {pressingLeft, {-1, -1}, 6},
          {pressingRight, {0, 0}, 6}, // change orientation
          {pressingRight, {1, 0}, 6},
          {pressingRight, {1, 1}, 7}, // falling again
          {pressingRight, {1, 1}, 7},
          {pressingRight, {1, 1}, 7},
          {pressingRight, {1, 2}, 8},
          {pressingRight, {1, 0}, 5},
          {pressingRight, {0, 0}, 0}, // recovery frame
        });
      }

      SECTION("Obstacle disappears after shortening jump") {
        makeFloor(map, position.y - (PLAYER_HEIGHT + 1), 0, 32);

        testMovementSequence(player, {
          {jumpButtonTriggered, {0, 0}, 5},
          {{}, {0, -1}, 6},
        });

        map = initialMap;

        testMovementSequence(player, {
          {{}, {0, -1}, 6},
          {{}, {0, 0}, 6},
          {{}, {0, 0}, 6},
          {{}, {0, 0}, 6},
          {{}, {0, 1}, 7}, // falling again
          {{}, {0, 1}, 7},
          {{}, {0, 0}, 0},
        });
      }

      SECTION("Can still jump on the frame where player walked off a ledge") {
        // Make a hole in the floor to the player's left
        for (int x = 0; x <= position.x + 1; ++x) {
          map.setTileAt(0, x, position.y + 1, 0);
        }

        CHECK(
          collisionChecker.isOnSolidGround(player.worldSpaceCollisionBox()));

        const auto expectedPosition = position - base::Vector{1, 0};
        player.update(pressingLeft & jumpButtonTriggered);

        CHECK(
          !collisionChecker.isOnSolidGround(player.worldSpaceCollisionBox()));
        CHECK(position == expectedPosition);
        CHECK(animationFrame == 5);

        player.update({});

        CHECK(animationFrame == 6);
      }

      SECTION("Lands on floor when already touching floor at apex of jump") {
        makeFloor(map, position.y - 4, 0, position.x-1);

        testMovementSequence(player, {
          {jumpButtonTriggered, {0, 0}, 5},
          {{}, {0, -2}, 6},
          {{}, {0, -2}, 6},
          {{}, {0, -1}, 6},
          {{pressingLeft}, {-1, 0}, 6},
          {{}, {0, 0}, 6},
          {{}, {0, 0}, 0}
        });
      }

      // TODO: Edge case: jump was shortened but ceiling is only one tile high
    }

    SECTION("Climbing ladders") {
      auto animationFrameValid = [&]() {
        return animationFrame == 35 || animationFrame == 36;
      };

      SECTION("Doesn't attach when ladder at non-attacheable height") {
        for (int yOffset = 0; yOffset < 4; ++yOffset) {
          map.setTileAt(0, position.x + 1, position.y - yOffset, 2);

          const auto previousPosition = position;

          player.update(pressingUp);

          CHECK(animationFrame != 35);
          CHECK(position == previousPosition);

          map.setTileAt(0, position.x + 1, position.y - yOffset, 0);
        }
      }

      SECTION("Doesn't attach when one unit above player") {
        map.setTileAt(0, position.x + 1, position.y - 5, 2);

        const auto previousPosition = position;

        player.update(pressingUp);

        CHECK(animationFrame != 35);
        CHECK(position == previousPosition);
      }

      SECTION("Attaches when touching ladder at right height") {
        map.setTileAt(0, position.x + 1, position.y - 4, 2);

        const auto previousPosition = position;

        player.update(pressingUp);

        CHECK(animationFrame == 35);
        CHECK(position == previousPosition);

        SECTION("Can't move horizontally while on ladder") {
          SECTION("Left") {
            player.update(pressingLeft);

            CHECK(animationFrame == 35);
            CHECK(position == previousPosition);
          }

          SECTION("Right") {
            player.update(pressingRight);

            CHECK(animationFrame == 35);
            CHECK(position == previousPosition);
          }
        }

        SECTION("Can move up on ladder") {
          map.setTileAt(0, position.x + 1, position.y - 5, 2);

          const auto expectedPosition = position + base::Vector{0, -1};
          player.update(pressingUp);

          CHECK(animationFrameValid());
          CHECK(position == expectedPosition);

          SECTION("Can't move up when end of ladder reached") {
            player.update(pressingUp);

            CHECK(animationFrameValid());
            CHECK(position == expectedPosition);
          }

          SECTION("Changing orientation changes to the proper animation frame") {
            CHECK(player.orientation() == Orientation::Left);
            CHECK(animationFrame == 36);

            player.update(pressingRight);

            CHECK(animationFrame == 36);
          }
        }

        SECTION("Can move down on ladder") {
          for (int offset = 0; offset < 5; ++offset) {
            map.setTileAt(0, position.x + 1, position.y - (5 + offset), 2);
          }

          position.y -= 5;

          const auto expectedPosition = position + base::Vector{0, 1};

          player.update(pressingDown);

          CHECK(animationFrameValid());
          CHECK(position == expectedPosition);

          SECTION("Falls off ladder when climbing past bottom rung") {
            testMovementSequence(player, {
              {pressingDown, {0, 1}, 7},
              {{}, {0, 1}, 7},
              {{}, {0, 1}, 7},
              {{}, {0, 1}, 5},
              {{}, {0, 0}, 0},
            });
          }

          SECTION("Can jump off ladder") {
            testMovementSequence(player, {
              {jumpButtonTriggered, {0, -2}, 6},
              {{}, {0, -2}, 6},
              {{}, {0, -1}, 6},
              {{}, {0, 0}, 6},
              {{}, {0, 0}, 6},
              {{}, {0, 1}, 7}, // falling again
              {{}, {0, 1}, 7},
              {{}, {0, 1}, 7},
              {{}, {0, 2}, 8},
              {{}, {0, 2}, 8},
              {{}, {0, 2}, 8},
              {{}, {0, 0}, 5},
              {{}, {0, 0}, 0},
            });
          }

          SECTION("Can immediately move horizontally when jumping off ladder") {
            testMovementSequence(player, {
              {jumpButtonTriggered & pressingLeft, {-1, -2}, 6},
              {pressingLeft, {-1, -2}, 6},
              {pressingLeft, {-1, -1}, 6},
              {{}, {0, 0}, 6},
              {{}, {0, 0}, 6},
              {{}, {0, 1}, 7}, // falling again
              {{}, {0, 1}, 7},
              {{}, {0, 1}, 7},
              {{}, {0, 2}, 8},
              {{}, {0, 2}, 8},
              {{}, {0, 2}, 8},
              {{}, {0, 0}, 5},
              {{}, {0, 0}, 0},
            });
          }

          SECTION("Doesn't re-attach immediately when pressing up while jumping") {
            testMovementSequence(player, {
              {jumpButtonTriggered & pressingUp, {0, -2}, 6},
              {{}, {0, -2}, 6},
            });
          }
        }
      }

      SECTION("Attaches and snaps player to ladder when player off to the left by one") {
        map.setTileAt(0, position.x, position.y - 4, 2);

        const auto expectedPosition = position + base::Vector{-1, 0};

        player.update(pressingUp);

        CHECK(animationFrame == 35);
        CHECK(position == expectedPosition);
      }

      SECTION("Attaches and snaps player to ladder when player off to the right by one") {
        map.setTileAt(0, position.x + 2, position.y - 4, 2);

        const auto expectedPosition = position + base::Vector{1, 0};

        player.update(pressingUp);

        CHECK(animationFrame == 35);
        CHECK(position == expectedPosition);
      }

      SECTION("Moves up by one when attaching and ladder tile above") {
        map.setTileAt(0, position.x + 1, position.y - 5, 2);
        map.setTileAt(0, position.x + 1, position.y - 4, 2);

        const auto expectedPositionAfterAttach = position + base::Vector{0, -1};

        player.update(pressingUp);

        CHECK(animationFrameValid());
        CHECK(position == expectedPositionAfterAttach);
      }

      SECTION("Can attach while falling") {
        map.setTileAt(0, position.x + 1, position.y - 7, 2);
        position.y -= 6;

        testMovementSequence(player, {
          {pressingUp, {0, 1}, 7},
          {pressingUp, {0, 1}, 7},
          {pressingUp, {0, 1}, 7},
          {pressingUp, {0, 0}, 35},
        });
      }

      position.y = 16;
      for (int yOffset = 4; yOffset < 10; ++yOffset) {
        map.setTileAt(0, position.x + 1, position.y - yOffset, 2);
      }

      SECTION("Can attach while jumping when on frame 4 (low jump)") {
        testMovementSequence(player, {
          {jumpButtonTriggered, {0, 0}, 5},
          {pressingUp, {0, -2}, 6},
          {pressingUp, {0, -2}, 6},
          {pressingUp, {0, -1}, 6},
          {pressingUp, {0, 0}, 35},
        });
      }

      SECTION("Can attach while jumping when beyond frame 4 (low jump)") {
        testMovementSequence(player, {
          {jumpButtonTriggered, {0, 0}, 5},
          {pressingUp, {0, -2}, 6},
          {pressingUp, {0, -2}, 6},
          {pressingUp, {0, -1}, 6},
          {{}, {0, 0}, 6},
          {pressingUp, {0, 0}, 35},
        });
      }

      SECTION("Can attach while jumping when on frame 4 (high jump)") {
        testMovementSequence(player, {
          {jumpButtonTriggered, {0, 0}, 5},
          {pressingUp, {0, -2}, 6},
          {pressingUp, {0, -2}, 6},
          {pressingUp & pressingJump, {0, -1}, 6},
          {pressingUp, {0, 0}, 35},
        });
      }

      SECTION("Can attach while jumping when beyond frame 4 (high jump)") {
        map.setTileAt(0, position.x + 1, position.y - 10, 2);

        testMovementSequence(player, {
          {jumpButtonTriggered, {0, 0}, 5},
          {pressingUp, {0, -2}, 6},
          {pressingUp, {0, -2}, 6},
          {pressingUp & pressingJump, {0, -1}, 6},
          {{}, {0, -1}, 6},
          {pressingUp, {0, 0}, 35},
        });
      }
    }

    SECTION("Climbing on pipes/climbables") {
      const auto pipeLength = 8;
      const auto pipeStartX = position.x - 4;
      const auto pipeEndX = pipeStartX + pipeLength - 1;
      const auto pipeY = position.y - 6;
      for (int i = 0; i < pipeLength; ++i) {
        map.setTileAt(0, pipeStartX + i, pipeY, 3);
      }

      SECTION("Attaches while falling when player top row touches climbable") {
        const auto originalY = position.y;
        position.y -= 3;

        player.update({});
        CHECK(animationFrame == 7);

        player.update({});
        CHECK(animationFrame == 20);
        CHECK(position.y == originalY - 1);

        SECTION("Doesn't fall down when attached to pipe") {
          player.update({});

          CHECK(animationFrame == 20);
          CHECK(position.y == originalY - 1);
        }

        SECTION("Movement on pipe") {
          SECTION("Changes orientation when opposite direction key pressed") {
            const auto previousPosition = position;
            CHECK(player.orientation() == Orientation::Left);

            player.update(pressingRight);

            CHECK(player.orientation() == Orientation::Right);
            CHECK(position == previousPosition);
          }

          SECTION("Moves along pipe when direction key pressed (left)") {
            const auto previousPosition = position;
            player.update(pressingLeft);

            const auto expectedPosition = previousPosition - base::Vector{1, 0};
            CHECK(position == expectedPosition);

            SECTION("Falls down when end of pipe reached (left)") {
              for (int i = 0; i < 4; ++i) {
                player.update(pressingLeft);
              }

              const auto positionAtEndOfPipe =
                base::Vector{pipeStartX - 1, position.y};
              CHECK(position == positionAtEndOfPipe);

              testMovementSequence(player, {
                {pressingLeft, {-1, 0}, 6},
                {{}, {0, 1}, 7},
                {{}, {0, 0}, 0}
              });
            }
          }

          SECTION("Moves along pipe when direction key pressed (right)") {
            resetOrientation(Orientation::Right);

            const auto previousPosition = position;
            player.update(pressingRight);

            const auto expectedPosition = previousPosition + base::Vector{1, 0};
            CHECK(position == expectedPosition);

            SECTION("Falls down when end of pipe reached (right)") {
              player.update(pressingRight);

              const auto playerRightEdge = pipeEndX + 1;
              const auto positionAtEndOfPipe =
                base::Vector{playerRightEdge - (PLAYER_WIDTH - 1), position.y};
              CHECK(position == positionAtEndOfPipe);

              testMovementSequence(player, {
                {pressingRight, {1, 0}, 6},
                {{}, {0, 1}, 7},
                {{}, {0, 0}, 0}
              });
            }
          }

          SECTION("Pulls legs up when up key pressed") {
            // TODO: Can't shoot when legs up
          }

          SECTION("Aims down when down key pressed") {
            // TODO: Correct shot position etc.
          }

          SECTION("Can jump off pipe") {
          }

          // TODO: Jumping respects world collision even when checking for
          // pipe attachment
        }
      }

      SECTION("Attaches while jumping") {
        SECTION("Doesn't attach on first frame of jumping") {
          testMovementSequence(player, {
            {jumpButtonTriggered, {0, 0}, 5},
            {{}, {0, -2}, 6},
            {{}, {0, -2}, 6}
          });
        }

        SECTION("Attaches on 2nd frame of jumping") {
          const auto newPipeY = position.y - 8;
          for (int i = 0; i < pipeLength; ++i) {
            map.setTileAt(0, pipeStartX + i, newPipeY, 3);
          }

          testMovementSequence(player, {
            {jumpButtonTriggered, {0, 0}, 5},
            {{}, {0, -2}, 6},
            {{}, {0, -1}, 20}
          });
        }

        SECTION("Attaches on 3nd frame of jumping") {
          const auto newPipeY = position.y - 9;
          for (int i = 0; i < pipeLength; ++i) {
            map.setTileAt(0, pipeStartX + i, newPipeY, 3);
          }

          testMovementSequence(player, {
            {jumpButtonTriggered, {0, 0}, 5},
            {{}, {0, -2}, 6},
            {{}, {0, -2}, 6},
            {{}, {0, 0}, 20}
          });
        }

        SECTION("Doesn't attach when pipe out of reach") {
          const auto newPipeY = position.y - 10;
          for (int i = 0; i < pipeLength; ++i) {
            map.setTileAt(0, pipeStartX + i, newPipeY, 3);
          }

          testMovementSequence(player, {
            {jumpButtonTriggered, {0, 0}, 5},
            {{}, {0, -2}, 6},
            {{}, {0, -2}, 6},
            {{}, {0, -1}, 6},
            {{}, {0, 0}, 6},
            {{}, {0, 0}, 6},
          });
        }

        SECTION("Doesn't attach when colliding before reaching pipe") {
          const auto newPipeY = position.y - 10;
          for (int i = 0; i < pipeLength; ++i) {
            map.setTileAt(0, pipeStartX + i, newPipeY, 3);
            map.setTileAt(0, pipeStartX + i, newPipeY + 1, 1);
          }

          testMovementSequence(player, {
            {jumpButtonTriggered, {0, 0}, 5},
            {{}, {0, -2}, 6},
            {{}, {0, -2}, 6},
            {{}, {0, 1}, 7},
            {{}, {0, 1}, 7},
          });
        }

        SECTION("Attaches with high jump") {
          const auto newPipeY = position.y - 10;
          for (int i = 0; i < pipeLength; ++i) {
            map.setTileAt(0, pipeStartX + i, newPipeY, 3);
          }

          testMovementSequence(player, {
            {jumpButtonTriggered, {0, 0}, 5},
            {{}, {0, -2}, 6},
            {{}, {0, -2}, 6},
            {{pressingJump}, {0, -1}, 6},
            {{}, {0, 0}, 20},
          });
        }
      }
    }

    SECTION("Death sequence") {
      drainMercyFrames();

      SECTION("Flies up and falls back down") {
        player.die();

        testMovementSequence(player, {
          {{}, {0, -2}, 29},
          {{}, {0, -1}, 29},
          {{}, {0, 0}, 29},
          {{}, {0, 0}, 29},
          {{}, {0, 1}, 30},
          {{}, {0, 1}, 31},
          {{}, {0, 1}, 32},
        });

        SECTION("Body disappears after some time") {
          for (int i = 0; i < 9; ++i) {
            player.update({});
          }

          CHECK(playerEntity.component<Sprite>()->mShow);

          player.update({});

          CHECK(!playerEntity.component<Sprite>()->mShow);

          SECTION("Emits event when sequence finished") {
            MockEventListener listener;
            entityx.events.subscribe<rigel::events::PlayerDied>(listener);

            for (int i = 0; i < 24; ++i) {
              player.update({});
            }

            CHECK(listener.mCallCount == 0);

            player.update({});

            CHECK(listener.mCallCount == 1);

            // Update some more frames, event shouldn't fire again
            for (int i = 0; i < 10; ++i) {
              player.update({});
            }

            CHECK(listener.mCallCount == 1);
          }
        }
      }

      SECTION("Doesn't continue walk animation after death animation when killed while walking") {
        player.update(pressingLeft);
        CHECK(animationFrame == 1);

        player.die();
        for (int i = 0; i < 9; ++i) {
          player.update({});
        }

        CHECK(animationFrame == 32);
      }

      SECTION("Cannot attach to ladder while dieing") {
        for (int yOffset = 4; yOffset < 8; ++yOffset) {
          map.setTileAt(0, position.x + 1, position.y - yOffset, 2);
        }

        const auto expectedPosition = position;

        player.die();
        for (int i = 0; i < 7; ++i) {
          player.update(pressingUp);
        }

        CHECK(animationFrame == 32);
        CHECK(position == expectedPosition);
      }

      // TODO: Collision ignored while flying up
      // TODO: Collision taken into account when falling down
      // TODO: Should we check the bounding box?
    }
  }

  //
  // ---------------------------------------------------------------------------
  //

  auto finishInteractionAnimation = [&]() {
    // TODO: Use shared constant here instead of magic number?
    for (int i = 0; i < 8; ++i) {
      player.update({});
    }

    CHECK(animationFrame != 33);
  };


  SECTION("Interaction animation") {
    player.doInteractionAnimation();
    player.update({});

    CHECK(animationFrame == 33);

    // TODO: Can die while interacting
    // TODO: Cannot attach to ladder while interacting
    // TODO: Doesn't fall while interacting (but moved by conveyor belts)

    SECTION("Cannot look up while interacting") {
      player.update(pressingUp);

      CHECK(animationFrame == 33);
    }

    SECTION("Cannot crouch while interacting") {
      player.update(pressingDown);

      CHECK(animationFrame == 33);
    }

    SECTION("Cannot change orientation while interacting") {
      const auto previousPosition = position;
      player.update(pressingRight);

      CHECK(player.orientation() == Orientation::Left);
      CHECK(position == previousPosition);
      CHECK(animationFrame == 33);
    }

    SECTION("Cannot walk while interacting") {
      const auto previousPosition = position;

      player.update(pressingLeft);

      CHECK(position == previousPosition);
      CHECK(animationFrame == 33);
    }

    SECTION("Cannot jump while interacting") {
      const auto previousPosition = position;

      player.update(jumpButtonTriggered);
      player.update({});

      CHECK(position == previousPosition);
      CHECK(animationFrame == 33);
    }

    SECTION("In normal state after interaction finished") {
      finishInteractionAnimation();

      const auto previousPosition = position;
      player.update(pressingLeft);

      CHECK(position != previousPosition);
    }
  }


  SECTION("Shooting") {
    auto& fireShotSpy = mockEntityFactory.mCreateProjectileCalls;

    auto lastFiredShot = [&]() {
      REQUIRE(!fireShotSpy.empty());
      return fireShotSpy.back();
    };

    SECTION("Fires one shot when fire button triggered") {
      player.update({});
      CHECK(fireShotSpy.size() == 0);

      player.update(fireButtonTriggered);
      CHECK(fireShotSpy.size() == 1);

      // holding the fire button doesn't trigger a shot
      player.update(pressingFire);
      CHECK(fireShotSpy.size() == 1);

      SECTION("Re-triggering fire button fires another shot") {
        player.update({});

        player.update(fireButtonTriggered);
        CHECK(fireShotSpy.size() == 2);
      }
    }


    SECTION("Shot position and direction depend on player orientation and state") {
      SECTION("Standing") {
        SECTION("Facing right") {
          resetOrientation(Orientation::Right);
          player.update(fireButtonTriggered);

          const auto expectedPosition = position + base::Vector{3, -2};
          CHECK(lastFiredShot().position == expectedPosition);
          CHECK(lastFiredShot().direction == ProjectileDirection::Right);
        }

        SECTION("Facing left") {
          resetOrientation(Orientation::Left);
          player.update(fireButtonTriggered);

          const auto expectedPosition = position + base::Vector{-1, -2};
          CHECK(lastFiredShot().position == expectedPosition);
          CHECK(lastFiredShot().direction == ProjectileDirection::Left);
        }

        SECTION("Player position offset") {
          SECTION("Facing right") {
            resetOrientation(Orientation::Right);

            player.update(fireButtonTriggered);
            CHECK(lastFiredShot().position.x == position.x + 3);
          }

          SECTION("Facing left") {
            resetOrientation(Orientation::Left);

            player.update(fireButtonTriggered);
            CHECK(lastFiredShot().position.x == position.x - 1);
          }
        }
      }

      SECTION("Crouching") {
        player.update(pressingDown);

        SECTION("Facing left") {
          player.update(fireButtonTriggered & pressingDown);

          const auto expectedPosition = position + base::Vector{-1, -1};
          CHECK(lastFiredShot().position == expectedPosition);
          CHECK(lastFiredShot().direction == ProjectileDirection::Left);
        }

        SECTION("Facing right") {
          resetOrientation(Orientation::Right);

          player.update(fireButtonTriggered & pressingDown);

          const auto expectedPosition = position + base::Vector{3, -1};
          CHECK(lastFiredShot().position == expectedPosition);
          CHECK(lastFiredShot().direction == ProjectileDirection::Right);
        }
      }

      SECTION("Looking up") {
        player.update(pressingUp);

        SECTION("Facing left") {
          player.update(fireButtonTriggered & pressingUp);

          const auto expectedPosition = position + base::Vector{0, -5};
          CHECK(lastFiredShot().position == expectedPosition);
          CHECK(lastFiredShot().direction == ProjectileDirection::Up);
        }

        SECTION("Facing right") {
          resetOrientation(Orientation::Right);

          player.update(fireButtonTriggered & pressingUp);

          const auto expectedPosition = position + base::Vector{2, -5};
          CHECK(lastFiredShot().position == expectedPosition);
          CHECK(lastFiredShot().direction == ProjectileDirection::Up);
        }
      }
    }


    SECTION("Player cannot shoot in certain states") {
      SECTION("Cannot shoot while climbing a ladder") {
        map.setTileAt(0, position.x + 1, position.y - 4, 2);
        player.update(pressingUp);
        CHECK(animationFrame == 35);

        player.update(fireButtonTriggered);

        CHECK(fireShotSpy.size() == 0);
      }

      SECTION("Cannot shoot while dieing") {
        drainMercyFrames();
        player.die();

        player.update(fireButtonTriggered);

        CHECK(fireShotSpy.size() == 0);

        SECTION("Cannot shoot when dead") {
          // Finish death animation
          for (int i = 0; i < 200; ++i) {
            player.update({});
          }

          player.update(fireButtonTriggered);

          CHECK(fireShotSpy.size() == 0);
        }
      }

      SECTION("Player cannot fire while in 'interacting' state") {
        player.doInteractionAnimation();

        player.update(fireButtonTriggered);

        CHECK(fireShotSpy.size() == 0);

        SECTION("Can fire again after interaction animation done") {
          finishInteractionAnimation();

          player.update(fireButtonTriggered);

          CHECK(fireShotSpy.size() == 1);
        }
      }
    }

    SECTION("Shot type depends on player's current weapon") {
      SECTION("Regular shot") {
        player.update(fireButtonTriggered);
        CHECK(lastFiredShot().type == ProjectileType::PlayerRegularShot);
      }

      SECTION("Laser shot") {
        playerModel.switchToWeapon(data::WeaponType::Laser);

        player.update(fireButtonTriggered);
        CHECK(lastFiredShot().type == ProjectileType::PlayerLaserShot);
      }

      SECTION("Rocket shot") {
        playerModel.switchToWeapon(data::WeaponType::Rocket);

        player.update(fireButtonTriggered);
        CHECK(lastFiredShot().type == ProjectileType::PlayerRocketShot);
      }

      SECTION("Flame shot") {
        playerModel.switchToWeapon(data::WeaponType::FlameThrower);

        player.update(fireButtonTriggered);
        CHECK(lastFiredShot().type == ProjectileType::PlayerFlameShot);
      }
    }


    SECTION("Shooting triggers appropriate sound") {
      CHECK(mockServiceProvider.mLastTriggeredSoundId == std::nullopt);

      SECTION("Normal shot") {
        player.update(fireButtonTriggered);
        REQUIRE(mockServiceProvider.mLastTriggeredSoundId != std::nullopt);
        CHECK(
            *mockServiceProvider.mLastTriggeredSoundId ==
            data::SoundId::DukeNormalShot);
      }

      SECTION("Laser") {
        playerModel.switchToWeapon(data::WeaponType::Laser);

        player.update(fireButtonTriggered);
        REQUIRE(mockServiceProvider.mLastTriggeredSoundId != std::nullopt);
        CHECK(
            *mockServiceProvider.mLastTriggeredSoundId ==
            data::SoundId::DukeLaserShot);
      }

      SECTION("Rocket launcher") {
        playerModel.switchToWeapon(data::WeaponType::Rocket);

        // The rocket launcher also uses the normal shot sound
        player.update(fireButtonTriggered);
        REQUIRE(mockServiceProvider.mLastTriggeredSoundId != std::nullopt);
        CHECK(
            *mockServiceProvider.mLastTriggeredSoundId ==
            data::SoundId::DukeNormalShot);
      }

      SECTION("Flame thrower") {
        playerModel.switchToWeapon(data::WeaponType::FlameThrower);

        player.update(fireButtonTriggered);
        REQUIRE(mockServiceProvider.mLastTriggeredSoundId != std::nullopt);
        CHECK(
            *mockServiceProvider.mLastTriggeredSoundId ==
            data::SoundId::FlameThrowerShot);
      }

      SECTION("Last shot before ammo depletion still uses appropriate sound") {
        playerModel.switchToWeapon(data::WeaponType::Laser);
        playerModel.setAmmo(1);

        player.update(fireButtonTriggered);

        REQUIRE(mockServiceProvider.mLastTriggeredSoundId != std::nullopt);
        CHECK(
            *mockServiceProvider.mLastTriggeredSoundId ==
            data::SoundId::DukeLaserShot);
      }
    }

    const auto fireOneShot = [&]() {
      player.update(fireButtonTriggered);
    };

    SECTION("Ammo consumption for non-regular weapons works") {
      SECTION("Normal shot doesn't consume ammo") {
        playerModel.setAmmo(24);
        fireOneShot();
        CHECK(playerModel.ammo() == 24);
      }

      SECTION("Laser consumes 1 unit of ammo per shot") {
        playerModel.switchToWeapon(data::WeaponType::Laser);
        playerModel.setAmmo(10);

        fireOneShot();
        CHECK(playerModel.ammo() == 9);
      }

      SECTION("Rocket launcher consumes 1 unit of ammo per shot") {
        playerModel.switchToWeapon(data::WeaponType::Rocket);
        playerModel.setAmmo(10);

        fireOneShot();
        CHECK(playerModel.ammo() == 9);
      }

      SECTION("Flame thrower consumes 1 unit of ammo per shot") {
        playerModel.switchToWeapon(data::WeaponType::FlameThrower);
        playerModel.setAmmo(10);

        fireOneShot();
        CHECK(playerModel.ammo() == 9);
      }

      SECTION("Multiple shots consume several units of ammo") {
        playerModel.switchToWeapon(data::WeaponType::Laser);
        playerModel.setAmmo(20);

        const auto shotsToFire = 15;
        for (int i = 0; i < shotsToFire; ++i) {
          fireOneShot();
        }

        CHECK(playerModel.ammo() == 20 - shotsToFire);
      }

      SECTION("Depleting ammo switches back to normal weapon") {
        playerModel.switchToWeapon(data::WeaponType::Rocket);
        playerModel.setAmmo(1);

        fireOneShot();

        CHECK(playerModel.weapon() == data::WeaponType::Normal);
        CHECK(playerModel.ammo() == playerModel.currentMaxAmmo());
      }
    }

    SECTION("Player fires continuously every other frame when owning rapid fire buff") {
      playerModel.giveItem(data::InventoryItemType::RapidFire);

      player.update(pressingFire & fireButtonTriggered);
      CHECK(fireShotSpy.size() == 1);

      player.update(pressingFire);
      player.update(pressingFire);
      CHECK(fireShotSpy.size() == 2);

      player.update(pressingFire);
      CHECK(fireShotSpy.size() == 2);

      player.update(pressingFire);
      CHECK(fireShotSpy.size() == 3);
    }

    SECTION("Firing stops when rapid fire is taken away") {
      playerModel.giveItem(data::InventoryItemType::RapidFire);

      for (int i = 0; i < 700; ++i) {
        player.update(pressingFire);
      }
      CHECK(fireShotSpy.size() == 350);

      playerModel.removeItem(data::InventoryItemType::RapidFire);

      for (int i = 0; i < 2; ++i) {
        player.update(pressingFire);
      }
      CHECK(fireShotSpy.size() == 350);
    }
  }


  SECTION("Facing right") {
    resetOrientation(Orientation::Right);

    SECTION("Doesn't move when no key pressed") {
      const auto previousPosition = position;

      player.update(PlayerInput{});

      CHECK(position == previousPosition);
    }

    SECTION("Doesn't move when both keys pressed") {
      const auto previousPosition = position;

      PlayerInput input;
      input.mLeft = true;
      input.mRight = true;
      player.update(input);

      CHECK(position == previousPosition);
    }

    SECTION("Moves right when right key pressed") {
      const auto expectedPosition = position + base::Vector{+1, 0};

      PlayerInput input;
      input.mRight = true;
      player.update(input);

      CHECK(position == expectedPosition);

      SECTION("Stops moving when key released") {
        input.mRight = false;
        player.update(input);

        CHECK(position == expectedPosition);
      }
    }

    SECTION("Changes orientation when left key pressed") {
      const auto expectedPosition = position;

      PlayerInput input;
      input.mLeft = true;
      player.update(input);

      CHECK(position == expectedPosition);
      CHECK(player.orientation() == Orientation::Left);
      CHECK(animationFrame == 0);
    }

    SECTION("Doesn't move when up against wall") {
      const auto previousPosition = position;
      makeWall(map, position.x + 2 + 1, 0, position.y + 1);

      PlayerInput input;
      input.mRight = true;
      player.update(input);

      CHECK(position == previousPosition);
    }

    SECTION("Doesn't move when up key pressed at the same time") {
      const auto previousPosition = position;

      PlayerInput input;
      input.mUp = true;
      input.mRight = true;
      player.update(input);

      CHECK(position == previousPosition);
    }

    SECTION("Doesn't move when down key pressed at the same time") {
      const auto previousPosition = position;

      PlayerInput input;
      input.mDown = true;
      input.mRight = true;
      player.update(input);

      CHECK(position == previousPosition);
    }

    SECTION("Ignores up/down keys when both pressed at the same time") {
      const auto expectedPosition = position + base::Vector{+1, 0};

      PlayerInput input;
      input.mRight = true;
      input.mUp = true;
      input.mDown = true;
      player.update(input);

      CHECK(position == expectedPosition);
    }

    SECTION("Aims up when up key pressed") {
      PlayerInput input;
      input.mUp = true;
      player.update(input);

      CHECK(animationFrame == 16);
      CHECK(player.isLookingUp());

      SECTION("isLookingUp() works correctly when recoil animation shown") {
        animationFrame = 19;
        CHECK(player.isLookingUp());
      }

      SECTION("Can change orientation while looking up") {
        const auto previousOrientation = player.orientation();

        player.update(pressingUp & pressingLeft);

        CHECK(player.isLookingUp());
        CHECK(player.orientation() != previousOrientation);
      }

      SECTION("Stops aiming up when key released") {
        input.mUp = false;
        player.update(input);

        CHECK(animationFrame == 0);
        CHECK(!player.isLookingUp());
      }
    }

    SECTION("Crouches when down key pressed") {
      PlayerInput input;
      input.mDown = true;
      player.update(input);

      CHECK(animationFrame == 17);
      CHECK(player.isCrouching());
      CHECK(
        player.worldSpaceHitBox().size.height == PLAYER_HITBOX_HEIGHT_CROUCHED);
      CHECK(bbox.size.height == PLAYER_HEIGHT_CROUCHED);

      SECTION("isCrouching() works correctly when recoil animation shown") {
        animationFrame = 34;
        CHECK(player.isCrouching());
      }

      SECTION("Can change orientation while crouching") {
        const auto previousOrientation = player.orientation();

        player.update(pressingDown & pressingLeft);

        CHECK(player.isCrouching());
        CHECK(player.orientation() != previousOrientation);
      }

      SECTION("Stops crouching when key released") {
        input.mDown = false;
        player.update(input);

        CHECK(animationFrame == 0);
        CHECK(!player.isCrouching());
        CHECK(bbox.size.height == PLAYER_HEIGHT);
      }
    }

    SECTION("Walks up a stair step") {
      map.setTileAt(0, position.x + 1 + 2, position.y, 1);
      const auto expectedPosition = position + base::Vector{+1, -1};

      PlayerInput input;
      input.mRight = true;
      player.update(input);

      CHECK(position == expectedPosition);
    }
  }
}
