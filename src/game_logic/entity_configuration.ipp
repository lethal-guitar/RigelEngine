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

namespace {

// The game draws player projectiles after drawing all regular actors, which
// makes them appear on top of everything. But in our case, they are rendered
// using the same mechanism as the other sprites, so we have to explicitly
// assign an order (which is higher than all regular actors' draw order).
const auto PLAYER_PROJECTILE_DRAW_ORDER = data::GameTraits::maxDrawOrder + 1;

// Same thing as for player projectiles
const auto MUZZLE_FLASH_DRAW_ORDER = 12;


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

    case ProjectileType::EnemyLaserShot:
      assert(isHorizontal(direction));
      return 136;

    case ProjectileType::EnemyRocket:
      return isHorizontal(direction)
        ? (isGoingRight ? 57 : 55)
        : 56;

  }

  assert(false);
  return 0;
}


float speedForProjectileType(const ProjectileType type) {
  switch (type) {
    case ProjectileType::PlayerLaserShot:
    case ProjectileType::PlayerFlameShot:
      return 5.0f;

    case ProjectileType::EnemyRocket:
      return 1.0f;

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


constexpr bool isPlayerProjectile(const ProjectileType type) {
  return
    type == ProjectileType::PlayerRegularShot ||
    type == ProjectileType::PlayerLaserShot ||
    type == ProjectileType::PlayerFlameShot ||
    type == ProjectileType::PlayerRocketShot;
}


using Message = ai::components::MessengerDrone::Message;

Message MESSAGE_TYPE_BY_INDEX[] = {
  Message::YourBrainIsOurs,
  Message::BringBackTheBrain,
  Message::LiveFromRigel,
  Message::Die,
  Message::CantEscape
};


int messengerDroneTypeIndex(const ActorID id) {
  switch (id) {
    case 213:
    case 214:
    case 215:
    case 216:
      return id - 213; // 0 to 3

    case 220:
      return 4;

    default:
      assert(false);
      return 0;
  }
}


auto createBlueGuardAiComponent(const ActorID id) {
  using ai::components::BlueGuard;

  if (id == 217) {
    return BlueGuard::typingOnTerminal();
  } else {
    const auto orientation = id == 159 ? Orientation::Right : Orientation::Left;
    return BlueGuard::patrolling(orientation);
  }
}


auto actorIDListForActor(const ActorID ID) {
  std::vector<ActorID> actorParts;

  switch (ID) {
    case 0:
      actorParts.push_back(0);
      actorParts.push_back(69);
      break;

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


    case 67:
      actorParts.push_back(67);
      actorParts.push_back(70);
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

    // Flying message ships
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
  switch (actorID) {
    case 0:
      sprite.mFramesToRender = {0};
      break;

    case 62:
      sprite.mFramesToRender = {1, 0};
      break;

    case 67:
      sprite.mFramesToRender = {0};
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
    default:
      return true;

    case 102:
    case 106:
    case 116:
    case 137:
    case 138:
    case 141:
    case 142:
    case 143:
    case 139:
    case 221:
    case 233:
    case 234:
    case 241:
    case 250:
    case 251:
    case 254:
      return false;
  }
}


ActorID actorIdForBoxColor(const ContainerColor color) {
  switch (color) {
    case ContainerColor::White: return 161;
    case ContainerColor::Green: return 162;
    case ContainerColor::Red: return 163;
    case ContainerColor::Blue: return 164;
  }

  assert(false);
  return 161;
}


int adjustedDrawOrder(const ActorID id, const int baseDrawOrder) {
  switch (id) {
    case 7: case 8: case 9: case 10:
    case 24: case 25: case 26: case 27:
    case 21: case 204: case 205: case 206:
      return PLAYER_PROJECTILE_DRAW_ORDER;

    case 33: case 34: case 35: case 36:
      return MUZZLE_FLASH_DRAW_ORDER;

    default:
      return baseDrawOrder;
  }
}

} // namespace


template<typename... Args>
void EntityFactory::configureItemContainer(
  ex::Entity entity,
  const ContainerColor color,
  const int givenScore,
  Args&&... components
) {
  configureItemContainer(
    entity, actorIdForBoxColor(color), givenScore, components...);
}


template<typename... Args>
void EntityFactory::configureItemContainer(
  ex::Entity entity,
  const ActorID containerId,
  const int givenScore,
  Args&&... components
) {
  auto physicalProperties = MovingBody{{0.0f, 0.0f}, true};
  auto activation = ActivationSettings{
    ActivationSettings::Policy::AlwaysAfterFirstActivation};

  // We don't assign a position here, as the box might move before being opened.
  // The item container's onHit callback will set the spawned entity's position
  // when the container is opened.
  components::ItemContainer container;
  container.mContainedComponents = {
    ComponentHolder{physicalProperties},
    ComponentHolder{activation},
    ComponentHolder{*entity.component<Sprite>()}
  };
  boost::fusion::for_each(
    std::make_tuple(components...),
    [&container](auto component) {
      container.mContainedComponents.emplace_back(std::move(component));
    });

  entity.remove<Sprite>();

  auto containerSprite = createSpriteForId(containerId);
  entity.assign<Sprite>(containerSprite);
  entity.assign<components::ItemContainer>(container);
  entity.assign<Shootable>(1, givenScore);
  addDefaultMovingBody(entity, engine::inferBoundingBox(containerSprite));
}


void EntityFactory::configureEntity(
  ex::Entity entity,
  const ActorID actorID,
  const BoundingBox& boundingBox
) {
  const auto difficultyOffset = mDifficulty != Difficulty::Easy
    ? (mDifficulty == Difficulty::Hard ? 2 : 1)
    : 0;

  switch (actorID) {
    // Bonus globes
    case 45:
      entity.assign<AnimationLoop>(1, 0, 3, 0);
      entity.assign<Shootable>(1, 100);
      addDefaultMovingBody(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 500;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 46:
      entity.assign<AnimationLoop>(1, 0, 3, 0);
      entity.assign<Shootable>(1, 100);
      addDefaultMovingBody(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 1000;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 47:
      entity.assign<AnimationLoop>(1, 0, 3, 0);
      entity.assign<Shootable>(1, 100);
      addDefaultMovingBody(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 5000;
        entity.assign<CollectableItem>(item);
      }
      break;

    case 48:
      entity.assign<AnimationLoop>(1, 0, 3, 0);
      entity.assign<Shootable>(1, 100);
      addDefaultMovingBody(entity, boundingBox);
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
      entity.assign<AnimationLoop>(1, 4);
      break;

    // ----------------------------------------------------------------------
    // Empty boxes
    // ----------------------------------------------------------------------
    case 162: // Empty green box
    case 163: // Empty red box
    case 164: // Empty blue box
    case 161: // Empty white box
      entity.assign<Shootable>(1, 100);
      addDefaultMovingBody(entity, boundingBox);
      break;

    // ----------------------------------------------------------------------
    // White boxes
    // ----------------------------------------------------------------------
    case 37: // Circuit board
      {
        CollectableItem item;
        item.mGivenScore = 500;
        item.mGivenItem = InventoryItemType::CircuitBoard;
        configureItemContainer(
          entity,
          ContainerColor::White,
          100,
          item,
          boundingBox);
      }
      break;

    case 121: // Blue key
      {
        CollectableItem item;
        item.mGivenScore = 500;
        item.mGivenItem = InventoryItemType::BlueKey;
        configureItemContainer(
          entity,
          ContainerColor::White,
          100,
          item,
          boundingBox);
      }
      break;

    case 53: // Rapid fire item
      {
        CollectableItem item;
        item.mGivenScore = 500;
        item.mGivenItem = InventoryItemType::RapidFire;
        item.mGivenPlayerBuff = PlayerBuff::RapidFire;
        auto animation = AnimationLoop{1};
        configureItemContainer(
          entity,
          ContainerColor::White,
          100,
          item,
          animation,
          boundingBox);
      }
      break;

    case 114: // Cloaking device
      {
        CollectableItem item;
        item.mGivenScore = 500;
        item.mGivenItem = InventoryItemType::CloakingDevice;
        item.mGivenPlayerBuff = PlayerBuff::Cloak;
        auto animation = AnimationLoop{1};
        configureItemContainer(
          entity,
          ContainerColor::White,
          100,
          item,
          animation,
          boundingBox);
      }
      break;

    // ----------------------------------------------------------------------
    // Red boxes
    // ----------------------------------------------------------------------
    case 42: // Napalm Bomb
      {
        const auto originalDrawOrder =
          entity.component<Sprite>()->mpDrawData->mDrawOrder;
        configureItemContainer(
          entity,
          ContainerColor::Red,
          0,
          AnimationLoop{1},
          boundingBox);
        entity.assign<OverrideDrawOrder>(originalDrawOrder);
      }
      break;

    case 168: // Soda can
      {
        CollectableItem item;
        item.mGivenScore = 100; // 2000 if shot and grabbed while flying
        item.mGivenHealth = 1;
        configureItemContainer(
          entity,
          ContainerColor::Red,
          100,
          item,
          AnimationLoop{1, 0, 5},
          boundingBox);
      }
      break;

    case 174: // 6-pack soda
      {
        CollectableItem item;
        item.mGivenScore = 100;
        item.mGivenHealth = 6;
        configureItemContainer(
          entity,
          ContainerColor::Red,
          100,
          item,
          Shootable{1, 10000},
          boundingBox);
      }
      break;

    case 201: // Turkey
      {
        CollectableItem item;
        // BUG in the original game: The turkey triggers a floating '100', but
        // doesn't actually give the player any score.
        //item.mGivenScore = 100;
        item.mGivenHealth = 1; // 2 if cooked
        configureItemContainer(
          entity,
          ContainerColor::Red,
          100,
          item,
          boundingBox);
      }
      break;

    // ----------------------------------------------------------------------
    // Green boxes
    // ----------------------------------------------------------------------
    case 19: // Rocket launcher
      {
        CollectableItem item;
        item.mGivenScore = 2000;
        item.mGivenWeapon = WeaponType::Rocket;
        configureItemContainer(
          entity,
          ContainerColor::Green,
          100,
          item,
          boundingBox);
      }
      break;

    case 20: // Flame thrower
      {
        CollectableItem item;
        item.mGivenScore = 2000;
        item.mGivenWeapon = WeaponType::FlameThrower;
        configureItemContainer(
          entity,
          ContainerColor::Green,
          100,
          item,
          boundingBox);
      }
      break;

    case 22: // Default weapon
      {
        CollectableItem item;
        item.mGivenWeapon = WeaponType::Normal;
        configureItemContainer(
          entity,
          ContainerColor::Green,
          100,
          item,
          boundingBox);
      }
      break;

    case 23: // Laser
      {
        CollectableItem item;
        item.mGivenScore = 2000;
        item.mGivenWeapon = WeaponType::Laser;
        configureItemContainer(
          entity,
          ContainerColor::Green,
          100,
          item,
          boundingBox);
      }
      break;

    // ----------------------------------------------------------------------
    // Blue boxes
    // ----------------------------------------------------------------------
    case 28: // Health molecule
      {
        CollectableItem item;
        item.mGivenScore = 500; // 10000 when at full health
        item.mGivenHealth = 1;
        configureItemContainer(
          entity,
          ContainerColor::Blue,
          0,
          item,
          AnimationLoop{1},
          boundingBox);
      }
      break;

    case 155: // Collectable letter N in blue box
      {
        CollectableItem item;
        item.mGivenScore = 101000;
        item.mGivenCollectableLetter = CollectableLetterType::N;
        configureItemContainer(
          entity,
          ContainerColor::Blue,
          0,
          item,
          boundingBox);
      }
      break;

    case 156: // Collectable letter U in blue box
      {
        CollectableItem item;
        item.mGivenScore = 101000;
        item.mGivenCollectableLetter = CollectableLetterType::U;
        configureItemContainer(
          entity,
          ContainerColor::Blue,
          0,
          item,
          boundingBox);
      }
      break;

    case 157: // Collectable letter K in blue box
      {
        CollectableItem item;
        item.mGivenScore = 101000;
        item.mGivenCollectableLetter = CollectableLetterType::K;
        configureItemContainer(
          entity,
          ContainerColor::Blue,
          0,
          item,
          boundingBox);
      }
      break;

    case 158: // Collectable letter E in blue box
      {
        CollectableItem item;
        item.mGivenScore = 101000;
        item.mGivenCollectableLetter = CollectableLetterType::E;
        configureItemContainer(
          entity,
          ContainerColor::Blue,
          0,
          item,
          boundingBox);
      }
      break;

    case 187: // Collectable letter M in blue box
      {
        CollectableItem item;
        item.mGivenScore = 101000;
        item.mGivenCollectableLetter = CollectableLetterType::M;
        configureItemContainer(
          entity,
          ContainerColor::Blue,
          0,
          item,
          boundingBox);
      }
      break;

    case 160: // Video game cartridge in blue box
      {
        CollectableItem item;
        item.mGivenScore = 500;
        configureItemContainer(
          entity,
          ContainerColor::Blue,
          0,
          item,
          boundingBox);
      }
      break;

    case 172: // Sunglasses in blue box
      {
        CollectableItem item;
        item.mGivenScore = 100;
        configureItemContainer(
          entity,
          ContainerColor::Blue,
          0,
          item,
          boundingBox);
      }
      break;

    case 173: // Phone in blue box
      {
        CollectableItem item;
        item.mGivenScore = 2000;
        configureItemContainer(
          entity,
          ContainerColor::Blue,
          0,
          item,
          boundingBox);
      }
      break;

    case 181: // Boom box in blue box
      {
        CollectableItem item;
        item.mGivenScore = 1000;
        configureItemContainer(
          entity,
          ContainerColor::Blue,
          0,
          item,
          boundingBox);
      }
      break;

    case 182: // Game disk in blue box
      {
        CollectableItem item;
        item.mGivenScore = 500;
        configureItemContainer(
          entity,
          ContainerColor::Blue,
          0,
          item,
          boundingBox);
      }
      break;

    case 183: // TV in blue box
      {
        CollectableItem item;
        item.mGivenScore = 1500;
        configureItemContainer(
          entity,
          ContainerColor::Blue,
          0,
          item,
          boundingBox);
      }
      break;

    case 184: // Camera in blue box
      {
        CollectableItem item;
        item.mGivenScore = 2500;
        configureItemContainer(
          entity,
          ContainerColor::Blue,
          0,
          item,
          boundingBox);
      }
      break;

    case 185: // Computer in blue box
      {
        CollectableItem item;
        item.mGivenScore = 3000;
        configureItemContainer(
          entity,
          ContainerColor::Blue,
          0,
          item,
          boundingBox);
      }
      break;

    case 186: // CD in blue box
      {
        CollectableItem item;
        item.mGivenScore = 500;
        configureItemContainer(
          entity,
          ContainerColor::Blue,
          0,
          item,
          boundingBox);
      }
      break;

    case 274: // T-Shirt in blue box
      {
        CollectableItem item;
        item.mGivenScore = 5000;
        configureItemContainer(
          entity,
          ContainerColor::Blue,
          0,
          item,
          boundingBox);
      }
      break;

    case 275: // Video tape in blue box
      {
        CollectableItem item;
        item.mGivenScore = 500;
        configureItemContainer(
          entity,
          ContainerColor::Blue,
          0,
          item,
          boundingBox);
      }
      break;

    case 50: // teleporter
    case 51: // teleporter
      entity.assign<AnimationLoop>(1);
      entity.assign<Interactable>(InteractableType::Teleporter);
      entity.assign<BoundingBox>(BoundingBox{{2, 0}, {2, 5}});
      break;

    case 239: // Special hint globe
      entity.assign<Shootable>(3, 100);
      entity.assign<AnimationLoop>(1);
      addDefaultMovingBody(entity, boundingBox);
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

    case 0: // Cylindrical robot with blinking 'head', aka hover-bot
      entity.assign<Shootable>(1 + difficultyOffset, 150);
      addDefaultMovingBody(entity, boundingBox);
      entity.component<Sprite>()->mShow = false;
      entity.assign<ai::components::HoverBot>();
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
      entity.assign<ai::components::RocketTurret>();
      break;

    case 62: // Bomb dropping space ship
      // Not player damaging, only the bombs are
      entity.assign<Shootable>(6 + difficultyOffset, 5000);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<AnimationLoop>(1, 1, 2);
      break;

    case 64: // Bouncing spike ball
      entity.assign<Shootable>(6 + difficultyOffset, 1000);
      entity.assign<PlayerDamaging>(1);
      entity.assign<BoundingBox>(boundingBox);
      break;

    case 67: // Green slime blob
      entity.assign<Shootable>(6 + difficultyOffset, 1500);
      entity.assign<PlayerDamaging>(1);
      entity.assign<ai::components::SlimeBlob>();
      addDefaultMovingBody(entity, boundingBox);
      entity.component<MovingBody>()->mGravityAffected = false;
      break;

    case 68: // Green slime container
      entity.assign<Shootable>(1, 100);
      ai::configureSlimeContainer(entity);
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

    case 115: // hover bot generator
      entity.assign<AnimationLoop>(1, 0, 3);
      entity.assign<Shootable>(20, 2500);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<ai::components::HoverBotSpawnMachine>();
      break;

    case 134: // Walking skeleton
      entity.assign<Shootable>(2 + difficultyOffset, 100);
      entity.assign<PlayerDamaging>(1);
      entity.assign<ai::components::Skeleton>();
      addDefaultMovingBody(entity, boundingBox);
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

    // Guard wearing blue space suit
    case 159: // ->
    case 171: // <-
    case 217: // using terminal
      entity.assign<PlayerDamaging>(1);
      entity.assign<Shootable>(2 + difficultyOffset, 3000);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<ActivationSettings>(
        ActivationSettings::Policy::AlwaysAfterFirstActivation);
      entity.assign<ai::components::BlueGuard>(
        createBlueGuardAiComponent(actorID));
      break;


    // Laser turret
    case 131:
      // gives one point when shot with normal shot, 500 when destroyed.
      entity.assign<BoundingBox>(boundingBox);
      ai::configureLaserTurret(entity, 500);
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
      entity.component<Shootable>()->mDestroyWhenKilled = false;
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

    case 14: // Nuclear waste barrel, empty
      entity.assign<Shootable>(1, 100);
      entity.assign<BoundingBox>(boundingBox);
      break;

    case 75: // Nuclear waste barrel, slime inside
      {
        const auto numAnimationFrames = static_cast<int>(
          entity.component<Sprite>()->mpDrawData->mFrames.size());
        configureItemContainer(
          entity,
          14,
          200,
          PlayerDamaging{1},
          AnimationLoop{1},
          AutoDestroy::afterTimeout(numAnimationFrames),
          boundingBox);
      }
      break;

    case 66: // Destroyable reactor
      entity.assign<Shootable>(10, 20000);
      entity.assign<PlayerDamaging>(9, true);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<AnimationLoop>(1);
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
      entity.assign<AnimationLoop>(1);
      break;

    case 117: // Pipe dripping green stuff
      entity.assign<AnimationLoop>(1);
      entity.assign<DrawTopMost>();
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<ai::components::SlimePipe>();
      break;

    case 208: // floating exit sign to right
    case 252: // floating exit sign to left
      entity.assign<Shootable>(5, 10000);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<AnimationLoop>(1);
      break;

    case 296: // floating arrow
      entity.assign<Shootable>(5, 500);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<AnimationLoop>(1);
      break;

    case 236: // Radar dish
      entity.assign<Shootable>(4, 2000);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<AnimationLoop>(1);
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
      entity.assign<AnimationLoop>(1);
      break;

    // Flying message ships
    case 213: // "Your brain is ours!"
    case 214: // "Bring back the brain! ... Please stand by"
    case 215: // "Live from Rigel it's Saturday night!"
    case 216: // "Die!"
    case 220: // "You cannot escape us! You will get your brain sucked!"
      {
        const auto typeIndex = messengerDroneTypeIndex(actorID);

        // The original game uses the actor's "score" field to store which
        // type of message is shown. The result is that the message ships will
        // give between 0 and 4 points of score, depending on their type.
        // It's unclear whether this is intentional, it seems like it might not
        // be because this score value is assigned in the update() function,
        // not when constructing the actor.
        entity.assign<Shootable>(1, typeIndex);
        entity.assign<BoundingBox>(boundingBox);
        entity.component<Sprite>()->mFramesToRender.clear();

        entity.assign<ai::components::MessengerDrone>(
          MESSAGE_TYPE_BY_INDEX[typeIndex]);
        entity.assign<ActivationSettings>(
          ActivationSettings::Policy::AlwaysAfterFirstActivation);
      }
      break;

    case 231: // Lava riser
      entity.assign<AnimationLoop>(1, 3, 5);
      break;

    case 246: // Rocket exhaust flame left
    case 247: // Rocket exhaust flame right
    case 248: // Small rocket exhaust flame left
    case 249: // Small rocket exhaust flame right
      entity.assign<AnimationLoop>(2);
      break;

    case 139: // level exit
      entity.assign<Trigger>(TriggerType::LevelExit);
      entity.assign<BoundingBox>(BoundingBox{{0, 0}, {1, 1}});
      break;

    case 106: // shootable wall, explodes into small pieces
      entity.assign<Shootable>(1);
      {
        // Shootable walls have a bounding box that's one unit wider than the
        // actual area.
        auto adjustedBbox = boundingBox;
        adjustedBbox.size.width += 2;
        adjustedBbox.size.height += 2;
        adjustedBbox.topLeft.x -= 1;
        adjustedBbox.topLeft.y += 1;
        entity.assign<BoundingBox>(adjustedBbox);
      }
      break;

    case 102: // dynamic wall: falls down, sinks into ground (when seen)
    case 116: // door, opened by blue key (slides into ground)
    case 137: // unknown dynamic geometry
    case 138: // dynamic wall: falls down, stays intact
    case 141: // unknown dynamic geometry
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
