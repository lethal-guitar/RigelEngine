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

#include "player.hpp"

#include "base/match.hpp"
#include "data/map.hpp"
#include "data/sound_ids.hpp"
#include "data/strings.hpp"
#include "data/player_model.hpp"
#include "engine/collision_checker.hpp"
#include "engine/random_number_generator.hpp"
#include "game_logic/effect_components.hpp"
#include "game_logic/entity_factory.hpp"

#include "game_service_provider.hpp"
#include "global_level_events.hpp"

#include <cassert>


namespace rigel { namespace game_logic {

using engine::moveHorizontally;
using engine::moveVertically;
using engine::MovementResult;

namespace c = engine::components;
namespace ex = entityx;

struct AnimationConfig {
  int startOfCycle;
  int endOfCycle;
};

namespace {

using EffectMovement = effects::EffectSprite::Movement;

const effects::EffectSpec PLAYER_DEATH_EFFECT_SPEC[] = {
  {effects::EffectSprite{{}, 71, EffectMovement::None}, 0},
  {effects::Particles{{2, 0}, loader::INGAME_PALETTE[6]}, 0},
  {effects::RandomExplosionSound{}, 1},
  {effects::Particles{{1, 0}, loader::INGAME_PALETTE[3], 1}, 2},
  {effects::RandomExplosionSound{}, 3},
  {effects::Particles{{2, 0}, loader::INGAME_PALETTE[10], -1}, 4},
  {effects::RandomExplosionSound{}, 5},
};


constexpr auto LADDER_CLIMB_ANIMATION = AnimationConfig{35, 36};
constexpr auto WALK_ANIMATION = AnimationConfig{1, 4};
constexpr auto CLIMB_ON_PIPE_ANIMATION = AnimationConfig{21, 24};

constexpr auto INITIAL_MERCY_FRAMES = 20;

constexpr auto TEMPORARY_ITEM_EXPIRATION_TIME = 700;
constexpr auto ITEM_ABOUT_TO_EXPIRE_TIME = TEMPORARY_ITEM_EXPIRATION_TIME - 30;


// Short jump arc: 2, 2, 1, 0, 0
constexpr std::array<int, 8> JUMP_ARC{2, 2, 1, 1, 1, 0, 0, 0};


constexpr auto DEATH_ANIMATION_STEPS = 6;

constexpr auto ELEVATOR_SPEED = 2;

constexpr auto FAN_PUSH_SPEED = 2;

constexpr std::array<int, DEATH_ANIMATION_STEPS> DEATH_ANIMATION_SEQUENCE{
  29, 29, 29, 29, 30, 31};

constexpr std::array<int, DEATH_ANIMATION_STEPS> DEATH_FLY_UP_SEQUENCE{
  -2, -1, 0, 0, 1, 1};



int mercyFramesForDifficulty(const data::Difficulty difficulty) {
  using data::Difficulty;

  switch (difficulty) {
    case Difficulty::Easy:
      return 40;

    case Difficulty::Medium:
      return 30;

    case Difficulty::Hard:
      return 20;
  }

  assert(false);
  std::terminate();
}


PlayerInput filterInput(PlayerInput input) {
  // Conflicting directional inputs are treated as if no input happened on the
  // conflicting axis. E.g. left + right both pressed -> no horizontal movement
  if (input.mLeft && input.mRight) {
    input.mLeft = input.mRight = false;
  }

  if (input.mUp && input.mDown) {
    input.mUp = input.mDown = false;
  }

  return input;
}


base::Vector inputToVec(const PlayerInput& input) {
  const auto x = input.mLeft ? -1 : (input.mRight ? 1 : 0);
  const auto y = input.mUp ? -1 : (input.mDown ? 1 : 0);
  return {x, y};
}


bool canWalkUpStairStep(
  const engine::CollisionChecker& collisionChecker,
  const engine::components::BoundingBox worldBBox,
  const int movement
) {
  auto stairSteppedBbox = worldBBox;
  stairSteppedBbox.topLeft.y -= 1;

  if (
    (movement < 0 && collisionChecker.isTouchingLeftWall(stairSteppedBbox)) ||
    (movement > 0 && collisionChecker.isTouchingRightWall(stairSteppedBbox))
  ) {
    return false;
  }

  stairSteppedBbox.topLeft.x += movement;
  return collisionChecker.isOnSolidGround(stairSteppedBbox);
}


ProjectileType projectileTypeForWeapon(const data::WeaponType weaponType) {
  using data::WeaponType;

  switch (weaponType) {
    case WeaponType::Normal:
      return ProjectileType::PlayerRegularShot;

    case WeaponType::Laser:
      return ProjectileType::PlayerLaserShot;

    case WeaponType::Rocket:
      return ProjectileType::PlayerRocketShot;

    case WeaponType::FlameThrower:
      return ProjectileType::PlayerFlameShot;
  }

  assert(false);
  return ProjectileType::PlayerRegularShot;
}


data::SoundId soundIdForWeapon(const data::WeaponType weaponType) {
  using data::SoundId;
  using data::WeaponType;

  switch (weaponType) {
    case WeaponType::Laser:
      return SoundId::DukeLaserShot;

    case WeaponType::FlameThrower:
      return SoundId::FlameThrowerShot;

    default:
      return SoundId::DukeNormalShot;
  };
}


ProjectileDirection shotDirection(
  const c::Orientation orientation,
  const WeaponStance stance
) {
  if (stance == WeaponStance::Upwards) {
    return ProjectileDirection::Up;
  } else if (stance == WeaponStance::Downwards) {
    return ProjectileDirection::Down;
  } else {
    return orientation == c::Orientation::Right
      ? ProjectileDirection::Right
      : ProjectileDirection::Left;
  }
}


base::Vector shotOffset(
  const c::Orientation orientation,
  const WeaponStance stance
) {
  const auto facingRight = orientation == c::Orientation::Right;

  if (stance == WeaponStance::Downwards) {
    return facingRight ? base::Vector{0, 1} : base::Vector{2, 1};
  }

  const auto shotOffsetHorizontal = stance == WeaponStance::Upwards
    ? (facingRight ? 2 : 0)
    : (facingRight ? 3 : -1);
  const auto shotOffsetVertical = stance != WeaponStance::RegularCrouched
    ? (stance == WeaponStance::Upwards ? -5 : -2)
    : -1;

  return base::Vector{shotOffsetHorizontal, shotOffsetVertical};
}


data::ActorID muzzleFlashActorId(const ProjectileDirection direction) {
  static const data::ActorID DIRECTION_MAP[] = { 35, 36, 33, 34 };
  return DIRECTION_MAP[static_cast<std::size_t>(direction)];
}


base::Vector muzzleFlashOffset(
  const c::Orientation orientation,
  const WeaponStance stance
) {
  const auto facingRight = orientation == c::Orientation::Right;

  if (stance == WeaponStance::Downwards) {
    return facingRight ? base::Vector{0, 1} : base::Vector{2, 1};
  }

  const auto horizontalOffset = stance == WeaponStance::Upwards
    ? (facingRight ? 2 : 0)
    : (facingRight ? 3 : -3);
  const auto verticalOffset = stance == WeaponStance::Upwards
    ? -5
    : (stance == WeaponStance::RegularCrouched ? -1 : -2);

  return {horizontalOffset, verticalOffset};
}


std::optional<int> recoilAnimationFrame(const VisualState state) {
  using VS = VisualState;
  switch (state) {
    case VS::Standing:
      return 18;

    case VS::LookingUp:
      return 19;

    case VS::Crouching:
      return 34;

    case VS::HangingFromPipe:
    case VS::MovingOnPipe:
      return 27;

    case VS::AimingDownOnPipe:
      return 26;

    case VS::UsingJetpack:
      return 38;

    default:
      break;
  }

  return std::nullopt;
}

}


Player::Player(
  ex::Entity entity,
  const data::Difficulty difficulty,
  data::PlayerModel* pPlayerModel,
  IGameServiceProvider* pServiceProvider,
  const engine::CollisionChecker* pCollisionChecker,
  const data::map::Map* pMap,
  IEntityFactory* pEntityFactory,
  ex::EventManager* pEvents,
  engine::RandomNumberGenerator* pRandomGenerator
)
  : mEntity(entity)
  , mpPlayerModel(pPlayerModel)
  , mpServiceProvider(pServiceProvider)
  , mpCollisionChecker(pCollisionChecker)
  , mpMap(pMap)
  , mpEntityFactory(pEntityFactory)
  , mpEvents(pEvents)
  , mpRandomGenerator(pRandomGenerator)
  , mMercyFramesPerHit(mercyFramesForDifficulty(difficulty))
  , mMercyFramesRemaining(INITIAL_MERCY_FRAMES)
{
  assert(mEntity.has_component<c::BoundingBox>());
  assert(mEntity.has_component<c::Orientation>());
  assert(mEntity.has_component<c::Sprite>());
  assert(mEntity.has_component<c::WorldPosition>());

  mEntity.component<c::Sprite>()->mFramesToRender = {0};

  pEvents->subscribe<events::ElevatorAttachmentChanged>(*this);
}


bool Player::isInRegularState() const {
  return stateIs<OnGround>() && !mIsRidingElevator;
}


bool Player::canTakeDamage() const {
  return !(isInMercyFrames() || isCloaked());
}


bool Player::isInMercyFrames() const {
  return mMercyFramesRemaining > 0;
}


bool Player::isCloaked() const {
  return mpPlayerModel->hasItem(data::InventoryItemType::CloakingDevice);
}


bool Player::isDead() const {
  return stateIs<Dieing>();
}


bool Player::isIncapacitated() const {
  return stateIs<Incapacitated>();
}


bool Player::isLookingUp() const {
  return mStance == WeaponStance::Upwards;
}


bool Player::isCrouching() const {
  return mStance == WeaponStance::RegularCrouched;
}


bool Player::isRidingElevator() const {
  return mIsRidingElevator;
}


engine::components::Orientation Player::orientation() const {
  return *mEntity.component<const c::Orientation>();
}


engine::components::BoundingBox Player::worldSpaceHitBox() const {
  return engine::toWorldSpace(mHitBox, position());
}


engine::components::BoundingBox Player::worldSpaceCollisionBox() const {
  return engine::toWorldSpace(
    *mEntity.component<const c::BoundingBox>(), position());
}


const base::Vector& Player::position() const {
  return *mEntity.component<const c::WorldPosition>();
}


base::Vector& Player::position() {
  return *mEntity.component<c::WorldPosition>();
}


int Player::animationFrame() const {
  return mEntity.component<const c::Sprite>()->mFramesToRender[0];
}


base::Vector Player::orientedPosition() const {
  const auto adjustment = orientation() == c::Orientation::Left ? 1 : 0;
  return position() - base::Vector{adjustment, 0};
}


void Player::receive(const events::ElevatorAttachmentChanged& event) {
  mAttachedElevator = event.mAttachedElevator;
}


void Player::beginBeingPushedByFan() {
  mState = PushedByFan{};
}


void Player::endBeingPushedByFan() {
  auto newState = Jumping{};
  newState.mFramesElapsed = 5;
  mState = newState;
  setVisualState(VisualState::Jumping);
}


void Player::update(const PlayerInput& unfilteredInput) {
  using namespace engine;

  updateTemporaryItemExpiration();

  if (!isIncapacitated() && !mIsRidingElevator) {
    applyConveyorBeltMotion(*mpCollisionChecker, *mpMap, mEntity);
  }

  if (isDead()) {
    updateDeathAnimation();
    return;
  }

  if (isIncapacitated()) {
    updateIncapacitatedState(std::get<Incapacitated>(mState));
    return;
  }

  updateAnimation();

  const auto previousPosY = position().y;
  const auto previousVisualState = mVisualState;

  const auto input = filterInput(unfilteredInput);
  const auto movementVector = inputToVec(input);

  updateLadderAttachment(movementVector);
  updateMovement(movementVector, input.mJump);
  updateShooting(input.mFire);
  updateCollisionBox();

  if (mVisualState != previousVisualState) {
    resetAnimation();
  }

  // The ladder climb animation gets a special case, since it depends on
  // knowing whether the Y position has changed
  if (mVisualState == VisualState::ClimbingLadder) {
    if (position().y % 2 != 0 && previousPosY != position().y) {
      updateAnimationLoop(LADDER_CLIMB_ANIMATION);
    }
  }

  dieIfFallenOutOfMap();
}


void Player::takeDamage(const int amount) {
  if (isDead() || mMercyFramesRemaining > 0) {
    return;
  }

  mpPlayerModel->takeDamage(amount);
  if (!mpPlayerModel->isDead()) {
    mMercyFramesRemaining = mMercyFramesPerHit;
    mpServiceProvider->playSound(data::SoundId::DukePain);
  } else {
    die();
  }
}


void Player::die() {
  mpPlayerModel->takeFatalDamage();
  mEntity.component<c::Sprite>()->mShow = true;
  mState = Dieing{};
  setVisualState(VisualState::Dieing);
  mpServiceProvider->playSound(data::SoundId::DukeDeath);
}


void Player::incapacitate(const int framesToKeepVisible) {
  if (framesToKeepVisible == 0) {
    mEntity.component<c::Sprite>()->mShow = false;
  }
  mState = Incapacitated{framesToKeepVisible};
}


void Player::setFree() {
  mEntity.component<c::Sprite>()->mShow = true;
  mState = OnGround{};
  mVisualState = VisualState::Standing;
  mEntity.component<c::Sprite>()->mFramesToRender = {0};
}


void Player::doInteractionAnimation() {
  mState = Interacting{INTERACTION_LOCK_DURATION};
}


void Player::resetAfterDeath(entityx::Entity newEntity) {
  // TODO: Refactor this - it would be much nicer if we could just consruct
  // a new player.
  mEntity = newEntity;

  resetAfterRespawn();

  mFramesElapsedHavingRapidFire = mFramesElapsedHavingCloak = 0;
}


void Player::resetAfterRespawn() {
  // TODO: Same as with resetAfterDeath()
  mState = OnGround{};
  mStance = WeaponStance::Regular;
  mVisualState = VisualState::Standing;
  mMercyFramesRemaining = INITIAL_MERCY_FRAMES;
  mRapidFiredLastFrame = false;
  mRecoilAnimationActive = false;
  mIsOddFrame = false;
  mAttachedSpiders.reset();

  mEntity.component<c::Sprite>()->mFramesToRender = {0};
}


void Player::updateTemporaryItemExpiration() {
  using data::InventoryItemType;

  auto updateExpiration = [this](
    const InventoryItemType itemType,
    const char* message,
    int& framesElapsedHavingItem
  ) {
    if (mpPlayerModel->hasItem(itemType)) {
      ++framesElapsedHavingItem;
      if (framesElapsedHavingItem == ITEM_ABOUT_TO_EXPIRE_TIME) {
        mpEvents->emit(rigel::events::PlayerMessage{message});
      }

      if (framesElapsedHavingItem >= TEMPORARY_ITEM_EXPIRATION_TIME) {
        mpPlayerModel->removeItem(itemType);
        framesElapsedHavingItem = 0;

        if (itemType == InventoryItemType::CloakingDevice) {
          mpEvents->emit(rigel::events::CloakExpired{});
        }
      }
    }
  };


  updateExpiration(
    InventoryItemType::RapidFire,
    data::Messages::RapidFireTimingOut,
    mFramesElapsedHavingRapidFire);

  updateExpiration(
    InventoryItemType::CloakingDevice,
    data::Messages::CloakTimingOut,
    mFramesElapsedHavingCloak);
}




void Player::updateAnimation() {
  if (mVisualState == VisualState::Walking && mIsOddFrame) {
    updateAnimationLoop(WALK_ANIMATION);
  }

  if (mVisualState == VisualState::MovingOnPipe && mIsOddFrame) {
    updateAnimationLoop(CLIMB_ON_PIPE_ANIMATION);
  }

  if (mRecoilAnimationActive) {
    resetAnimation();
    mRecoilAnimationActive = false;
  }

  updateMercyFramesAnimation();
  updateCloakedAppearance();

  mIsOddFrame = !mIsOddFrame;
}


void Player::updateMovement(
  const base::Vector& movementVector,
  const Button& jumpButton
) {
  mStance = WeaponStance::Regular;
  mIsRidingElevator = false;

  auto& position = *mEntity.component<c::WorldPosition>();
  auto& bbox = *mEntity.component<c::BoundingBox>();

  updateJumpButtonStateTracking(jumpButton);

  base::match(mState,
    [&, this](const OnGround&) {
      if (mAttachedElevator && movementVector.y != 0) {
        const auto didMove = updateElevatorMovement(movementVector.y);
        if (didMove) {
          mIsRidingElevator = true;
          setVisualState(VisualState::Interacting);
          return;
        }
      }

      const auto walkingDirection =
        engine::orientation::toMovement(orientation());

      if (movementVector.y != 0) {
        const auto movement = movementVector.y;

        mStance = movement < 0
          ? WeaponStance::Upwards
          : WeaponStance::RegularCrouched;

        setVisualState(
          movement < 0 ? VisualState::LookingUp : VisualState::Crouching);

        if (movementVector.x != 0 && movementVector.x != walkingDirection) {
          switchOrientation();
        }
      } else {
        setVisualState(VisualState::Standing);

        if (movementVector.x != 0) {
          const auto movement = movementVector.x;

          if (walkingDirection != movement) {
            switchOrientation();
          } else {
            const auto worldBBox = engine::toWorldSpace(bbox, position);

            const auto result =
              moveHorizontally(*mpCollisionChecker, mEntity, movement);
            if (result == MovementResult::Completed) {
              setVisualState(VisualState::Walking);
            } else if (
              canWalkUpStairStep(*mpCollisionChecker, worldBBox, movement)
            ) {
              position.x += movement;
              position.y -= 1;
              setVisualState(VisualState::Walking);
            }
          }
        }
      }

      if (
        mJumpRequested &&
        !mpCollisionChecker->isTouchingCeiling(position, bbox)
      ) {
        jump();
      } else {
        if (!mpCollisionChecker->isOnSolidGround(position, bbox)) {
          startFalling();
        }
      }
    },

    [&, this](Jumping& state) {
      updateJumpMovement(state, movementVector, jumpButton.mIsPressed);
    },

    [&, this](Falling& state) {
      // Gravity acceleration
      const auto reachedTerminalVelocity = state.mFramesElapsed >= 2;
      if (reachedTerminalVelocity) {
        setVisualState(VisualState::FallingFullSpeed);
      } else {
        setVisualState(VisualState::Falling);
        ++state.mFramesElapsed;
      }

      updateHorizontalMovementInAir(movementVector);

      // Vertical movement and landing
      const auto fallVelocity = reachedTerminalVelocity ? 2 : 1;

      const auto result = moveVerticallyInAir(fallVelocity);
      if (
        !result.mAttachedToClimbable &&
        result.mMoveResult != MovementResult::Completed
      ) {
        const auto needRecoveryFrame = reachedTerminalVelocity;
        landOnGround(needRecoveryFrame);
      }
    },

    [&, this](const PushedByFan&) {
      setVisualState(VisualState::Jumping);
      updateHorizontalMovementInAir(movementVector);
      moveVertically(*mpCollisionChecker, mEntity, -FAN_PUSH_SPEED);
    },

    [this](const RecoveringFromLanding&) {
      // TODO: What if ground disappears on this frame?
      mState = OnGround{};
      setVisualState(VisualState::Standing);
      mpServiceProvider->playSound(data::SoundId::DukeLanding);
    },

    [&, this](ClimbingLadder& state) {
      if (
        mJumpRequested &&
        !mpCollisionChecker->isTouchingCeiling(position, bbox)
      ) {
        jumpFromLadder(movementVector);
        return;
      }

      if (
        movementVector.x != 0 &&
        movementVector.x != engine::orientation::toMovement(orientation())
      ) {
        switchOrientation();
      }

      if (movementVector.y != 0) {
        const auto movement = movementVector.y;
        const auto worldBBox = engine::toWorldSpace(bbox, position);

        const auto attachX = worldBBox.topLeft.x + 1;
        const auto nextY = movement < 0
          ? worldBBox.top() - 1
          : worldBBox.bottom() + 1;

        const auto canContinue = mpMap->attributes(attachX, nextY).isLadder();

        if (canContinue) {
          moveVertically(*mpCollisionChecker, mEntity, movement);
        } else if (movement > 0) {
          startFalling();
        }
      }
    },

    [&, this](OnPipe& state) {
      if (
        movementVector.y <= 0 &&
        mJumpRequested &&
        !mpCollisionChecker->isTouchingCeiling(position, bbox)
      ) {
        jumpFromLadder(movementVector);
        return;
      }

      setVisualState(VisualState::HangingFromPipe);

      const auto orientationAsMovement =
        engine::orientation::toMovement(orientation());

      if (movementVector.y != 0) {
        const auto movement = movementVector.y;

        mStance = movement < 0
          ? WeaponStance::Upwards
          : WeaponStance::Downwards;

        setVisualState(movement < 0
          ? VisualState::PullingLegsUpOnPipe
          : VisualState::AimingDownOnPipe);

        if (movementVector.x != 0 && movementVector.x != orientationAsMovement) {
          switchOrientation();
        }

        if (mJumpRequested && movement > 0) {
          startFallingDelayed();
        }
      } else if (movementVector.x != 0) {
        if (movementVector.x != orientationAsMovement) {
          switchOrientation();
        } else {
          const auto worldBBox = engine::toWorldSpace(bbox, position);
          const auto testX = movementVector.x < 0
            ? worldBBox.topLeft.x
            : worldBBox.right();

          const auto result = moveHorizontally(
            *mpCollisionChecker, mEntity, orientationAsMovement);
          if (result != MovementResult::Failed) {
            if (mpMap->attributes(testX, worldBBox.top() - 1).isClimbable()) {
              setVisualState(VisualState::MovingOnPipe);
            } else {
              startFallingDelayed();
            }
          }
        }
      }
    },

    [this](Interacting& state) {
      setVisualState(VisualState::Interacting);

      if (state.mFramesElapsed >= state.mDuration) {
        mState = OnGround{};
        setVisualState(VisualState::Standing);
      } else {
        ++state.mFramesElapsed;
      }
    },

    [](const Dieing&) {
      // should be handled in updateDeathAnimation()
      assert(false);
    },

    [](const Incapacitated&) {
      // should be handled in top-level update()
      assert(false);
    });
}


void Player::updateJumpButtonStateTracking(const Button& jumpButton) {
  if (jumpButton.mWasTriggered) {
    mJumpRequested = true;
  }
  if (!jumpButton.mIsPressed) {
    mJumpRequested = false;
  }
}


void Player::updateShooting(const Button& fireButton) {
  if (
    stateIs<ClimbingLadder>() ||
    stateIs<Interacting>() ||
    mIsRidingElevator ||
    (stateIs<OnPipe>() && mStance == WeaponStance::Upwards) ||
    hasSpiderAt(SpiderClingPosition::Weapon)
  ) {
    return;
  }

  const auto hasRapidFire =
    mpPlayerModel->hasItem(data::InventoryItemType::RapidFire) ||
    mpPlayerModel->weapon() == data::WeaponType::FlameThrower;

  if (
    fireButton.mWasTriggered ||
    (fireButton.mIsPressed && hasRapidFire && !mRapidFiredLastFrame)
  ) {
    fireShot();
  }

  if (fireButton.mIsPressed && hasRapidFire) {
    mRapidFiredLastFrame = !mRapidFiredLastFrame;
  } else {
    mRapidFiredLastFrame = false;
  }
}


bool Player::updateElevatorMovement(const int movementDirection) {
  auto& playerPosition = *mEntity.component<c::WorldPosition>();
  auto& elevatorPosition = *mAttachedElevator.component<c::WorldPosition>();
  const auto& elevatorBbox = *mAttachedElevator.component<c::BoundingBox>();

  auto colliding = [&]() {
    const auto elevatorOnGround = mpCollisionChecker->isOnSolidGround(
      elevatorPosition, elevatorBbox);
    const auto playerTouchingCeiling = mpCollisionChecker->isTouchingCeiling(
      playerPosition,
      DEFAULT_PLAYER_BOUNDS);

    return
      (movementDirection > 0 && elevatorOnGround) ||
      (movementDirection < 0 && playerTouchingCeiling);
  };

  const auto previousY = playerPosition.y;

  for (int i = 0; i < ELEVATOR_SPEED; ++i) {
    if (colliding()) {
      break;
    }

    elevatorPosition.y += movementDirection;
    playerPosition.y += movementDirection;
  }

  return playerPosition.y != previousY;
}


void Player::updateLadderAttachment(const base::Vector& movementVector) {
  const auto& bbox = *mEntity.component<c::BoundingBox>();
  auto& position = *mEntity.component<c::WorldPosition>();

  const auto canAttachToLadder =
    !stateIs<ClimbingLadder>() &&
    (!stateIs<Jumping>() || std::get<Jumping>(mState).mFramesElapsed >= 3);
  const auto wantsToAttach = movementVector.y < 0;
  if (canAttachToLadder && wantsToAttach) {
    const auto worldBBox = engine::toWorldSpace(bbox, position);

    std::optional<base::Vector> maybeLadderTouchPoint;
    for (int i = 0; i < worldBBox.size.width; ++i) {
      const auto attributes =
        mpMap->attributes(worldBBox.left() + i, worldBBox.top());
      if (attributes.isLadder()) {
        maybeLadderTouchPoint =
          base::Vector{worldBBox.left() + i, worldBBox.top()};
        break;
      }
    }

    if (maybeLadderTouchPoint) {
      mState = ClimbingLadder{};
      setVisualState(VisualState::ClimbingLadder);

      // snap player to ladder
      const auto playerCenterX = worldBBox.topLeft.x + worldBBox.size.width / 2;
      const auto offsetToCenter = playerCenterX - maybeLadderTouchPoint->x;
      position.x -= offsetToCenter;
    }
  }
}


void Player::updateHorizontalMovementInAir(const base::Vector& movementVector) {
  if (movementVector.x != 0) {
    const auto movement = movementVector.x;
    const auto moveDirection =
      engine::orientation::toMovement(orientation());

    if (moveDirection != movement) {
      switchOrientation();
    } else {
      moveHorizontally(*mpCollisionChecker, mEntity, movement);
    }
  }
}


void Player::updateJumpMovement(
  Jumping& state,
  const base::Vector& movementVector,
  const bool jumpPressed
) {
  auto updateSomersaultAnimation = [&, this]() {
    if (state.mDoingSomersault) {
      auto& animationFrame = mEntity.component<c::Sprite>()->mFramesToRender[0];
      ++animationFrame;

      if (animationFrame == 16 || movementVector.x == 0) {
        state.mDoingSomersault = false;
        setVisualState(VisualState::Jumping);
      }
    }

    if (
      state.mFramesElapsed == 1 &&
      !state.mDoingSomersault &&
      movementVector.x != 0 &&
      mAttachedSpiders.none()
    ) {
      const auto shouldDoSomersault = mpRandomGenerator->gen() % 6 == 0;
      if (shouldDoSomersault) {
        state.mDoingSomersault = true;
        setVisualState(VisualState::DoingSomersault);
      }
    }
  };


  if (state.mFramesElapsed == 0) {
    setVisualState(VisualState::Jumping);
  }

  if (state.mFramesElapsed != 0 || state.mJumpedFromLadder) {
    updateHorizontalMovementInAir(movementVector);
  }

  if (state.mFramesElapsed >= JUMP_ARC.size()) {
    startFalling();
  } else {
    const auto offset = JUMP_ARC[state.mFramesElapsed];

    engine::MovementResult movementOutcome = engine::MovementResult::Failed;
    if (state.mFramesElapsed > 0) {
      const auto result = moveVerticallyInAir(-offset);
      if (result.mAttachedToClimbable) {
        return;
      }

      movementOutcome = result.mMoveResult;
    } else {
      movementOutcome = moveVertically(*mpCollisionChecker, mEntity, -offset);
    }

    if (movementOutcome != MovementResult::Completed) {
      if (offset == 2 && movementOutcome == MovementResult::MovedPartially) {
        // TODO: Add explanatory comment here
        state.mFramesElapsed = 3;
      } else {
        startFalling();
        return;
      }
    }

    updateSomersaultAnimation();

    // On the 3rd frame, check if we should do a high jump (jump key still
    // pressed). If not, we skip part of the jump arc, which then results
    // in the lower jump.
    const auto isShortJump =
      state.mFramesElapsed == 2 &&
      (!jumpPressed || hasSpiderAt(SpiderClingPosition::Head));
    if (isShortJump) {
      state.mFramesElapsed = 6;
    } else {
      ++state.mFramesElapsed;
    }
  }
}


void Player::updateDeathAnimation() {
  using namespace death_animation;

  auto& position = *mEntity.component<c::WorldPosition>();
  auto& animationFrame = mEntity.component<c::Sprite>()->mFramesToRender[0];
  auto& deathAnimationState = std::get<Dieing>(mState);

  base::match(deathAnimationState,
    [&, this](FlyingUp& state) {
      animationFrame =
        DEATH_ANIMATION_SEQUENCE[state.mFramesElapsed];
      position.y += DEATH_FLY_UP_SEQUENCE[state.mFramesElapsed];
      ++state.mFramesElapsed;

      if (state.mFramesElapsed >= DEATH_ANIMATION_STEPS) {
        deathAnimationState = FallingDown{};
      }
    },

    [&, this](FallingDown& state) {
      const auto result =
        moveVertically(*mpCollisionChecker, mEntity, 2);
      if (result != MovementResult::Completed) {
        deathAnimationState = Exploding{};
        animationFrame = 32;
      }
    },

    [&, this](Exploding& state) {
      ++state.mFramesElapsed;

      if (state.mFramesElapsed >= 10) {
        if (state.mFramesElapsed == 10) {
          mEntity.component<c::Sprite>()->mShow = false;
          // TODO: Use triggerEffects() here
          auto explosionEffect = components::DestructionEffects{
            PLAYER_DEATH_EFFECT_SPEC};
          explosionEffect.mActivated = true;
          mEntity.assign<components::DestructionEffects>(explosionEffect);
        }

        if (state.mFramesElapsed == 35) {
          mpEvents->emit<rigel::events::PlayerDied>();
          deathAnimationState = Finished{};
        }
      }
    },

    [&, this](const Finished&) {
      // no-op
    });
}


void Player::updateIncapacitatedState(Incapacitated& state) {
  auto& visibleFramesRemaining = state.mVisibleFramesRemaining;
  if (visibleFramesRemaining > 0) {
    --visibleFramesRemaining;
    if (visibleFramesRemaining == 0) {
      mEntity.component<c::Sprite>()->mShow = false;
    }
  }

  if (mMercyFramesRemaining > 0) {
    --mMercyFramesRemaining;
  }
}


Player::VerticalMovementResult Player::moveVerticallyInAir(const int amount) {
  const auto distance = std::abs(amount);
  const auto movement = amount < 0 ? -1 : 1;

  VerticalMovementResult result;
  result.mMoveResult = engine::MovementResult::Completed;
  for (int step = 0; step < distance; ++step) {
    auto worldBBox = worldSpaceCollisionBox();

    if (movement < 0) {
      --worldBBox.topLeft.y;
    }

    std::optional<base::Vector> maybeClimbableTouchPoint;
    for (int i = 0; i < worldBBox.size.width; ++i) {
      const auto attributes =
        mpMap->attributes(worldBBox.left() + i, worldBBox.top());
      if (attributes.isClimbable()) {
        maybeClimbableTouchPoint =
          base::Vector{worldBBox.left() + i, worldBBox.top()};
        break;
      }
    }

    if (maybeClimbableTouchPoint) {
      setVisualState(VisualState::HangingFromPipe);
      mState = OnPipe{};
      mpServiceProvider->playSound(data::SoundId::DukeAttachClimbable);
      position().y = maybeClimbableTouchPoint->y + PLAYER_HEIGHT;
      result.mAttachedToClimbable = true;
      break;
    }

    const auto moveResult =
      moveVertically(*mpCollisionChecker, mEntity, movement);

    if (moveResult != engine::MovementResult::Completed) {
      result.mMoveResult = step == 0
        ? engine::MovementResult::Failed
        : engine::MovementResult::MovedPartially;
      break;
    }
  }

  return result;
}


void Player::updateAnimationLoop(const AnimationConfig& config) {
  auto& animationFrame = mEntity.component<c::Sprite>()->mFramesToRender[0];

  ++animationFrame;

  if (animationFrame > config.endOfCycle) {
    animationFrame = config.startOfCycle;
  }
}


void Player::resetAnimation() {
  mEntity.component<c::Sprite>()->mFramesToRender[0] =
    static_cast<int>(mVisualState);
}


void Player::updateMercyFramesAnimation() {
  auto& sprite = *mEntity.component<c::Sprite>();

  if (mMercyFramesRemaining > 0) {
    sprite.mShow = true;

    const auto effectActive = mMercyFramesRemaining % 2 != 0;
    if (effectActive) {
      if (mMercyFramesRemaining > 10) {
        sprite.mShow = false;
      } else {
        sprite.flashWhite();
      }
    }

    --mMercyFramesRemaining;
  }
}


void Player::updateCloakedAppearance() {
  const auto hasCloak = isCloaked();

  auto& sprite = *mEntity.component<c::Sprite>();
  sprite.mTranslucent = hasCloak;

  if (
    hasCloak &&
    mFramesElapsedHavingCloak > ITEM_ABOUT_TO_EXPIRE_TIME &&
    mIsOddFrame
  ) {
    sprite.flashWhite();
  }
}


void Player::updateCollisionBox() {
  auto& bbox = *mEntity.component<c::BoundingBox>();
  bbox.size.height = isCrouching() ? PLAYER_HEIGHT_CROUCHED : PLAYER_HEIGHT;
}


void Player::updateHitBox() {
  using VS = VisualState;

  mHitBox = DEFAULT_PLAYER_BOUNDS;

  switch (mVisualState) {
    case VS::CoilingForJumpOrLanding:
      mHitBox.size.height = 4;
      break;

    case VS::DoingSomersault:
      mHitBox.size = {4, 4};
      break;

    case VS::LookingUp:
    case VS::HangingFromPipe:
    case VS::MovingOnPipe:
    case VS::AimingDownOnPipe:
      mHitBox.size.height = 6;
      break;

    case VS::PullingLegsUpOnPipe:
      mHitBox.size.height = 4;
      mHitBox.topLeft.y -= 2;
      break;

    case VS::Crouching:
      mHitBox.size.height = PLAYER_HITBOX_HEIGHT_CROUCHED;
      break;

    case VS::ClimbingLadder:
      mHitBox.size.width = 4;
      break;

    default:
      break;
  }
}


void Player::dieIfFallenOutOfMap() {
  if (position().y > mpMap->height() + 3) {
    mpServiceProvider->playSound(data::SoundId::DukeDeath);
    mpEvents->emit<rigel::events::PlayerDied>();
  }
}


void Player::fireShot() {
  const auto& position = *mEntity.component<c::WorldPosition>();
  const auto weaponType = mpPlayerModel->weapon();
  const auto direction = shotDirection(orientation(), mStance);

  mpEntityFactory->createProjectile(
    projectileTypeForWeapon(weaponType),
    position + shotOffset(orientation(), mStance),
    direction);
  mpPlayerModel->useAmmo();

  mpServiceProvider->playSound(soundIdForWeapon(weaponType));
  spawnOneShotSprite(
    *mpEntityFactory,
    muzzleFlashActorId(direction),
    position + muzzleFlashOffset(orientation(), mStance));

  if (auto maybeRecoilFrame = recoilAnimationFrame(mVisualState)) {
    mEntity.component<c::Sprite>()->mFramesToRender[0] = *maybeRecoilFrame;
    mRecoilAnimationActive = true;
  }

  mpEvents->emit(rigel::events::PlayerFiredShot{});
}


void Player::setVisualState(const VisualState visualState) {
  mVisualState = visualState;
  updateHitBox();
}


void Player::jump() {
  mState = Jumping{};
  setVisualState(VisualState::CoilingForJumpOrLanding);
  mpServiceProvider->playSound(data::SoundId::DukeJumping);
  mJumpRequested = false;
}


void Player::jumpFromLadder(const base::Vector& movementVector) {
  auto newState = Jumping{Jumping::FromLadder{}};
  updateJumpMovement(newState, movementVector, true);

  mState = newState;
  setVisualState(VisualState::Jumping);
  mpServiceProvider->playSound(data::SoundId::DukeJumping);
  mJumpRequested = false;
}


void Player::startFalling() {
  if (mpCollisionChecker->isOnSolidGround(worldSpaceCollisionBox())) {
    mState = OnGround{};
    setVisualState(VisualState::Standing);
  } else {
    mState = Falling{};
    setVisualState(VisualState::Falling);
    position().y += 1;
  }
}


void Player::startFallingDelayed() {
  mState = Falling{};
  setVisualState(VisualState::Jumping);
}


void Player::landOnGround(bool needRecoveryFrame) {
  if (needRecoveryFrame) {
    mState = RecoveringFromLanding{};
    setVisualState(VisualState::CoilingForJumpOrLanding);
  } else {
    mState = OnGround{};
    setVisualState(VisualState::Standing);
  }
}

void Player::switchOrientation() {
  auto& orientation = *mEntity.component<c::Orientation>();
  orientation = engine::orientation::opposite(orientation);
}

}}
