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

// This file is meant to be included into entity_factory.cpp. It's only
// a separate file to make the amount of code in one file more manageable.

/* NOTE-WORTHY ACTOR IDs
 *
 * 1-2: Explosions
 * 3: Explosion when Duke's normal shot hits wall
 * 4: Technical blinking thing, maybe UI-element or part of some enemy???
 * 7-10: Duke's rocket shot
 * 11: Debris/puff of smoke
 * 12-13: Sentry robot drone debris
 * 15-18: Barrel debris
 * 21: Duke's flame thrower shot (upwards)
 * 24-25: Duke's laser shot
 * 26-27: Duke's default shot
 * 71: Debris flying upwards from dying Duke
 * 72/73: Halves of a broken bonus globe
 * 74: White circle ???
 * 75: Collapsing slime bundle, after shooting nuclear waste barrel
 * 76: Small red bomb being dropped
 * 84: Smoke cloud, dissolving (maybe for turkey?)
 * 85/86: debris/particles from destroying shootable reactor
 * 91: Player ship shots?
 * 92: exhaust flames (for player ship maybe?)
 * 94: flying slime parts (after shooting moving green slime enemy, or suction plant)
 * 95: Rocket toppling over
 * 96: debris/parts of above rocket
 * 100: eye-ball (thrown by enemy)
 * 118: Green slime drop (for pipe dripping slime)
 * 123-127: Floating score numbers
 * 136: Green laser shot
 * 137-143: ??? maybe UI elements?
 * 144: Standing rocket, flies up if shot
 * 147/148: Green laser shot muzzle flash
 * 149: Rocket exhaust flame
 * 152/153: Debris/parts of destroyed ceiling grabber claw
 * 165-167: Explosion/particle effect animations
 * 169/170: Coke can debris/parts
 * 175: The end - until duke 3d
 * 191: Technical blinking thing, maybe UI-element or part of some enemy???
 * 192-199: Stone cat debris pieces
 * 204-206: Flame thrower shots
 * 232: Single frame of spider enemy.. huh?
 * 241-242: small puff of smoke, or debris parts
 * 243: More spider
 * 255: Hand of caged monster, flying after shooting it off
 * 256: Small rocket flying upwards
 * 259: Small rocket flying downwards
 * 280: WTF ???
 * 290-294: HUD elements for collected letters
 * 300: Green slime ball, thrown by Rigelatin enemies
 */

base::Point<float> directionToVector(const ProjectileDirection direction) {
  const auto isNegative =
    direction == ProjectileDirection::Left ||
    direction == ProjectileDirection::Up;
  const auto value = isNegative ? -1.0f : 1.0f;

  using Vec = base::Point<float>;
  return isHorizontal(direction) ? Vec{value, 0.0f} : Vec{0.0f, value};
}


ActorID actorIdForProjectile(
  const ProjectileType type,
  const ProjectileDirection direction
) {
  const auto isGoingRight = direction == ProjectileDirection::Right;
  const auto isGoingUp = direction == ProjectileDirection::Up;

  switch (type) {
    case ProjectileType::PlayerRegularShot:
      return isHorizontal(direction) ? 26 : 27;

    case ProjectileType::PlayerLaserShot:
      return isHorizontal(direction) ? 24 : 25;

    case ProjectileType::PlayerRocketShot:
      return isHorizontal(direction)
        ? (isGoingRight ? 10 : 9)
        : (isGoingUp ? 7 : 8);

    case ProjectileType::PlayerFlameShot:
      return isHorizontal(direction)
        ? (isGoingRight ? 206 : 205)
        : (isGoingUp ? 21 : 204);
  }

  assert(false);
  return 0;
}


float speedForProjectileType(const ProjectileType type) {
  switch (type) {
    case ProjectileType::PlayerLaserShot:
    case ProjectileType::PlayerFlameShot:
      return 5.0f;

    default:
      return 2.0f;
  }
}


int damageForProjectileType(const ProjectileType type) {
  switch (type) {
    case ProjectileType::PlayerFlameShot:
      return 2;

    case ProjectileType::PlayerLaserShot:
      return 4;

    case ProjectileType::PlayerRocketShot:
      return 8;

    default:
      return 1;
  }
}


void configureProjectile(
  entityx::Entity entity,
  const ProjectileType type,
  WorldPosition position,
  const ProjectileDirection direction,
  const BoundingBox& boundingBox
) {
  const auto isGoingLeft = direction == ProjectileDirection::Left;

  // Position adjustment for the flame thrower shot
  if (type == ProjectileType::PlayerFlameShot) {
    if (isHorizontal(direction)) {
      position.y += 1;
    } else {
      position.x -= 1;
    }
  }

  // Position adjustment for left-facing projectiles. We want the incoming
  // position to always represent the projectile's origin, which means we need
  // to adjust the position by the projectile's length to match the left-bottom
  // corner positioning system.
  if (isHorizontal(direction) && isGoingLeft) {
    position.x -= boundingBox.size.width - 1;

    if (type == ProjectileType::PlayerFlameShot) {
      position.x += 3;
    }
  }

  const auto speed = speedForProjectileType(type);
  const auto damageAmount = damageForProjectileType(type);
  entity.assign<WorldPosition>(position);
  entity.assign<Physical>(
    Physical{directionToVector(direction) * speed, false});
  entity.assign<DamageInflicting>(damageAmount);

  entity.assign<AutoDestroy>(AutoDestroy{
    AutoDestroy::Condition::OnWorldCollision,
    AutoDestroy::Condition::OnLeavingActiveRegion});
}


void configureEntity(
  ex::Entity entity,
  const ActorID actorID,
  const BoundingBox& boundingBox,
  const Difficulty difficulty
) {
  const auto difficultyOffset = difficulty != Difficulty::Easy
    ? (difficulty == Difficulty::Hard ? 2 : 1)
    : 0;

  switch (actorID) {
    // Bonus globes
    case 45:
      entity.assign<Animated>(Animated{{AnimationSequence(2, 0, 3, 0)}});
      entity.assign<Shootable>(1, 100);
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 500;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 46:
      entity.assign<Animated>(Animated{{AnimationSequence(2, 0, 3, 0)}});
      entity.assign<Shootable>(1, 100);
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 1000;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 47:
      entity.assign<Animated>(Animated{{AnimationSequence(2, 0, 3, 0)}});
      entity.assign<Shootable>(1, 100);
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 5000;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 48:
      entity.assign<Animated>(Animated{{AnimationSequence(2, 0, 3, 0)}});
      entity.assign<Shootable>(1, 100);
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 1000;
        entity.assign<CollectableItem>(item);
      }
      break;

    // Circuit card force field
    case 119:
      interaction::configureForceField(entity);
      break;

    case 120: // Keyhole (circuit board)
      interaction::configureKeyCardSlot(entity, boundingBox);
      break;

    // Keyhole (blue key)
    case 122:
      entity.assign<Animated>(Animated{{AnimationSequence(2, 4)}});
      break;

    // ----------------------------------------------------------------------
    // Empty boxes
    // ----------------------------------------------------------------------
    case 162: // Empty green box
    case 163: // Empty red box
    case 164: // Empty blue box
    case 161: // Empty white box
      entity.assign<Shootable>(1, 100);
      addDefaultPhysical(entity, boundingBox);
      break;

    // ----------------------------------------------------------------------
    // White boxes
    // ----------------------------------------------------------------------
    case 37: // Circuit board
      // 100 pts when box is shot
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 500;
        item.mGivenItem = InventoryItemType::CircuitBoard;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 121: // Blue key
      // 100 pts when box is shot
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 500;
        item.mGivenItem = InventoryItemType::BlueKey;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 53: // Rapid fire item
      // 100 pts when box is shot
      {
        CollectableItem item;
        item.mGivenScore = 500;
        item.mGivenItem = InventoryItemType::RapidFire;
        item.mGivenPlayerBuff = PlayerBuff::RapidFire;
        entity.assign<CollectableItem>(item);
      }
      entity.assign<Animated>(Animated{{AnimationSequence(2)}});
      addDefaultPhysical(entity, boundingBox);
      break;

    case 114: // Cloaking device
      // 100 pts when box is shot
      entity.assign<Animated>(Animated{{AnimationSequence(2)}});
      {
        CollectableItem item;
        item.mGivenScore = 500;
        item.mGivenItem = InventoryItemType::CloakingDevice;
        item.mGivenPlayerBuff = PlayerBuff::Cloak;
        entity.assign<CollectableItem>(item);
      }
      addDefaultPhysical(entity, boundingBox);
      break;

    // ----------------------------------------------------------------------
    // Red boxes
    // ----------------------------------------------------------------------
    case 168: // Soda can
      // 100 pts when box is shot
      entity.assign<Animated>(Animated{{AnimationSequence(2, 0, 5)}});
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 100; // 2000 if shot and grabbed while flying
        item.mGivenHealth = 1;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 174: // 6-pack soda
      // 100 pts when box is shot
      entity.assign<Shootable>(1, 10000);
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 100;
        item.mGivenHealth = 6;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 201: // Turkey
      // 100 pts when box is shot
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        // BUG in the original game: The turkey triggers a floating '100', but
        // doesn't actually give the player any score.
        //item.mGivenScore = 100;
        item.mGivenHealth = 1; // 2 if cooked
        entity.assign<CollectableItem>(item);
      }
      break;

    // ----------------------------------------------------------------------
    // Green boxes
    // ----------------------------------------------------------------------
    case 19: // Rocket launcher
      // 100 pts when box is shot
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 2000;
        item.mGivenWeapon = WeaponType::Rocket;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 20: // Flame thrower
      // 100 pts when box is shot
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 2000;
        item.mGivenWeapon = WeaponType::FlameThrower;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 22: // Default weapon
      // 100 pts when box is shot
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenWeapon = WeaponType::Normal;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 23: // Laser
      // 100 pts when box is shot
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 2000;
        item.mGivenWeapon = WeaponType::Laser;
        entity.assign<CollectableItem>(item);
      }
      break;

    // ----------------------------------------------------------------------
    // Blue boxes
    // ----------------------------------------------------------------------
    case 28: // Health molecule
      entity.assign<Animated>(Animated{{AnimationSequence(2)}});
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 500; // 10000 when at full health
        item.mGivenHealth = 1;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 155: // Collectable letter N in blue box
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 101000;
        item.mGivenCollectableLetter = CollectableLetterType::N;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 156: // Collectable letter U in blue box
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 101000;
        item.mGivenCollectableLetter = CollectableLetterType::U;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 157: // Collectable letter K in blue box
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 101000;
        item.mGivenCollectableLetter = CollectableLetterType::K;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 158: // Collectable letter E in blue box
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 101000;
        item.mGivenCollectableLetter = CollectableLetterType::E;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 187: // Collectable letter M in blue box
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 101000;
        item.mGivenCollectableLetter = CollectableLetterType::M;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 160: // Video game cartridge in blue box
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 500;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 172: // Sunglasses in blue box
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 100;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 173: // Phone in blue box
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 2000;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 181: // Boom box in blue box
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 1000;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 182: // Game disk in blue box
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 500;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 183: // TV in blue box
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 1500;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 184: // Camera in blue box
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 2500;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 185: // Computer in blue box
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 3000;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 186: // CD in blue box
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 500;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 274: // T-Shirt in blue box
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 5000;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 275: // Video tape in blue box
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 500;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 50: // teleporter
    case 51: // teleporter
      entity.assign<Animated>(Animated{{AnimationSequence(2)}});
      entity.assign<Interactable>(InteractableType::Teleporter);
      entity.assign<BoundingBox>(BoundingBox{{2, 0}, {2, 5}});
      break;

    case 239: // Special hint globe
      entity.assign<Shootable>(3, 100);
      entity.assign<Animated>(Animated{{AnimationSequence(2)}});
      addDefaultPhysical(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 10000;
        item.mGivenItem = InventoryItemType::SpecialHintGlobe;
        entity.assign<CollectableItem>(item);
      }
      break;


    // ----------------------------------------------------------------------
    // Enemies
    // ----------------------------------------------------------------------

    case 0: // Cylindrical robot with blinking 'head'
      entity.assign<Shootable>(1 + difficultyOffset, 150);
      entity.assign<PlayerDamaging>(1);
      entity.assign<BoundingBox>(boundingBox);
      break;

    case 49: // Bouncing robot with big eye
      entity.assign<Shootable>(6 + difficultyOffset, 1000);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<PlayerDamaging>(1);
      break;

    case 54: // Rocket launcher turret
      // Shooting the rockets: 10 pts
      entity.assign<Shootable>(3, 500);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<PlayerDamaging>(1);
      break;

    case 62: // Bomb dropping space ship
      // Not player damaging, only the bombs are
      entity.assign<Shootable>(6 + difficultyOffset, 5000);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<Animated>(Animated{{AnimationSequence(2, 1, 2)}});
      break;

    case 64: // Bouncing spike ball
      entity.assign<Shootable>(6 + difficultyOffset, 1000);
      entity.assign<PlayerDamaging>(1);
      entity.assign<BoundingBox>(boundingBox);
      break;

    case 67: // Green slime blob
      entity.assign<Shootable>(6 + difficultyOffset, 500);
      entity.assign<PlayerDamaging>(1);
      entity.assign<BoundingBox>(boundingBox);
      break;

    case 68: // Green slime container
      // 100 pts when shot, 1 health
      break;

    case 78: // Snake
      // Not player damaging, but can eat duke
      // Only 1 health when Duke has been eaten
      entity.assign<Shootable>(8 + difficultyOffset, 5000);
      entity.assign<BoundingBox>(boundingBox);
      break;

    case 79: // Security camera, ceiling-mounted
    case 80: // Security camera, floor-mounted
      entity.assign<Shootable>(1, 100);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<ai::components::SecurityCamera>();
      break;

    case 81:
      entity.assign<Shootable>(15 + 3 * difficultyOffset, 300);
      entity.assign<BoundingBox>(boundingBox);
      break;

    case 98: // Eye-ball throwing monster
      entity.assign<Shootable>(8, 2000);
      entity.assign<PlayerDamaging>(1);
      entity.assign<BoundingBox>(boundingBox);
      break;

    case 134: // Walking skeleton
      entity.assign<Shootable>(2 + difficultyOffset, 100);
      entity.assign<PlayerDamaging>(1);
      entity.assign<BoundingBox>(boundingBox);
      break;

    case 151: // Floating ball, opens up and shoots lasers
      entity.assign<Shootable>(3 + difficultyOffset, 1000);
      entity.assign<PlayerDamaging>(1);
      entity.assign<BoundingBox>(boundingBox);
      break;

    case 154: // Spider
      entity.assign<Shootable>(1 + difficultyOffset, 101);
      entity.assign<PlayerDamaging>(1);
      entity.assign<BoundingBox>(boundingBox);
      break;

    case 271: // Small flying ship 1
    case 272: // Small flying ship 2
    case 273: // Small flying ship 3
      entity.assign<PlayerDamaging>(1);
      entity.assign<BoundingBox>(boundingBox);
      break;

    // Guard wearing blue suit
    case 159: // ->
    case 171: // <-
    case 217: // using terminal
      entity.assign<PlayerDamaging>(1);
      entity.assign<Shootable>(2 + difficultyOffset, 3000);
      addDefaultPhysical(entity, boundingBox);
      break;


    // Laser turret
    case 131:
      // gives one point when shot with normal shot
      // gives 500 pts when destroyed
      entity.assign<PlayerDamaging>(1);
      entity.assign<BoundingBox>(boundingBox);
      break;

    case 203: // Red bird
      entity.assign<Shootable>(1 + difficultyOffset, 100);
      entity.assign<PlayerDamaging>(1);
      entity.assign<BoundingBox>(boundingBox);
      break;

    case 253: // Monster in prison cell, aggressive
      entity.assign<ai::components::Prisoner>(true);
      entity.assign<BoundingBox>(BoundingBox{{2,0}, {3, 3}});
      entity.assign<Shootable>(1, 500);
      entity.component<Shootable>()->mInvincible = true;
      break;

    case 261: // Monster in prison cell, passive
      entity.assign<ai::components::Prisoner>(false);
      entity.assign<BoundingBox>(boundingBox);
      break;

    case 299: // Rigelatin soldier
      entity.assign<Shootable>(27 + 2 * difficultyOffset, 2100);
      entity.assign<PlayerDamaging>(1);
      entity.assign<BoundingBox>(boundingBox);
      break;

    // ----------------------------------------------------------------------
    // Various
    // ----------------------------------------------------------------------

    case 14: // Nuclear waste can, empty
      entity.assign<Shootable>(1, 100);
      entity.assign<BoundingBox>(boundingBox);
      break;

    case 75: // Nuclear waste can, slime inside
      entity.assign<Shootable>(1, 200);
      entity.assign<BoundingBox>(boundingBox);
      break;

    case 66: // Destroyable reactor
      entity.assign<Shootable>(10, 20000);
      entity.assign<PlayerDamaging>(9, true);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<Animated>(Animated{{AnimationSequence(2)}});
      break;

    case 93: // Blue force field (disabled by cloak)
      entity.assign<PlayerDamaging>(1);
      entity.assign<BoundingBox>(boundingBox);
      break;

    case 128: // Sliding door, vertical
      entity.assign<ai::components::VerticalSlidingDoor>();
      entity.assign<BoundingBox>(BoundingBox{{0, 0}, {1, 8}});
      entity.assign<engine::components::SolidBody>();
      entity.assign<CustomRenderFunc>(&renderVerticalSlidingDoor);
      break;

    case 132: // Sliding door, horizontal
      entity.assign<ai::components::HorizontalSlidingDoor>();
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<engine::components::SolidBody>();
      break;

    case 209: // Rocket elevator
      interaction::configureElevator(entity);
      break;

    case 212: // Lava pool
    case 235: // Slime pool
    case 262: // Fire (variant 1)
    case 263: // Fire (variant 2)
      entity.assign<PlayerDamaging>(1);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<Animated>(Animated{{AnimationSequence(2)}});
      break;

    case 117: // Pipe dripping green stuff
      entity.assign<Animated>(Animated{{AnimationSequence(2)}});
      entity.assign<DrawTopMost>();
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<ai::components::SlimePipe>();
      break;

    case 208: // floating exit sign to right
    case 252: // floating exit sign to left
      entity.assign<Shootable>(5, 10000);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<Animated>(Animated{{AnimationSequence(2)}});
      break;

    case 296: // floating arrow
      entity.assign<Shootable>(5, 500);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<Animated>(Animated{{AnimationSequence(2)}});
      break;

    case 236: // Radar dish
      entity.assign<Shootable>(4, 2000);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<Animated>(Animated{{AnimationSequence(2)}});
      break;

    case 188: // rotating floor spikes
    case 210: // Computer showing "Duke escaped"
    case 222: // Lava fall left
    case 223: // Lava fall right
    case 224: // Water fall left
    case 225: // Water fall right
    case 228: // Water surface splash left
    case 229: // Water surface splash center
    case 230: // Water surface splash right
    case 257: // Shallow water (variant 1)
    case 258: // Shallow water (variant 2)
      entity.assign<Animated>(Animated{{AnimationSequence(2)}});
      break;

    // Flying message ships
    case 213: // "Your brain is ours!"
    case 214: // "Bring back the brain! ... Please stand by"
    case 215: // "Live from Rigel it's Saturday night!"
    case 216: // "Die!"
    case 220: // "You cannot escape us! You will get your brain sucked!"
      //slot  0   1   2   3   4       5      6
      //frame 0   1   2   3   4   5   6   7  8   9
      //actor 107 108 109 110 111     112    113
      entity.assign<Animated>(Animated{{
        AnimationSequence(2, 4, 5, 4),
        AnimationSequence(2, 6, 7, 5),
        AnimationSequence(2, 8, 9, 6)}});

      entity.assign<Shootable>(1);
      entity.assign<BoundingBox>(boundingBox);
      break;

    case 231: // Lava riser
      entity.assign<Animated>(Animated{{AnimationSequence(2, 3, 5)}});
      break;

    case 246: // Rocket exhaust flame left
    case 247: // Rocket exhaust flame right
    case 248: // Small rocket exhaust flame left
    case 249: // Small rocket exhaust flame right
      entity.assign<Animated>(Animated{{AnimationSequence(4)}});
      break;

    case 139: // level exit
      entity.assign<Trigger>(TriggerType::LevelExit);
      entity.assign<BoundingBox>(BoundingBox{{0, 0}, {1, 1}});
      break;

    case 106: // shootable wall, explodes into small pieces
      entity.assign<Shootable>(1);
      {
        // If we keep the bounding box unchanged, the collision from the
        // underlying map geometry will prevent projectiles from ever reaching
        // the bounding box, thus preventing the wall's destruction.
        // Making the bounding box slightly wider solves this problem in a
        // simple way, without making the world collision detection more
        // complicated. It seems that the original game is actually doing
        // something similar, too.
        auto adjustedBbox = boundingBox;
        adjustedBbox.size.width += 2;
        adjustedBbox.topLeft.x -= 1;
        entity.assign<BoundingBox>(adjustedBbox);
      }
      break;

    case 102: // dynamic wall: falls down, sinks into ground (when seen)
    case 116: // door, opened by blue key (slides into ground)
    case 137: // unknown dynamic geometry
    case 138: // dynamic wall: falls down, stays intact
    case 142: // unknown dynamic geometry
    case 143: // shootable wall, burns away

    case 221: // water
    case 233: // water surface A
    case 234: // water surface B

    case 241: // windblown-spider generator
    case 250: // airlock effect, left
    case 251: // airlock effect, right
    case 254: // explosion effect trigger
      break;

    default:
      break;
  }
}


auto actorIDListForActor(const ActorID ID) {
  std::vector<ActorID> actorParts;

  switch (ID) {
    case 5: // player facing left
    case 6: // player facing right
      actorParts.push_back(5);
      actorParts.push_back(6);
      break;

    case 45:
    case 46:
    case 47:
    case 48:
      actorParts.push_back(ID);
      actorParts.push_back(44);
      break;

    case 50:
      actorParts.push_back(51);
      break;

    case 58:
      actorParts.push_back(58);
      actorParts.push_back(59);
      break;

    case 130:
      actorParts.push_back(260);
      actorParts.push_back(130);
      break;

    case 171:
    case 217:
      actorParts.push_back(159);
      break;

    case 201:
      actorParts.push_back(202);
      break;

    case 213:
    case 214:
    case 215:
    case 216:
    case 220:
      actorParts.push_back(107);
      actorParts.push_back(108);
      actorParts.push_back(109);
      actorParts.push_back(110);
      actorParts.push_back(111);
      actorParts.push_back(112);
      actorParts.push_back(113);
      actorParts.push_back(ID);
      break;

    default:
      actorParts.push_back(ID);
      break;
  }
  return actorParts;
}


void configureSprite(Sprite& sprite, const ActorID actorID) {
  if (actorID == 5 || actorID == 6) {
    for (int i=0; i<39; ++i) {
      sprite.mFrames[i].mDrawOffset.x -= 1;
    }
  }

  switch (actorID) {
    case 0:
      sprite.mFramesToRender = {0, 6};
      break;

    case 62:
      sprite.mFramesToRender = {1, 0};
      break;

    case 68:
      sprite.mFramesToRender = {0, 2, 8};
      break;

    case 93:
      sprite.mFramesToRender = {1, 3};
      break;

    case 115:
      sprite.mFramesToRender = {0, 4};
      break;

    case 150:
      sprite.mFramesToRender = {1};
      break;

    case 154:
      sprite.mFramesToRender = {6};
      break;

    case 171:
      sprite.mFramesToRender = {6};
      break;

    case 200:
      sprite.mFramesToRender = {0, 2};
      break;

    case 209:
      sprite.mFramesToRender = {5, 0};
      break;

    case 217:
      sprite.mFramesToRender = {12};
      break;

    case 231:
      sprite.mFramesToRender = {3};
      break;

    case 237:
      sprite.mFramesToRender = {0, 1, 2, 3};
      break;

    case 279:
      sprite.mFramesToRender = {0, 2};
      break;
  }
}


bool hasAssociatedSprite(const ActorID actorID) {
  switch (actorID) {
    case 102: case 106: case 116: case 137: case 138: case 142: case 143:
    case 139: case 221: case 233: case 234: case 241: case 250: case 251:
    case 254:
      return false;

    default:
      return true;
  }
}
