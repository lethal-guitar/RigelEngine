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

#pragma once

#include "base/warnings.hpp"
#include "data/game_session_data.hpp"
#include "engine/base_components.hpp"
#include "engine/movement.hpp"
#include "game_logic/global_dependencies.hpp"
#include "game_logic/player/components.hpp"
#include "game_logic_common/input.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <bitset>
#include <cstdint>
#include <variant>

namespace rigel
{
struct IGameServiceProvider;

namespace data
{
struct GameOptions;
class PersistentPlayerState;

namespace map
{
class Map;
}
} // namespace data

namespace engine
{
class CollisionChecker;
class RandomNumberGenerator;
} // namespace engine

namespace game_logic
{
struct IEntityFactory;
}
} // namespace rigel


namespace rigel::game_logic
{

// !!! These enum values are used as array indices !!!
enum class WeaponStance : std::uint8_t
{
  Regular = 0,
  RegularCrouched = 1,
  Upwards = 2,
  Downwards = 3,
  UsingJetpack = 4
};


struct OnGround
{
};

struct Jumping
{
  struct FromLadder
  {
  }; // tag

  Jumping() = default;
  explicit Jumping(const FromLadder&)
    : mJumpedFromLadder(true)
  {
  }

  std::uint16_t mFramesElapsed = 0;
  bool mJumpedFromLadder = false;
  bool mDoingSomersault = false;
};

struct Falling
{
  int mFramesElapsed = 0;
};

struct PushedByFan
{
};

struct UsingJetpack
{
};

struct RecoveringFromLanding
{
};

struct ClimbingLadder
{
};

struct OnPipe
{
};

struct InShip
{
  int mSpeed = 0;
};

struct Interacting
{
  explicit Interacting(const int duration)
    : mDuration(duration)
  {
  }

  int mDuration = 0;
  int mFramesElapsed = 0;
};


struct Incapacitated
{
  int mVisibleFramesRemaining;
};


struct GettingSuckedIntoSpace
{
  int mFramesElapsed;
};

namespace death_animation
{

struct FlyingUp
{
  FlyingUp() = default;

  int mFramesElapsed = 0;
};

struct FallingDown
{
};

struct Exploding
{
  int mFramesElapsed = 0;
};

struct Finished
{
};

} // namespace death_animation


using Dying = std::variant<
  death_animation::FlyingUp,
  death_animation::FallingDown,
  death_animation::Exploding,
  death_animation::Finished>;


using PlayerState = std::variant<
  OnGround,
  Jumping,
  Falling,
  PushedByFan,
  UsingJetpack,
  RecoveringFromLanding,
  ClimbingLadder,
  OnPipe,
  InShip,
  Interacting,
  Incapacitated,
  GettingSuckedIntoSpace,
  Dying>;


// The enum's values are chosen to match the corresponding animation frames.
// For animated states (like walking), the first frame of the cycle/sequence is
// used.
enum class VisualState
{
  Standing = 0,
  Walking = 1,
  LookingUp = 16,
  Crouching = 17,
  HangingFromPipe = 20,
  MovingOnPipe = 21,
  AimingDownOnPipe = 25,
  PullingLegsUpOnPipe = 28,
  CoilingForJumpOrLanding = 5,
  Jumping = 6,
  DoingSomersault = 9,
  Falling = 7,
  FallingFullSpeed = 8,
  Interacting = 33,
  ClimbingLadder = 35,
  UsingJetpack = 37,
  Dying = 29,
  Dead = 32,
  InShip = 100
};


struct AnimationConfig;


enum class SpiderClingPosition
{
  Head = 0,
  Weapon = 1,
  Back = 2
};


constexpr auto INTERACTION_LOCK_DURATION = 8;


class Player : public entityx::Receiver<Player>
{
public:
  Player(
    entityx::Entity entity,
    data::Difficulty difficulty,
    data::PersistentPlayerState* pModel,
    IGameServiceProvider* pServiceProvider,
    const data::GameOptions* pOptions,
    const engine::CollisionChecker* pCollisionChecker,
    const data::map::Map* pMap,
    IEntityFactory* pEntityFactory,
    entityx::EventManager* pEvents,
    engine::RandomNumberGenerator* pRandomGenerator);
  Player(const Player&) = delete;
  Player(Player&&) = default;

  Player& operator=(const Player&) = delete;
  Player& operator=(Player&&) = default;

  void synchronizeTo(const Player& other, entityx::EntityManager& es);

  void update(const PlayerInput& inputs);

  void takeDamage(int amount);
  void takeFatalDamage();
  void die();

  /** Set to true to prevent player taking damage (fatal or regular) */
  // For simplicity, this a public member instead of a getter/setter pair.
  // There is no need to encapsulate this state, and should we ever need to
  // in the future, it's easy to introduce a setter/getter then.
  bool mGodModeOn = false;

  void enterShip(
    const base::Vec2& shipPosition,
    engine::components::Orientation shipOrientation);
  void exitShip();

  void incapacitate(int framesToKeepVisible = 0);
  void setFree();

  void doInteractionAnimation();

  void reSpawnAt(const base::Vec2& spawnPosition);

  bool isInRegularState() const;

  bool isInMercyFrames() const;
  bool isCloaked() const;
  bool isDead() const;
  bool isIncapacitated() const;
  bool isLookingUp() const;
  bool isCrouching() const;
  bool isRidingElevator() const;
  engine::components::Orientation orientation() const;
  engine::components::BoundingBox worldSpaceHitBox() const;
  engine::components::BoundingBox worldSpaceCollisionBox() const;
  engine::components::BoundingBox collisionBox() const;
  const base::Vec2& position() const;
  int animationFrame() const;

  // TODO: Explain what this is
  base::Vec2 orientedPosition() const;

  template <typename StateT>
  bool stateIs() const
  {
    return std::holds_alternative<StateT>(mState);
  }

  base::Vec2& position();

  data::PersistentPlayerState& model() { return *mpPersistentPlayerState; }

  const entityx::Entity& entity() const { return mEntity; }

  void receive(const rigel::events::CloakPickedUp& event);
  void receive(const rigel::events::RapidFirePickedUp& event);
  void receive(const events::ElevatorAttachmentChanged& event);
  void receive(const events::AirLockOpened& event);

  bool hasSpiderAt(const SpiderClingPosition position) const
  {
    return mAttachedSpiders.test(static_cast<size_t>(position));
  }

  void attachSpider(const SpiderClingPosition position)
  {
    mAttachedSpiders.set(static_cast<size_t>(position));
  }

  void detachSpider(const SpiderClingPosition position)
  {
    mAttachedSpiders.reset(static_cast<size_t>(position));
  }

  void beginBeingPushedByFan();
  void endBeingPushedByFan();

private:
  struct VerticalMovementResult
  {
    engine::MovementResult mMoveResult = engine::MovementResult::Failed;
    bool mAttachedToClimbable = false;
  };

  void updateTemporaryItemExpiration();
  void updateAnimation();
  void updateMovement(
    const base::Vec2& movementVector,
    const Button& jumpButton,
    const Button& fireButton);
  void updateJumpButtonStateTracking(const Button& jumpButton);
  void updateShooting(const Button& fireButton);
  void updateLadderAttachment(const base::Vec2& movementVector);
  bool updateElevatorMovement(int movement);
  void updateHorizontalMovementInAir(const base::Vec2& movementVector);
  void updateJumpMovement(
    Jumping& state,
    const base::Vec2& movementVector,
    bool jumpPressed);
  void updateDeathAnimation();
  void updateGettingSuckedIntoSpaceAnimation(GettingSuckedIntoSpace& state);
  void updateIncapacitatedState(Incapacitated& state);

  VerticalMovementResult moveVerticallyInAir(int amount);
  bool tryAttachToClimbable();

  void updateAnimationLoop(const AnimationConfig& config);
  void resetAnimation();
  void updateMercyFramesAnimation();
  void updateCloakedAppearance();
  void updateHitBox();
  void dieIfFallenOutOfMap();

  void fireShot();
  bool canFire() const;

  void setVisualState(VisualState visualState);
  void jump();
  void jumpFromLadder(const base::Vec2& movementVector);
  void startFalling();
  void startFallingDelayed();
  void landOnGround(bool needRecoveryFrame);
  void switchOrientation();
  void switchOrientationWithPositionChange();

  PlayerState mState;
  entityx::Entity mEntity;
  entityx::Entity mAttachedElevator;
  data::PersistentPlayerState* mpPersistentPlayerState;
  IGameServiceProvider* mpServiceProvider;
  const engine::CollisionChecker* mpCollisionChecker;
  const data::map::Map* mpMap;
  IEntityFactory* mpEntityFactory;
  entityx::EventManager* mpEvents;
  engine::RandomNumberGenerator* mpRandomGenerator;
  const data::GameOptions* mpOptions;
  engine::components::BoundingBox mHitBox;
  WeaponStance mStance = WeaponStance::Regular;
  VisualState mVisualState = VisualState::Standing;
  int mMercyFramesPerHit;
  int mMercyFramesRemaining;
  int mFramesElapsedHavingRapidFire = 0;
  int mFramesElapsedHavingCloak = 0;
  std::bitset<3> mAttachedSpiders;
  bool mRapidFiredLastFrame = false;
  bool mFiredLastFrame = false;
  bool mIsOddFrame = false;
  bool mRecoilAnimationActive = false;
  bool mIsRidingElevator = false;
  bool mJumpRequested = false;
};

} // namespace rigel::game_logic
