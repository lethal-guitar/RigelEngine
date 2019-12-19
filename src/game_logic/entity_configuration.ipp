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

using namespace engine::components::parameter_aliases;
using namespace game_logic::components::parameter_aliases;


// The game draws player projectiles after drawing all regular actors, which
// makes them appear on top of everything. But in our case, they are rendered
// using the same mechanism as the other sprites, so we have to explicitly
// assign an order (which is higher than all regular actors' draw order).
const auto PLAYER_PROJECTILE_DRAW_ORDER = data::GameTraits::maxDrawOrder + 1;

const auto MUZZLE_FLASH_DRAW_ORDER = PLAYER_PROJECTILE_DRAW_ORDER + 1;

const auto EFFECT_DRAW_ORDER = MUZZLE_FLASH_DRAW_ORDER + 1;


const auto SCORE_NUMBER_LIFE_TIME = 60;

const base::Point<float> SCORE_NUMBER_MOVE_SEQUENCE[] = {
  {0.0f, -1.0f},
  {0.0f, -1.0f},
  {0.0f, -1.0f},
  {0.0f, -1.0f},
  {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f},
  {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f},
  {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f},
  {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f},
  {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f},
  {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f},
  {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f},
  {0.0f, 0.0f},
  {0.0f, -1.0f}
};


const int SCORE_NUMBER_ANIMATION_SEQUENCE[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 2, 3, 4, 5, 6, 7, 6, 5, 4, 3, 2, 1,
  0, 1, 2, 3, 4, 5, 6, 7, 6, 5, 4, 3, 2, 1,
  0, 1, 2, 3, 4, 5, 6, 7, 6, 5, 4, 3, 2
};


const int FLY_ANIMATION_SEQUENCE[] = { 0, 1, 2, 1 };


const base::Point<float> CONTAINER_BOUNCE_SEQUENCE[] = {
  {0.0f, -3.0f},
  {0.0f, -2.0f},
  {0.0f, -1.0f},
  {0.0f, 0.0f},
  {0.0f, 1.0f},
  {0.0f, 2.0f},
  {0.0f, 3.0f},
  {0.0f, -1.0f},
  {0.0f, 1.0f}
};


#include "destruction_effect_specs.ipp"


// NOTE: This is only an animation sequence (as opposed to a simple loop)
// because we cannot have more than one instance of the same component type
// per entity, i.e. we can't have two AnimationLoop components.
const int SODA_CAN_ROCKET_FIRE_ANIMATION[] = {6, 7};

const int BOMB_DROPPING_ANIMATION[] = {0, 1, 1, 2};

const int HINT_GLOBE_ANIMATION[] = {
  0, 1, 2, 3, 4, 5, 4, 5, 4, 5, 4, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


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
      return isHorizontal(direction) ? data::ActorID::Duke_regular_shot_horizontal : data::ActorID::Duke_regular_shot_vertical;

    case ProjectileType::PlayerLaserShot:
      return isHorizontal(direction) ? data::ActorID::Duke_laser_shot_horizontal : data::ActorID::Duke_laser_shot_vertical;

    case ProjectileType::PlayerRocketShot:
      return isHorizontal(direction)
        ? (isGoingRight ? data::ActorID::Duke_rocket_right : data::ActorID::Duke_rocket_left)
        : (isGoingUp ? data::ActorID::Duke_rocket_up : data::ActorID::Duke_rocket_down);

    case ProjectileType::PlayerFlameShot:
      return isHorizontal(direction)
        ? (isGoingRight ? data::ActorID::Duke_flame_shot_right : data::ActorID::Duke_flame_shot_left)
        : (isGoingUp ? data::ActorID::Duke_flame_shot_up : data::ActorID::Duke_flame_shot_down);

    case ProjectileType::ReactorDebris:
      return isGoingRight ? data::ActorID::Reactor_fire_RIGHT : data::ActorID::Reactor_fire_LEFT;

    case ProjectileType::EnemyLaserShot:
      assert(isHorizontal(direction));
      return data::ActorID::Enemy_laser_shot_RIGHT;

    case ProjectileType::EnemyRocket:
      return isHorizontal(direction)
        ? (isGoingRight ? data::ActorID::Enemy_rocket_right : data::ActorID::Enemy_rocket_left)
        : data::ActorID::Enemy_rocket_up;

  }

  assert(false);
  return data::ActorID::Hoverbot;
}


float speedForProjectileType(const ProjectileType type) {
  switch (type) {
    case ProjectileType::PlayerLaserShot:
    case ProjectileType::PlayerFlameShot:
      return 5.0f;

    case ProjectileType::ReactorDebris:
      return 3.0f;
      break;

    case ProjectileType::EnemyRocket:
      return 1.0f;

    default:
      return 2.0f;
  }
}


int damageForProjectileType(const ProjectileType type) {
  switch (type) {
    case ProjectileType::PlayerFlameShot:
    case ProjectileType::PlayerLaserShot:
      return 2;

    case ProjectileType::ReactorDebris:
      return 5;
      break;

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
    case ActorID::Messenger_drone_1:
      return 0;
    case ActorID::Messenger_drone_2:
      return 1;
    case ActorID::Messenger_drone_3:
      return 2;
    case ActorID::Messenger_drone_4:
      return 3;
    case ActorID::Messenger_drone_5:
      return 4;

    default:
      assert(false);
      return 0;
  }
}


template<typename EntityLike>
void configureMovingEffectSprite(
  EntityLike& entity,
  const SpriteMovement movement
) {
  using namespace engine::components::parameter_aliases;

  entity.template assign<ActivationSettings>(ActivationSettings::Policy::Always);
  // TODO: To match the original, the condition should actually be
  // OnLeavingActiveRegion, but only after the movement sequence is
  // finished.
  entity.template assign<AutoDestroy>(AutoDestroy::afterTimeout(120));

  const auto movementIndex = static_cast<int>(movement);
  entity.template assign<MovementSequence>(MOVEMENT_SEQUENCES[movementIndex]);
  entity.template assign<MovingBody>(
    Velocity{},
    GravityAffected{false},
    IgnoreCollisions{true});
}


void assignSpecialEffectSpriteProperties(ex::Entity entity, const ActorID id) {
  switch (id) {
    case ActorID::Shot_impact_FX:
      entity.assign<BehaviorController>(behaviors::TileBurner{});
      break;

    case ActorID::Nuclear_explosion:
    case ActorID::Eyeball_projectile:
    case ActorID::Flame_thrower_fire_RIGHT:
    case ActorID::Flame_thrower_fire_LEFT:
      entity.assign<PlayerDamaging>(1);
      break;

    default:
      break;
  }
}


auto createBlueGuardAiComponent(const ActorID id) {
  using ai::components::BlueGuard;

  if (id == ActorID::Blue_guard_using_a_terminal) {
    return BlueGuard::typingOnTerminal();
  } else {
    const auto orientation = id == ActorID::Blue_guard_RIGHT ? Orientation::Right : Orientation::Left;
    return BlueGuard::patrolling(orientation);
  }
}


auto skeletonAiConfig() {
  static auto config = []() {
    ai::components::SimpleWalker::Configuration c;
    c.mAnimEnd = 3;
    c.mWalkAtFullSpeed = false;
    return c;
  }();

  return &config;
}


auto turkeyAiConfig() {
  static auto config = []() {
    ai::components::SimpleWalker::Configuration c;
    c.mAnimEnd = 1;
    c.mWalkAtFullSpeed = true;
    return c;
  }();

  return &config;
}


void configureBonusGlobe(
  ex::Entity entity,
  const BoundingBox& boundingBox,
  const int scoreValue
) {
  entity.assign<AnimationLoop>(1, 0, 3, 0);
  entity.assign<Shootable>(Health{1}, GivenScore{100});
  entity.assign<DestructionEffects>(BONUS_GLOBE_KILL_EFFECT_SPEC);
  entity.assign<ActorTag>(ActorTag::Type::ShootableBonusGlobe);
  addDefaultMovingBody(entity, boundingBox);

  CollectableItem item;
  item.mGivenScore = scoreValue;
  entity.assign<CollectableItem>(item);

  // The entity's sprite contains both the "glass ball" background as
  // well as the colored contents, by using two render slots. The background
  // is using the 2nd render slot (see actorIDListForActor()), so by removing
  // that one, we get just the content.
  auto crystalSprite = *entity.component<Sprite>();
  crystalSprite.mFramesToRender.pop_back();

  ItemContainer coloredDestructionEffect;
  coloredDestructionEffect.assign<Sprite>(crystalSprite);
  coloredDestructionEffect.assign<BoundingBox>(boundingBox);
  coloredDestructionEffect.assign<OverrideDrawOrder>(EFFECT_DRAW_ORDER);
  coloredDestructionEffect.assign<AnimationLoop>(1, 0, 3);
  configureMovingEffectSprite(coloredDestructionEffect, SpriteMovement::FlyUp);

  entity.assign<ItemContainer>(std::move(coloredDestructionEffect));
}


ActorID scoreNumberActor(const ScoreNumberType type) {
  switch (type)
  {
    case ScoreNumberType::S100: return ActorID::Score_number_FX_100;
    case ScoreNumberType::S500: return ActorID::Score_number_FX_500;
    case ScoreNumberType::S2000: return ActorID::Score_number_FX_2000;
    case ScoreNumberType::S5000: return ActorID::Score_number_FX_5000;
    case ScoreNumberType::S10000: return ActorID::Score_number_FX_10000;
  }

  assert(false);
  return ActorID::Score_number_FX_100;
}


auto actorIDListForActor(const ActorID ID) {
  std::vector<ActorID> actorParts;

  switch (ID) {
    case ActorID::Hoverbot:
      actorParts.push_back(ActorID::Hoverbot);
      actorParts.push_back(ActorID::Hoverbot_teleport_FX);
      break;

    case ActorID::Duke_LEFT:
    case ActorID::Duke_RIGHT:
      actorParts.push_back(ActorID::Duke_LEFT);
      actorParts.push_back(ActorID::Duke_RIGHT);
      break;

    case ActorID::Blue_bonus_globe_1:
    case ActorID::Blue_bonus_globe_2:
    case ActorID::Blue_bonus_globe_3:
    case ActorID::Blue_bonus_globe_4:
      actorParts.push_back(ID);
      actorParts.push_back(ActorID::Bonus_globe_shell);
      break;

    case ActorID::Teleporter_1:
      actorParts.push_back(ActorID::Teleporter_2);
      break;

    case ActorID::Green_slime_blob:
      actorParts.push_back(ActorID::Green_slime_blob);
      actorParts.push_back(ActorID::Green_slime_blob_flying_on_ceiling);
      break;

    case ActorID::Eyeball_thrower_LEFT:
      actorParts.push_back(ActorID::Eyeball_thrower_LEFT);
      actorParts.push_back(ActorID::Eyeball_thrower_RIGHT);
      break;

    case ActorID::Blowing_fan:
      actorParts.push_back(ActorID::Blowing_fan);
      actorParts.push_back(ActorID::Blowing_fan_threads_on_top);
      break;

    case ActorID::Missile_intact:
      actorParts.push_back(ActorID::Missile_intact);
      actorParts.push_back(ActorID::Missile_exhaust_flame);
      break;

    case ActorID::Blue_guard_LEFT:
    case ActorID::Blue_guard_using_a_terminal:
      actorParts.push_back(ActorID::Blue_guard_RIGHT);
      break;

    case ActorID::Red_box_turkey:
      actorParts.push_back(ActorID::Turkey);
      break;

    case ActorID::Messenger_drone_1:
    case ActorID::Messenger_drone_2:
    case ActorID::Messenger_drone_3:
    case ActorID::Messenger_drone_4:
    case ActorID::Messenger_drone_5:
      actorParts.push_back(ActorID::Messenger_drone_body);
      actorParts.push_back(ActorID::Messenger_drone_part_1);
      actorParts.push_back(ActorID::Messenger_drone_part_2);
      actorParts.push_back(ActorID::Messenger_drone_part_3);
      actorParts.push_back(ActorID::Messenger_drone_exhaust_flame_1);
      actorParts.push_back(ActorID::Messenger_drone_exhaust_flame_2);
      actorParts.push_back(ActorID::Messenger_drone_exhaust_flame_3);
      actorParts.push_back(ID);
      break;

    case ActorID::Big_green_cat_LEFT:
    case ActorID::Big_green_cat_RIGHT:
      actorParts.push_back(ActorID::Big_green_cat_LEFT);
      actorParts.push_back(ActorID::Big_green_cat_RIGHT);
      break;

    case ActorID::Spiked_green_creature_LEFT:
    case ActorID::Spiked_green_creature_RIGHT:
      actorParts.push_back(ActorID::Spiked_green_creature_LEFT);
      actorParts.push_back(ActorID::Spiked_green_creature_RIGHT);
      break;

    default:
      actorParts.push_back(ID);
      break;
  }
  return actorParts;
}


void configureSprite(Sprite& sprite, const ActorID actorID) {
  switch (actorID) {
    case ActorID::Hoverbot:
      sprite.mFramesToRender = {0};
      break;

    case ActorID::Bomb_dropping_spaceship:
      sprite.mFramesToRender = {0, 1};
      break;

    case ActorID::Green_slime_blob:
      sprite.mFramesToRender = {0};
      break;

    case ActorID::Eyeball_thrower_LEFT:
      sprite.mFramesToRender = {0};
      break;

    case ActorID::Sentry_robot_generator:
      sprite.mFramesToRender = {0, 4};
      break;

    case ActorID::Missile_intact:
      sprite.mFramesToRender = {0};
      break;

    case ActorID::Metal_grabber_claw:
      sprite.mFramesToRender = {1};
      break;

    case ActorID::Spider:
      sprite.mFramesToRender = {6};
      break;

    case ActorID::Blue_guard_LEFT:
      sprite.mFramesToRender = {6};
      break;

    case ActorID::BOSS_Episode_1:
      sprite.mFramesToRender = {0, 2};
      break;

    case ActorID::Rocket_elevator:
      sprite.mFramesToRender = {5, 0};
      break;

    case ActorID::Blue_guard_using_a_terminal:
      sprite.mFramesToRender = {12};
      break;

    case ActorID::Lava_fountain:
      // Handled by custom render func
      sprite.mFramesToRender = {};
      break;

    case ActorID::Radar_computer_terminal:
      sprite.mFramesToRender = {0, 1, 2, 3};
      break;

    case ActorID::BOSS_Episode_4:
      sprite.mFramesToRender = {0, 2};
      break;

    case ActorID::Big_green_cat_LEFT:
    case ActorID::Big_green_cat_RIGHT:
    case ActorID::Spiked_green_creature_LEFT:
    case ActorID::Spiked_green_creature_RIGHT:
      sprite.mFramesToRender = {0};
      break;

    default:
      break;
  }
}


bool hasAssociatedSprite(const ActorID actorID) {
  switch (actorID) {
    default:
      return true;

    case ActorID::Dynamic_geometry_1:
    case ActorID::Dynamic_geometry_2:
    case ActorID::Dynamic_geometry_3:
    case ActorID::Dynamic_geometry_4:
    case ActorID::Dynamic_geometry_5:
    case ActorID::Dynamic_geometry_6:
    case ActorID::Dynamic_geometry_7:
    case ActorID::Dynamic_geometry_8:
    case ActorID::Exit_trigger:
    case ActorID::Water_body:
    case ActorID::Water_surface_1:
    case ActorID::Water_surface_2:
    case ActorID::Windblown_spider_generator:
    case ActorID::Airlock_effect_LEFT:
    case ActorID::Airlock_effect_RIGHT:
    case ActorID::Explosion_FX_trigger:
      return false;
  }
}


ActorID actorIdForBoxColor(const ContainerColor color) {
  switch (color) {
    case ContainerColor::White: return ActorID::White_box_empty;
    case ContainerColor::Green: return ActorID::Green_box_empty;
    case ContainerColor::Red: return ActorID::Red_box_empty;
    case ContainerColor::Blue: return ActorID::Blue_box_empty;
  }

  assert(false);
  return ActorID::White_box_empty;
}


int adjustedDrawOrder(const ActorID id, const int baseDrawOrder) {
  auto scale = [](const int drawOrderValue) {
    constexpr auto SCALE_FACTOR = 10;
    return drawOrderValue * SCALE_FACTOR;
  };

  switch (id) {
    case ActorID::Duke_rocket_up: case ActorID::Duke_rocket_down: case ActorID::Duke_rocket_left: case ActorID::Duke_rocket_right:
    case ActorID::Duke_laser_shot_horizontal: case ActorID::Duke_laser_shot_vertical: case ActorID::Duke_regular_shot_horizontal: case ActorID::Duke_regular_shot_vertical:
    case ActorID::Duke_flame_shot_up: case ActorID::Duke_flame_shot_down: case ActorID::Duke_flame_shot_left: case ActorID::Duke_flame_shot_right:
    case ActorID::Reactor_fire_LEFT: case ActorID::Reactor_fire_RIGHT:
      return scale(PLAYER_PROJECTILE_DRAW_ORDER);

    case ActorID::Muzzle_flash_up: case ActorID::Muzzle_flash_down: case ActorID::Muzzle_flash_left: case ActorID::Muzzle_flash_right: // player muzzle flash
      return scale(MUZZLE_FLASH_DRAW_ORDER);

    case ActorID::Explosion_FX_1:
    case ActorID::Explosion_FX_2:
    case ActorID::Shot_impact_FX:
    case ActorID::Smoke_puff_FX:
    case ActorID::Hoverbot_debris_1:
    case ActorID::Hoverbot_debris_2:
    case ActorID::Nuclear_waste_can_debris_1:
    case ActorID::Nuclear_waste_can_debris_2:
    case ActorID::Nuclear_waste_can_debris_3:
    case ActorID::Nuclear_waste_can_debris_4:
    case ActorID::Flame_thrower_fire_RIGHT:
    case ActorID::Flame_thrower_fire_LEFT:
    case ActorID::Nuclear_explosion:
    case ActorID::Watchbot_container_debris_1:
    case ActorID::Watchbot_container_debris_2:
    case ActorID::Fire_bomb_fire:
    case ActorID::Duke_death_particles:
    case ActorID::Bonus_globe_debris_1:
    case ActorID::Bonus_globe_debris_2:
    case ActorID::White_circle_flash_FX:
    case ActorID::Nuclear_waste_can_green_slime_inside:
    case ActorID::Smoke_cloud_FX:
    case ActorID::Biological_enemy_debris:
    case ActorID::Missile_debris:
    case ActorID::Eyeball_projectile:
    case ActorID::Enemy_laser_muzzle_flash_1:
    case ActorID::Enemy_laser_muzzle_flash_2:
    case ActorID::Metal_grabber_claw_debris_1:
    case ActorID::Metal_grabber_claw_debris_2:
    case ActorID::Yellow_fireball_FX:
    case ActorID::Green_fireball_FX:
    case ActorID::Blue_fireball_FX:
    case ActorID::Coke_can_debris_1:
    case ActorID::Coke_can_debris_2:
    case ActorID::Spiked_green_creature_eye_FX_LEFT:
    case ActorID::Spiked_green_creature_eye_FX_RIGHT:
    case ActorID::Spiked_green_creature_stone_debris_1_LEFT:
    case ActorID::Spiked_green_creature_stone_debris_2_LEFT:
    case ActorID::Spiked_green_creature_stone_debris_3_LEFT:
    case ActorID::Spiked_green_creature_stone_debris_4_LEFT:
    case ActorID::Spiked_green_creature_stone_debris_1_RIGHT:
    case ActorID::Spiked_green_creature_stone_debris_2_RIGHT:
    case ActorID::Spiked_green_creature_stone_debris_3_RIGHT:
    case ActorID::Spiked_green_creature_stone_debris_4_RIGHT:
    case ActorID::Spider_shaken_off:
    case ActorID::Windblown_spider_generator:
    case ActorID::Spider_debris_2:
    case ActorID::Spider_blowing_in_wind:
    case ActorID::Prisoner_hand_debris:
    case ActorID::Rigelatin_soldier_projectile:
      return scale(EFFECT_DRAW_ORDER);

    case ActorID::Score_number_FX_100:
    case ActorID::Score_number_FX_500:
    case ActorID::Score_number_FX_2000:
    case ActorID::Score_number_FX_5000:
    case ActorID::Score_number_FX_10000:
      return scale(EFFECT_DRAW_ORDER);

    case ActorID::Napalm_bomb:
      // Make the bomb appear behind the bomber plane
      return scale(baseDrawOrder) - 1;

    default:
      return scale(baseDrawOrder);
  }
}


template<typename... Args>
void addToContainer(components::ItemContainer& container, Args&&... components) {
  (container.mContainedComponents.emplace_back(std::move(components)), ...);
}


template<typename... Args>
components::ItemContainer makeContainer(Args&&... components) {
  typename components::ItemContainer container;
  addToContainer(container, components...);
  return container;
}


void turnIntoContainer(
  ex::Entity entity,
  Sprite containerSprite,
  const int givenScore,
  components::ItemContainer&& container
) {
  // We don't assign a position here, as the container might move before being
  // opened. The item container's onHit callback will set the spawned entity's
  // position when the container is opened.
  auto originalSprite = *entity.component<Sprite>();
  addToContainer(container, originalSprite);

  entity.assign<components::ItemContainer>(std::move(container));
  entity.assign<Shootable>(Health{1}, givenScore);
  addDefaultMovingBody(
    entity,
    engine::inferBoundingBox(containerSprite, entity));
  entity.remove<Sprite>();
  entity.assign<Sprite>(std::move(containerSprite));
}


void addBarrelDestroyEffect(ex::Entity entity) {
  auto container = makeContainer();
  container.mStyle = ItemContainer::ReleaseStyle::NuclearWasteBarrel;
  entity.assign<ItemContainer>(std::move(container));
}


void addItemBoxDestroyEffect(ex::Entity entity) {
  auto container = makeContainer();
  container.mStyle = ItemContainer::ReleaseStyle::ItemBox;
  entity.assign<ItemContainer>(std::move(container));
}

} // namespace


template<typename... Args>
void EntityFactory::configureItemBox(
  ex::Entity entity,
  const ContainerColor color,
  const int givenScore,
  Args&&... components
) {
  auto container = makeContainer(components...);
  container.mStyle = components::ItemContainer::ReleaseStyle::ItemBox;
  addToContainer(
    container,
    Active{},
    MovementSequence{
      CONTAINER_BOUNCE_SEQUENCE, ResetAfterSequence{true}, EnableX{false}});
  addDefaultMovingBody(
    container,
    engine::inferBoundingBox(*entity.component<Sprite>(), entity));

  auto containerSprite = createSpriteForId(actorIdForBoxColor(color));
  turnIntoContainer(entity, containerSprite, givenScore, std::move(container));
  entity.assign<DestructionEffects>(CONTAINER_BOX_KILL_EFFECT_SPEC);
  entity.assign<AppearsOnRadar>();
}


void EntityFactory::configureEntity(
  ex::Entity entity,
  const ActorID actorID,
  const BoundingBox& boundingBox
) {
  using namespace effects;

  using DGType = behaviors::DynamicGeometryController::Type;

  const auto difficultyOffset = mDifficulty != Difficulty::Easy
    ? (mDifficulty == Difficulty::Hard ? 2 : 1)
    : 0;

  switch (actorID) {
    case ActorID::Blue_bonus_globe_1: // Blue bonus globe
      configureBonusGlobe(entity, boundingBox, GivenScore{500});
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Blue_bonus_globe_2: // Red bonus globe
      configureBonusGlobe(entity, boundingBox, GivenScore{2000});
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Blue_bonus_globe_3: // Green bonus globe
      configureBonusGlobe(entity, boundingBox, GivenScore{5000});
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Blue_bonus_globe_4: // White bonus globe
      configureBonusGlobe(entity, boundingBox, GivenScore{10000});
      entity.assign<AppearsOnRadar>();
      break;

    // Circuit card force field
    case ActorID::Force_field:
      entity.assign<PlayerDamaging>(9, true);
      interaction::configureForceField(entity, mSpawnIndex);
      {
        const auto& position = *entity.component<WorldPosition>();

        // There is some additional decoration representing the "emitters"
        // on top/bottom.
        auto fieldEmitters = createSprite(ActorID::Force_field, position);
        fieldEmitters.component<Sprite>()->mFramesToRender = {0, 1};
      }
      break;

    case ActorID::Circuit_card_keyhole: // Keyhole (circuit board)
      interaction::configureKeyCardSlot(entity, boundingBox);
      entity.assign<AppearsOnRadar>();
      break;

    // Keyhole (blue key)
    case ActorID::Blue_key_keyhole:
      interaction::configureKeyHole(entity, boundingBox);
      entity.assign<AppearsOnRadar>();
      break;

    // ----------------------------------------------------------------------
    // Empty boxes
    // ----------------------------------------------------------------------
    case ActorID::Green_box_empty: // Empty green box
    case ActorID::Red_box_empty: // Empty red box
    case ActorID::Blue_box_empty: // Empty blue box
    case ActorID::White_box_empty: // Empty white box
      entity.assign<Shootable>(Health{1}, GivenScore{100});
      entity.assign<DestructionEffects>(CONTAINER_BOX_KILL_EFFECT_SPEC);
      addDefaultMovingBody(entity, boundingBox);
      addItemBoxDestroyEffect(entity);
      entity.assign<AppearsOnRadar>();
      break;

    // ----------------------------------------------------------------------
    // White boxes
    // ----------------------------------------------------------------------
    case ActorID::White_box_circuit_card: // Circuit board
      {
        CollectableItem item;
        item.mGivenScore = 500;
        item.mGivenItem = InventoryItemType::CircuitBoard;
        item.mShownTutorialMessage = TutorialMessageId::FoundAccessCard;
        configureItemBox(
          entity,
          ContainerColor::White,
          100,
          item,
          AppearsOnRadar{});
        entity.remove<ActivationSettings>();
      }
      break;

    case ActorID::White_box_blue_key: // Blue key
      {
        CollectableItem item;
        item.mGivenScore = 500;
        item.mGivenItem = InventoryItemType::BlueKey;
        item.mShownTutorialMessage = TutorialMessageId::FoundBlueKey;
        configureItemBox(
          entity,
          ContainerColor::White,
          100,
          item,
          AppearsOnRadar{});
        entity.remove<ActivationSettings>();
      }
      break;

    case ActorID::White_box_rapid_fire: // Rapid fire item
      {
        CollectableItem item;
        item.mGivenScore = 500;
        item.mGivenItem = InventoryItemType::RapidFire;
        item.mShownTutorialMessage = TutorialMessageId::FoundRapidFire;
        auto animation = AnimationLoop{1};
        configureItemBox(
          entity,
          ContainerColor::White,
          100,
          item,
          animation,
          AppearsOnRadar{});
        entity.remove<ActivationSettings>();
      }
      break;

    case ActorID::White_box_cloaking_device: // Cloaking device
      {
        CollectableItem item;
        item.mGivenScore = 500;
        item.mGivenItem = InventoryItemType::CloakingDevice;
        item.mSpawnScoreNumbers = false;
        auto animation = AnimationLoop{1};
        configureItemBox(
          entity,
          ContainerColor::White,
          100,
          item,
          animation,
          AppearsOnRadar{});
        entity.remove<ActivationSettings>();
      }
      break;

    // ----------------------------------------------------------------------
    // Red boxes
    // ----------------------------------------------------------------------
    case ActorID::Red_box_bomb: // Napalm Bomb
      {
        const auto originalDrawOrder =
          entity.component<Sprite>()->mpDrawData->mDrawOrder;

        auto shootable = Shootable{Health{1}};
        shootable.mDestroyWhenKilled = false;
        configureItemBox(
          entity,
          ContainerColor::Red,
          100,
          AnimationLoop{1},
          shootable,
          ActorTag{ActorTag::Type::FireBomb},
          AppearsOnRadar{},
          DestructionEffects{
            NAPALM_BOMB_KILL_EFFECT_SPEC,
            DestructionEffects::TriggerCondition::Manual},
          BehaviorController{behaviors::NapalmBomb{}});

        entity.assign<OverrideDrawOrder>(originalDrawOrder);
        entity.assign<ActorTag>(ActorTag::Type::FireBomb);
        entity.remove<ActivationSettings>();
      }
      break;

    case ActorID::Red_box_cola: // Soda can
      {
        CollectableItem intactSodaCanCollectable;
        intactSodaCanCollectable.mGivenScore = 100;
        intactSodaCanCollectable.mGivenHealth = 1;
        intactSodaCanCollectable.mShownTutorialMessage =
          TutorialMessageId::FoundSoda;

        CollectableItem flyingSodaCanCollectable;
        flyingSodaCanCollectable.mGivenScore = 2000;

        auto flyingSodaCanSprite = *entity.component<Sprite>();
        // HACK: This is a little trick in order to get the soda can fly up
        // animation to look (almost) exactly as in the original game. The
        // problem is that (in the original) the rocket flame only appears once
        // the can has started moving, which happens one frame after being hit.
        // While our version also correctly starts movement on the frame after
        // being hit, the animation would start one frame too early if we were
        // to initialize the 2nd render slot correctly by pushing back the
        // first element of SODA_CAN_ROCKET_FIRE_ANIMATION. This would be quite
        // noticeable, since the flame can be visible through the floor tiles.
        // So to avoid that, we instead initialize the 2nd render slot with
        // frame 0, which is redundant, since the 1st render slot is already
        // showing it, but that doesn't hurt, and it will be overriden by
        // the animation sequence on the next frame.
        //
        // Note that there is still a small difference between the original and
        // our version: The "shot" soda can will always restart the soda can
        // "turn" animation from frame 0, whereas in the original game, it
        // starts from the frame that was previously shown during the
        // "intact/not shot" version. This is barely noticeable though, and
        // would require a custom Component and System in order to fix -
        // doesn't seem worth it for such a small detail.
        flyingSodaCanSprite.mFramesToRender.push_back(0);

        auto flyingSodaCanContainer = makeContainer(
          flyingSodaCanCollectable,
          flyingSodaCanSprite,
          boundingBox,
          DestructionEffects{
            SODA_CAN_ROCKET_KILL_EFFECT_SPEC,
            DestructionEffects::TriggerCondition::OnCollision},
          AnimationLoop{1, 0, 5},
          AnimationSequence{SODA_CAN_ROCKET_FIRE_ANIMATION, 1, true},
          MovingBody{Velocity{0.0f, -1.0f}, GravityAffected{false}},
          ActivationSettings{ActivationSettings::Policy::Always},
          AutoDestroy{AutoDestroy::Condition::OnWorldCollision},
          AppearsOnRadar{});

        configureItemBox(
          entity,
          ContainerColor::Red,
          100,
          intactSodaCanCollectable,
          flyingSodaCanContainer,
          Shootable{Health{1}, GivenScore{0}},
          AnimationLoop{1, 0, 5},
          AppearsOnRadar{});
      }
      break;

    case ActorID::Red_box_6_pack_cola: // 6-pack soda
      {
        CollectableItem item;
        item.mGivenScore = 100;
        item.mGivenHealth = 6;
        item.mShownTutorialMessage = TutorialMessageId::FoundSoda;
        configureItemBox(
          entity,
          ContainerColor::Red,
          100,
          item,
          Shootable{Health{1}, GivenScore{10000}},
          DestructionEffects{SODA_SIX_PACK_KILL_EFFECT_SPEC},
          AppearsOnRadar{});
      }
      break;

    case ActorID::Red_box_turkey: // Turkey
      {
        // BUG in the original game: The turkey triggers a floating '100', but
        // doesn't actually give the player any score. Therefore, we don't
        // assign givenScore here.
        CollectableItem cookedTurkeyCollectable;
        cookedTurkeyCollectable.mGivenHealth = 2;

        CollectableItem walkingTurkeyCollectable;
        walkingTurkeyCollectable.mGivenHealth = 1;

        auto cookedTurkeySprite = *entity.component<Sprite>();
        // TODO: It would be nice if we could apply startAnimationLoop() on
        // containers. Since we can't, we currently have to manually setup
        // the render slot with the right frame, in addition to adding a
        // matching AnimationLoop component
        cookedTurkeySprite.mFramesToRender[0] = 4;

        // The turkey is implemented as a nested container: First, the box
        // spawns the living turkey, which in turn is a container spawning
        // the cooked turkey.
        auto cookedTurkeyContainer = makeContainer(
          cookedTurkeyCollectable,
          cookedTurkeySprite,
          AnimationLoop{1, 4, 7},
          Active{},
          AppearsOnRadar{});
        addDefaultMovingBody(cookedTurkeyContainer, boundingBox);

        auto livingTurkeyContainer = makeContainer(
          walkingTurkeyCollectable,
          Shootable{1, 0},
          DestructionEffects{LIVING_TURKEY_KILL_EFFECT_SPEC},
          cookedTurkeyContainer,
          AnimationLoop{1, 0, 1},
          ai::components::SimpleWalker{turkeyAiConfig()},
          Active{},
          AppearsOnRadar{});
        addDefaultMovingBody(livingTurkeyContainer, boundingBox);

        // We don't use configureItemBox here, since we don't want the bounce
        // we normally get after opening a box.
        turnIntoContainer(
          entity,
          createSpriteForId(actorIdForBoxColor(ContainerColor::Red)),
          100,
          std::move(livingTurkeyContainer));
        entity.assign<DestructionEffects>(CONTAINER_BOX_KILL_EFFECT_SPEC);
        entity.component<ItemContainer>()->mStyle =
          ItemContainer::ReleaseStyle::ItemBox;
        entity.assign<AppearsOnRadar>();
      }
      break;

    // ----------------------------------------------------------------------
    // Green boxes
    // ----------------------------------------------------------------------
    case ActorID::Green_box_rocket_launcher: // Rocket launcher
      {
        CollectableItem item;
        item.mGivenScore = 2000;
        item.mGivenWeapon = WeaponType::Rocket;
        item.mShownTutorialMessage = TutorialMessageId::FoundRocketLauncher;
        configureItemBox(
          entity,
          ContainerColor::Green,
          100,
          item,
          ActorTag{ActorTag::Type::CollectableWeapon},
          AppearsOnRadar{});
        entity.assign<ActorTag>(ActorTag::Type::CollectableWeapon);
        entity.remove<ActivationSettings>();
      }
      break;

    case ActorID::Green_box_flame_thrower: // Flame thrower
      {
        CollectableItem item;
        item.mGivenScore = 2000;
        item.mGivenWeapon = WeaponType::FlameThrower;
        item.mShownTutorialMessage = TutorialMessageId::FoundFlameThrower;
        configureItemBox(
          entity,
          ContainerColor::Green,
          100,
          item,
          ActorTag{ActorTag::Type::CollectableWeapon},
          AppearsOnRadar{});
        entity.assign<ActorTag>(ActorTag::Type::CollectableWeapon);
        entity.remove<ActivationSettings>();
      }
      break;

    case ActorID::Green_box_normal_weapon: // Default weapon
      {
        CollectableItem item;
        item.mGivenScore = 2000;
        item.mGivenWeapon = WeaponType::Normal;
        item.mShownTutorialMessage = TutorialMessageId::FoundRegularWeapon;
        configureItemBox(
          entity,
          ContainerColor::Green,
          100,
          item,
          ActorTag{ActorTag::Type::CollectableWeapon},
          AppearsOnRadar{});
        entity.assign<ActorTag>(ActorTag::Type::CollectableWeapon);
        entity.remove<ActivationSettings>();
      }
      break;

    case ActorID::Green_box_laser: // Laser
      {
        CollectableItem item;
        item.mGivenScore = 2000;
        item.mGivenWeapon = WeaponType::Laser;
        item.mShownTutorialMessage = TutorialMessageId::FoundLaser;
        configureItemBox(
          entity,
          ContainerColor::Green,
          100,
          item,
          ActorTag{ActorTag::Type::CollectableWeapon},
          AppearsOnRadar{});
        entity.assign<ActorTag>(ActorTag::Type::CollectableWeapon);
        entity.remove<ActivationSettings>();
      }
      break;

    // ----------------------------------------------------------------------
    // Blue boxes
    // ----------------------------------------------------------------------
    case ActorID::Blue_box_health_molecule: // Health molecule
      {
        CollectableItem item;
        item.mGivenScore = 500;
        item.mGivenScoreAtFullHealth = 10000;
        item.mGivenHealth = 1;
        item.mShownTutorialMessage = TutorialMessageId::FoundHealthMolecule;
        configureItemBox(
          entity,
          ContainerColor::Blue,
          0,
          item,
          AnimationLoop{1},
          ActorTag{ActorTag::Type::Merchandise},
          AppearsOnRadar{});
        entity.assign<ActorTag>(ActorTag::Type::Merchandise);
      }
      break;

    case ActorID::Blue_box_N: // Collectable letter N in blue box
      {
        CollectableItem item;
        item.mGivenCollectableLetter = CollectableLetterType::N;
        item.mShownTutorialMessage = TutorialMessageId::FoundLetterN;
        configureItemBox(
          entity,
          ContainerColor::Blue,
          0,
          item,
          ActorTag{ActorTag::Type::Merchandise},
          AppearsOnRadar{});
        entity.assign<ActorTag>(ActorTag::Type::Merchandise);
      }
      break;

    case ActorID::Blue_box_U: // Collectable letter U in blue box
      {
        CollectableItem item;
        item.mGivenCollectableLetter = CollectableLetterType::U;
        item.mShownTutorialMessage = TutorialMessageId::FoundLetterU;
        configureItemBox(
          entity,
          ContainerColor::Blue,
          0,
          item,
          ActorTag{ActorTag::Type::Merchandise},
          AppearsOnRadar{});
        entity.assign<ActorTag>(ActorTag::Type::Merchandise);
      }
      break;

    case ActorID::Blue_box_K: // Collectable letter K in blue box
      {
        CollectableItem item;
        item.mGivenCollectableLetter = CollectableLetterType::K;
        item.mShownTutorialMessage = TutorialMessageId::FoundLetterK;
        configureItemBox(
          entity,
          ContainerColor::Blue,
          0,
          item,
          ActorTag{ActorTag::Type::Merchandise},
          AppearsOnRadar{});
        entity.assign<ActorTag>(ActorTag::Type::Merchandise);
      }
      break;

    case ActorID::Blue_box_E: // Collectable letter E in blue box
      {
        CollectableItem item;
        item.mGivenCollectableLetter = CollectableLetterType::E;
        item.mShownTutorialMessage = TutorialMessageId::FoundLetterE;
        configureItemBox(
          entity,
          ContainerColor::Blue,
          0,
          item,
          ActorTag{ActorTag::Type::Merchandise},
          AppearsOnRadar{});
        entity.assign<ActorTag>(ActorTag::Type::Merchandise);
      }
      break;

    case ActorID::Blue_box_M: // Collectable letter M in blue box
      {
        CollectableItem item;
        item.mGivenCollectableLetter = CollectableLetterType::M;
        configureItemBox(
          entity,
          ContainerColor::Blue,
          0,
          item,
          ActorTag{ActorTag::Type::Merchandise},
          AppearsOnRadar{});
        entity.assign<ActorTag>(ActorTag::Type::Merchandise);
      }
      break;

    case ActorID::Blue_box_video_game_cartridge: // Video game cartridge in blue box
      {
        CollectableItem item;
        item.mGivenScore = 500;
        configureItemBox(
          entity,
          ContainerColor::Blue,
          0,
          item,
          ActorTag{ActorTag::Type::Merchandise},
          AppearsOnRadar{});
        entity.assign<ActorTag>(ActorTag::Type::Merchandise);
      }
      break;

    case ActorID::Blue_box_sunglasses: // Sunglasses in blue box
      {
        CollectableItem item;
        item.mGivenScore = 100;
        configureItemBox(
          entity,
          ContainerColor::Blue,
          0,
          item,
          ActorTag{ActorTag::Type::Merchandise},
          AppearsOnRadar{});
        entity.assign<ActorTag>(ActorTag::Type::Merchandise);
      }
      break;

    case ActorID::Blue_box_phone: // Phone in blue box
      {
        CollectableItem item;
        item.mGivenScore = 2000;
        configureItemBox(
          entity,
          ContainerColor::Blue,
          0,
          item,
          ActorTag{ActorTag::Type::Merchandise},
          AppearsOnRadar{});
        entity.assign<ActorTag>(ActorTag::Type::Merchandise);
      }
      break;

    case ActorID::Blue_box_boom_box: // Boom box in blue box
      {
        CollectableItem item;
        item.mGivenScore = 1000;
        configureItemBox(
          entity,
          ContainerColor::Blue,
          0,
          item,
          ActorTag{ActorTag::Type::Merchandise},
          AppearsOnRadar{});
        entity.assign<ActorTag>(ActorTag::Type::Merchandise);
      }
      break;

    case ActorID::Blue_box_disk: // Game disk in blue box
      {
        CollectableItem item;
        item.mGivenScore = 500;
        configureItemBox(
          entity,
          ContainerColor::Blue,
          0,
          item,
          ActorTag{ActorTag::Type::Merchandise},
          AppearsOnRadar{});
        entity.assign<ActorTag>(ActorTag::Type::Merchandise);
      }
      break;

    case ActorID::Blue_box_TV: // TV in blue box
      {
        CollectableItem item;
        item.mGivenScore = 1500;
        configureItemBox(
          entity,
          ContainerColor::Blue,
          0,
          item,
          ActorTag{ActorTag::Type::Merchandise},
          AppearsOnRadar{});
        entity.assign<ActorTag>(ActorTag::Type::Merchandise);
      }
      break;

    case ActorID::Blue_box_camera: // Camera in blue box
      {
        CollectableItem item;
        item.mGivenScore = 2500;
        configureItemBox(
          entity,
          ContainerColor::Blue,
          0,
          item,
          ActorTag{ActorTag::Type::Merchandise},
          AppearsOnRadar{});
        entity.assign<ActorTag>(ActorTag::Type::Merchandise);
      }
      break;

    case ActorID::Blue_box_PC: // Computer in blue box
      {
        CollectableItem item;
        item.mGivenScore = 500;
        configureItemBox(
          entity,
          ContainerColor::Blue,
          0,
          item,
          ActorTag{ActorTag::Type::Merchandise},
          AppearsOnRadar{});
        entity.assign<ActorTag>(ActorTag::Type::Merchandise);
      }
      break;

    case ActorID::Blue_box_CD: // CD in blue box
      {
        CollectableItem item;
        item.mGivenScore = 500;
        configureItemBox(
          entity,
          ContainerColor::Blue,
          0,
          item,
          ActorTag{ActorTag::Type::Merchandise},
          AppearsOnRadar{});
        entity.assign<ActorTag>(ActorTag::Type::Merchandise);
      }
      break;

    case ActorID::Blue_box_T_shirt: // T-Shirt in blue box
      {
        CollectableItem item;
        item.mGivenScore = 5000;
        configureItemBox(
          entity,
          ContainerColor::Blue,
          0,
          item,
          ActorTag{ActorTag::Type::Merchandise},
          AppearsOnRadar{});
        entity.assign<ActorTag>(ActorTag::Type::Merchandise);
      }
      break;

    case ActorID::Blue_box_videocassette: // Video tape in blue box
      {
        CollectableItem item;
        item.mGivenScore = 500;
        configureItemBox(
          entity,
          ContainerColor::Blue,
          0,
          item,
          ActorTag{ActorTag::Type::Merchandise},
          AppearsOnRadar{});
        entity.assign<ActorTag>(ActorTag::Type::Merchandise);
      }
      break;

    case ActorID::Teleporter_1: // teleporter
    case ActorID::Teleporter_2: // teleporter
      entity.assign<AnimationLoop>(1);
      entity.assign<Interactable>(InteractableType::Teleporter);
      entity.assign<BoundingBox>(BoundingBox{{2, 0}, {2, 5}});
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Respawn_checkpoint: // respawn checkpoint
      entity.assign<BehaviorController>(interaction::RespawnCheckpoint{});
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<ActivationSettings>(ActivationSettings::Policy::Always);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Special_hint_globe: // Special hint globe
      entity.assign<Shootable>(Health{3}, GivenScore{100});
      entity.assign<DestructionEffects>(TECH_KILL_EFFECT_SPEC);
      entity.assign<AnimationSequence>(HINT_GLOBE_ANIMATION, 0, true);
      addDefaultMovingBody(entity, boundingBox);
      {
        CollectableItem item;
        item.mGivenScore = 10000;
        item.mGivenItem = InventoryItemType::SpecialHintGlobe;
        entity.assign<CollectableItem>(item);
      }
      entity.assign<AppearsOnRadar>();
      break;


    // ----------------------------------------------------------------------
    // Enemies
    // ----------------------------------------------------------------------

    case ActorID::Hoverbot: // Cylindrical robot with blinking 'head', aka hover-bot
      entity.assign<Shootable>(Health{1 + difficultyOffset}, GivenScore{150});
      addDefaultMovingBody(entity, boundingBox);
      entity.component<Sprite>()->mShow = false;
      entity.assign<ai::components::HoverBot>();
      entity.assign<DestructionEffects>(
        HOVER_BOT_KILL_EFFECT_SPEC,
        DestructionEffects::TriggerCondition::OnKilled,
        mSpriteFactory.actorFrameRect(actorID, 0));
      break;

    case ActorID::Big_green_cat_LEFT:
    case ActorID::Big_green_cat_RIGHT:
      entity.assign<Shootable>(Health{5}, GivenScore{1000});
      entity.assign<PlayerDamaging>(Damage{1});
      entity.assign<Orientation>(
        actorID == ActorID::Big_green_cat_LEFT
          ? Orientation::Left
          : Orientation::Right);
      addDefaultMovingBody(entity, boundingBox);
      entity.assign<BehaviorController>(behaviors::BigGreenCat{});
      entity.assign<DestructionEffects>(
        BIOLOGICAL_ENEMY_KILL_EFFECT_SPEC,
        DestructionEffects::TriggerCondition::OnKilled,
        mSpriteFactory.actorFrameRect(actorID, 0));
      entity.assign<AppearsOnRadar>();
      break;

    // Wall-mounted flame thrower
    case ActorID::Wall_mounted_flamethrower_RIGHT: // ->
    case ActorID::Wall_mounted_flamethrower_LEFT: // <-
      entity.assign<Shootable>(Health{12}, GivenScore{5000});
      entity.assign<PlayerDamaging>(Damage{1});
      entity.assign<Orientation>(
        actorID == ActorID::Wall_mounted_flamethrower_RIGHT
          ? Orientation::Right
          : Orientation::Left);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<BehaviorController>(behaviors::FlameThrowerBot{});
      entity.assign<DestructionEffects>(TECH_KILL_EFFECT_SPEC);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Watchbot: // Bouncing robot with big eye
      entity.assign<Shootable>(Health{6 + difficultyOffset}, GivenScore{1000});
      entity.assign<PlayerDamaging>(Damage{1});
      entity.assign<DestructionEffects>(
        SIMPLE_TECH_KILL_EFFECT_SPEC,
        DestructionEffects::TriggerCondition::OnKilled,
        mSpriteFactory.actorFrameRect(actorID, 0));
      addDefaultMovingBody(entity, boundingBox);
      entity.component<MovingBody>()->mGravityAffected = false;
      entity.assign<BehaviorController>(behaviors::WatchBot{});
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Rocket_launcher_turret: // Rocket launcher turret
      entity.assign<Shootable>(Health{3}, GivenScore{500});
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<PlayerDamaging>(Damage{1});
      entity.assign<ai::components::RocketTurret>();
      entity.assign<DestructionEffects>(
        SIMPLE_TECH_KILL_EFFECT_SPEC,
        DestructionEffects::TriggerCondition::OnKilled,
        mSpriteFactory.actorFrameRect(actorID, 0));
      entity.assign<AppearsOnRadar>();
      break;

    // Rocket turret rockets
    case ActorID::Enemy_rocket_left:
    case ActorID::Enemy_rocket_up:
    case ActorID::Enemy_rocket_right:
      entity.assign<Shootable>(Health{1}, GivenScore{10});
      entity.assign<DestructionEffects>(TECH_KILL_EFFECT_SPEC);
      entity.assign<BoundingBox>(boundingBox);
      entity.component<Sprite>()->mFramesToRender.push_back(1);
      entity.assign<AnimationLoop>(1, 1, 2, 1);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Watchbot_container_carrier: // Watch-bot container carrier
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<Shootable>(Health{5}, GivenScore{500});
      entity.assign<PlayerDamaging>(1);
      entity.assign<DestructionEffects>(TECH_KILL_EFFECT_SPEC);
      entity.assign<ActivationSettings>(
        ActivationSettings::Policy::AlwaysAfterFirstActivation);
      entity.assign<BehaviorController>(behaviors::WatchBotCarrier{});
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Bomb_dropping_spaceship: // Bomb dropping space ship
      // Not player damaging, only the bombs are
      entity.assign<Shootable>(Health{6 + difficultyOffset}, GivenScore{5000});
      entity.assign<DestructionEffects>(TECH_KILL_EFFECT_SPEC);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<AnimationLoop>(1, 1, 2, 1);
      entity.assign<ActivationSettings>(
        ActivationSettings::Policy::AlwaysAfterFirstActivation);
      entity.assign<BehaviorController>(behaviors::BomberPlane{});
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Napalm_bomb: // Big bomb
      entity.assign<Shootable>(Health{1}, GivenScore{200});
      entity.assign<PlayerDamaging>(1);
      entity.assign<AnimationSequence>(BOMB_DROPPING_ANIMATION);
      entity.assign<DestructionEffects>(
        BIG_BOMB_DETONATE_EFFECT_SPEC,
        DestructionEffects::TriggerCondition::OnCollision);
      entity.assign<BehaviorController>(behaviors::BigBomb{});
      addDefaultMovingBody(entity, boundingBox);
      engine::reassign<ActivationSettings>(
        entity, ActivationSettings{ActivationSettings::Policy::Always});
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Bouncing_spike_ball: // Bouncing spike ball
      entity.assign<Shootable>(Health{6 + difficultyOffset}, GivenScore{1000});
      entity.assign<DestructionEffects>(SPIKE_BALL_KILL_EFFECT_SPEC);
      entity.assign<PlayerDamaging>(1);
      entity.assign<BoundingBox>(boundingBox);
      ai::configureSpikeBall(entity);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Green_slime_blob: // Green slime blob
      entity.assign<Shootable>(Health{6 + difficultyOffset}, GivenScore{1500});
      entity.assign<DestructionEffects>(
        BIOLOGICAL_ENEMY_KILL_EFFECT_SPEC,
        DestructionEffects::TriggerCondition::OnKilled,
        mSpriteFactory.actorFrameRect(actorID, 0));
      entity.assign<PlayerDamaging>(Damage{1});
      entity.assign<ai::components::SlimeBlob>();
      addDefaultMovingBody(entity, boundingBox);
      entity.component<MovingBody>()->mGravityAffected = false;
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Green_slime_container: // Green slime container
      entity.assign<Shootable>(Health{1}, GivenScore{100});
      ai::configureSlimeContainer(entity);
      entity.assign<DestructionEffects>(SLIME_CONTAINER_KILL_EFFECT_SPEC);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Napalm_bomb_small: // Small bomb
      entity.assign<PlayerDamaging>(1);
      entity.assign<AnimationSequence>(BOMB_DROPPING_ANIMATION);
      entity.assign<DestructionEffects>(
        SMALL_BOMB_DETONATE_EFFECT_SPEC,
        DestructionEffects::TriggerCondition::OnCollision);
      entity.assign<BehaviorController>(behaviors::BigBomb{});
      addDefaultMovingBody(entity, boundingBox);
      engine::reassign<ActivationSettings>(
        entity, ActivationSettings{ActivationSettings::Policy::Always});
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Snake: // Snake
      // Not player damaging, but can eat duke
      // Only 1 health when Duke has been eaten
      entity.assign<Shootable>(Health{8 + difficultyOffset}, GivenScore{5000});
      entity.assign<DestructionEffects>(TECH_KILL_EFFECT_SPEC);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<ActivationSettings>(
        ActivationSettings::Policy::AlwaysAfterFirstActivation);
      entity.assign<BehaviorController>(behaviors::Snake{});
      entity.assign<Orientation>(Orientation::Left);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Camera_on_ceiling: // Security camera, ceiling-mounted
    case ActorID::Camera_on_floor: // Security camera, floor-mounted
      entity.assign<Shootable>(Health{1}, GivenScore{100});
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<BehaviorController>(behaviors::SecurityCamera{});
      entity.assign<DestructionEffects>(CAMERA_KILL_EFFECT_SPEC);
      entity.assign<ActorTag>(ActorTag::Type::ShootableCamera);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Green_hanging_suction_plant: // Green creature attached to ceiling, sucking in player
      entity.assign<Shootable>(
        Health{15 + 3 * difficultyOffset}, GivenScore{300});
      entity.assign<DestructionEffects>(
        BIOLOGICAL_ENEMY_KILL_EFFECT_SPEC,
        DestructionEffects::TriggerCondition::OnKilled,
        mSpriteFactory.actorFrameRect(actorID, 0));
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<BehaviorController>(behaviors::CeilingSucker{});
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Wall_walker: // Small eye-shaped robot, walking on wall
      entity.assign<Shootable>(Health{2}, GivenScore{100});
      entity.assign<PlayerDamaging>(1);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<BehaviorController>(
        behaviors::WallWalker{*mpRandomGenerator});
      entity.assign<DestructionEffects>(TECH_KILL_EFFECT_SPEC);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Eyeball_thrower_LEFT: // Eye-ball throwing monster
      entity.assign<Shootable>(Health{8}, GivenScore{2000});
      entity.assign<DestructionEffects>(EYE_BALL_THROWER_KILL_EFFECT_SPEC);
      entity.assign<PlayerDamaging>(1);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<Orientation>(Orientation::Left);
      entity.assign<ActivationSettings>(
        ActivationSettings::Policy::AlwaysAfterFirstActivation);
      entity.assign<BehaviorController>(behaviors::EyeballThrower{});
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Sentry_robot_generator: // hover bot generator
      entity.assign<AnimationLoop>(1, 0, 3);
      entity.assign<Shootable>(Health{20}, GivenScore{2500});
      entity.assign<DestructionEffects>(TECH_KILL_EFFECT_SPEC);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<ai::components::HoverBotSpawnMachine>();
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Skeleton: // Walking skeleton
      entity.assign<Shootable>(Health{2 + difficultyOffset}, GivenScore{100});
      entity.assign<DestructionEffects>(
        SKELETON_KILL_EFFECT_SPEC,
        DestructionEffects::TriggerCondition::OnKilled,
        mSpriteFactory.actorFrameRect(actorID, 0));
      entity.assign<PlayerDamaging>(Damage{1});
      entity.assign<ai::components::SimpleWalker>(skeletonAiConfig());
      addDefaultMovingBody(entity, boundingBox);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Metal_grabber_claw:
      entity.component<WorldPosition>()->y += 1;
      entity.assign<BoundingBox>(BoundingBox{{0, -1}, {1, 1}});
      entity.assign<Shootable>(Health{1}, GivenScore{250});
      entity.assign<ActivationSettings>(
        ActivationSettings::Policy::AlwaysAfterFirstActivation);
      entity.assign<BehaviorController>(behaviors::GrabberClaw{});
      entity.assign<DestructionEffects>(
        GRABBER_CLAW_KILL_EFFECT_SPEC,
        DestructionEffects::TriggerCondition::OnKilled,
        mSpriteFactory.actorFrameRect(actorID, 0));
      entity.assign<CustomRenderFunc>(&behaviors::GrabberClaw::render);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Hovering_laser_turret: // Floating ball, opens up and shoots lasers
      entity.assign<Shootable>(Health{3 + difficultyOffset}, GivenScore{1000});
      entity.assign<DestructionEffects>(TECH_KILL_EFFECT_SPEC);
      entity.assign<PlayerDamaging>(Damage{1});
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<ActivationSettings>(
        ActivationSettings::Policy::AlwaysAfterFirstActivation);
      entity.assign<BehaviorController>(behaviors::FloatingLaserBot{});
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Spider: // Spider
      entity.assign<Shootable>(Health{1 + difficultyOffset}, GivenScore{101});
      entity.assign<DestructionEffects>(SPIDER_KILL_EFFECT_SPEC);
      entity.assign<PlayerDamaging>(Damage{1});
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<Orientation>(Orientation::Left);
      entity.assign<MovingBody>(Velocity{0.f, 0.f}, GravityAffected{false});
      entity.assign<ActivationSettings>(
        ActivationSettings::Policy::AlwaysAfterFirstActivation);
      entity.assign<ai::components::Spider>();
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Ugly_green_bird: // green bird
      // Unclear if this is intentional or accidental, but the green bird's
      // score is equal to its y position...
      {
        const auto& position = *entity.component<WorldPosition>();
        entity.assign<Shootable>(Health{2}, GivenScore{position.y});
      }

      entity.assign<PlayerDamaging>(Damage{1});
      entity.assign<ActivationSettings>(
        ActivationSettings::Policy::AlwaysAfterFirstActivation);
      entity.assign<BehaviorController>(behaviors::GreenBird{});
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<DestructionEffects>(
        BIOLOGICAL_ENEMY_KILL_EFFECT_SPEC,
        DestructionEffects::TriggerCondition::OnKilled,
        mSpriteFactory.actorFrameRect(actorID, 0));
      entity.assign<AnimationSequence>(FLY_ANIMATION_SEQUENCE, 0, true);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Spiked_green_creature_LEFT:
    case ActorID::Spiked_green_creature_RIGHT:
      entity.assign<Shootable>(Health{5}, GivenScore{1000});
      entity.assign<PlayerDamaging>(Damage{1});
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<MovingBody>(Velocity{}, GravityAffected{false});
      entity.assign<Orientation>(actorID == ActorID::Spiked_green_creature_LEFT
        ? Orientation::Left
        : Orientation::Right);
      entity.assign<ActivationSettings>(
        ActivationSettings::Policy::AlwaysAfterFirstActivation);
      entity.assign<BehaviorController>(behaviors::SpikedGreenCreature{});
      entity.assign<DestructionEffects>(
        EXTENDED_BIOLOGICAL_ENEMY_KILL_EFFECT_SPEC,
        DestructionEffects::TriggerCondition::OnKilled,
        mSpriteFactory.actorFrameRect(actorID, 0));
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Small_flying_ship_1: // Small flying ship 1
    case ActorID::Small_flying_ship_2: // Small flying ship 2
    case ActorID::Small_flying_ship_3: // Small flying ship 3
      entity.assign<PlayerDamaging>(Damage{1});
      entity.assign<Shootable>(Health{1}, GivenScore{100});
      entity.assign<ActivationSettings>(
        ActivationSettings::Policy::AlwaysAfterFirstActivation);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<AnimationSequence>(FLY_ANIMATION_SEQUENCE, 0, true);
      entity.assign<DestructionEffects>(SMALL_FLYING_SHIP_KILL_EFFECT_SPEC);
      entity.assign<BehaviorController>(behaviors::SmallFlyingShip{});
      entity.assign<AppearsOnRadar>();
      break;

    // Guard wearing blue space suit
    case ActorID::Blue_guard_RIGHT: // ->
    case ActorID::Blue_guard_LEFT: // <-
    case ActorID::Blue_guard_using_a_terminal: // using terminal
      entity.assign<PlayerDamaging>(Damage{1});
      entity.assign<Shootable>(Health{2 + difficultyOffset}, GivenScore{3000});
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<ActivationSettings>(
        ActivationSettings::Policy::AlwaysAfterFirstActivation);
      entity.assign<ai::components::BlueGuard>(
        createBlueGuardAiComponent(actorID));
      entity.assign<DestructionEffects>(
        BLUE_GUARD_KILL_EFFECT_SPEC,
        DestructionEffects::TriggerCondition::OnKilled,
        mSpriteFactory.actorFrameRect(ActorID::Blue_guard_RIGHT, 0));
      entity.assign<AppearsOnRadar>();
      break;


    // Laser turret
    case ActorID::Laser_turret:
      // gives one point when shot with normal shot, 500 when destroyed.
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<ActorTag>(ActorTag::Type::MountedLaserTurret);
      ai::configureLaserTurret(entity, GivenScore{500});
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::BOSS_Episode_1: // Boss (episode 1)
      entity.assign<AnimationLoop>(1, 0, 1);
      entity.assign<PlayerDamaging>(Damage{1});
      entity.assign<Shootable>(
        Health{110 + 20 * difficultyOffset}, GivenScore{0});
      entity.component<Shootable>()->mDestroyWhenKilled = false;
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<MovingBody>(Velocity{}, GravityAffected{false});
      entity.assign<BehaviorController>(behaviors::BossEpisode1{});
      entity.assign<ActivationSettings>(
        ActivationSettings::Policy::AlwaysAfterFirstActivation);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Red_bird: // Red bird
      entity.assign<Shootable>(Health{1 + difficultyOffset}, GivenScore{100});
      entity.assign<DestructionEffects>(RED_BIRD_KILL_EFFECT_SPEC);
      entity.assign<PlayerDamaging>(Damage{1});
      entity.assign<BoundingBox>(boundingBox);
      configureRedBird(entity);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Smash_hammer: // Smash hammer
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<ActivationSettings>(
        ActivationSettings::Policy::AlwaysAfterFirstActivation);
      entity.assign<BehaviorController>(behaviors::SmashHammer{});
      entity.assign<CustomRenderFunc>(&behaviors::SmashHammer::render);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Unicycle_bot:
      entity.assign<Shootable>(Health{2}, GivenScore{300});
      entity.assign<PlayerDamaging>(1);
      addDefaultMovingBody(entity, boundingBox);
      entity.assign<Orientation>(Orientation::Left);
      entity.assign<BehaviorController>(behaviors::UnicycleBot{});
      entity.assign<DestructionEffects>(TECH_KILL_EFFECT_SPEC);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Aggressive_prisoner: // Monster in prison cell, aggressive
      entity.assign<ai::components::Prisoner>(true);
      entity.assign<BoundingBox>(BoundingBox{{2,0}, {3, 3}});
      entity.assign<Shootable>(Health{1}, GivenScore{500});
      entity.component<Shootable>()->mInvincible = true;
      entity.component<Shootable>()->mDestroyWhenKilled = false;
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Passive_prisoner: // Monster in prison cell, passive
      entity.assign<ai::components::Prisoner>(false);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::BOSS_Episode_4_projectile: // final boss projectile
      entity.assign<DestructionEffects>(
        BOSS4_PROJECTILE_KILL_EFFECT_SPEC,
        DestructionEffects::TriggerCondition::OnKilled,
        mSpriteFactory.actorFrameRect(actorID, 0));
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Rigelatin_soldier: // Rigelatin soldier
      entity.assign<Shootable>(
        Health{27 + 2 * difficultyOffset}, GivenScore{2100});
      entity.assign<BehaviorController>(behaviors::RigelatinSoldier{});
      entity.assign<Orientation>(Orientation::Left);
      addDefaultMovingBody(entity, boundingBox);
      entity.component<MovingBody>()->mGravityAffected = false;
      entity.assign<DestructionEffects>(
        RIGELATIN_KILL_EFFECT_SPEC,
        DestructionEffects::TriggerCondition::OnKilled,
        mSpriteFactory.actorFrameRect(ActorID::Rigelatin_soldier, 0));
      entity.assign<AppearsOnRadar>();
      break;

    // ----------------------------------------------------------------------
    // Various
    // ----------------------------------------------------------------------

    case ActorID::Nuclear_waste_can_empty: // Nuclear waste barrel, empty
      entity.assign<Shootable>(Health{1}, GivenScore{100});
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<DestructionEffects>(NUCLEAR_WASTE_BARREL_KILL_EFFECT_SPEC);
      addBarrelDestroyEffect(entity);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Nuclear_waste_can_green_slime_inside: // Nuclear waste barrel, slime inside
      {
        const auto& sprite = *entity.component<Sprite>();
        const auto numAnimationFrames = static_cast<int>(
          sprite.mpDrawData->mFrames.size());
        auto container = makeContainer(
          PlayerDamaging{Damage{1}},
          AnimationLoop{1},
          AutoDestroy::afterTimeout(numAnimationFrames),
          Active{});
        container.mStyle = ItemContainer::ReleaseStyle::NuclearWasteBarrel;
        addDefaultMovingBody(container, boundingBox);

        auto barrelSprite = createSpriteForId(ActorID::Nuclear_waste_can_empty);
        turnIntoContainer(entity, barrelSprite, 200, std::move(container));
        entity.assign<DestructionEffects>(
          NUCLEAR_WASTE_BARREL_KILL_EFFECT_SPEC);
        entity.assign<AppearsOnRadar>();
      }
      break;

    case ActorID::Electric_reactor: // Destroyable reactor
      entity.assign<Shootable>(Health{10}, GivenScore{20000});
      entity.assign<PlayerDamaging>(Damage{9}, IsFatal{true});
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<AnimationLoop>(1);
      entity.assign<DestructionEffects>(
        REACTOR_KILL_EFFECT_SPEC,
        DestructionEffects::TriggerCondition::OnKilled,
        mSpriteFactory.actorFrameRect(ActorID::Electric_reactor, 0));
      entity.assign<ActorTag>(ActorTag::Type::Reactor);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Super_force_field_LEFT: // Blue force field (disabled by cloak)
      entity.assign<PlayerDamaging>(Damage{1});
      entity.assign<Shootable>(Health{100});
      entity.component<Shootable>()->mDestroyWhenKilled = false;

      entity.assign<BoundingBox>(boundingBox);
      entity.assign<BehaviorController>(behaviors::SuperForceField{});
      entity.assign<ActivationSettings>(ActivationSettings::Policy::Always);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Missile_broken: // Missile, broken (falls over)
      {
        auto shootable = Shootable{Health{1}};
        shootable.mDestroyWhenKilled = false;

        addDefaultMovingBody(entity, boundingBox);
        entity.assign<Shootable>(shootable);
        entity.assign<DestructionEffects>(
          BROKEN_MISSILE_DETONATE_EFFECT_SPEC,
          DestructionEffects::TriggerCondition::Manual);
        entity.assign<BehaviorController>(behaviors::BrokenMissile{});
        entity.assign<AppearsOnRadar>();
      }
      break;

    case ActorID::Sliding_door_vertical: // Sliding door, vertical
      entity.assign<ai::components::VerticalSlidingDoor>();
      entity.assign<BoundingBox>(BoundingBox{{0, 0}, {1, 8}});
      entity.assign<engine::components::SolidBody>();
      entity.assign<CustomRenderFunc>(&renderVerticalSlidingDoor);
      break;

    case ActorID::Blowing_fan: // Blowing fan
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<ActivationSettings>(
        ActivationSettings::Policy::AlwaysAfterFirstActivation);
      entity.assign<BehaviorController>(behaviors::BlowingFan{});
      break;

    case ActorID::Sliding_door_horizontal: // Sliding door, horizontal
      entity.assign<ai::components::HorizontalSlidingDoor>();
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<engine::components::SolidBody>();
      break;

    case ActorID::Missile_intact: // Wall-destroying missile
      {
        auto shootable = Shootable{Health{1}};
        shootable.mDestroyWhenKilled = false;

        entity.assign<Shootable>(shootable);
        entity.assign<BoundingBox>(boundingBox);
        entity.assign<DestructionEffects>(
          MISSILE_DETONATE_EFFECT_SPEC,
          DestructionEffects::TriggerCondition::Manual);
        entity.assign<ActivationSettings>(
          ActivationSettings::Policy::AlwaysAfterFirstActivation);
        entity.assign<BehaviorController>(behaviors::Missile{});
        entity.assign<AppearsOnRadar>();
      }
      break;

    case ActorID::Rocket_elevator: // Rocket elevator
      interaction::configureElevator(entity);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Lava_pit: // Lava pool
    case ActorID::Green_acid_pit: // Slime pool
      entity.assign<PlayerDamaging>(Damage{1});
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<AnimationLoop>(1);
      break;

    case ActorID::Fire_on_floor_1: // Fire (variant 1)
    case ActorID::Fire_on_floor_2: // Fire (variant 2)
      entity.assign<PlayerDamaging>(Damage{1});
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<AnimationLoop>(1);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Slime_pipe: // Pipe dripping green stuff
      entity.assign<AnimationLoop>(1);
      entity.assign<DrawTopMost>();
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<BehaviorController>(behaviors::SlimePipe{});
      break;

    case ActorID::Floating_exit_sign_RIGHT: // floating exit sign to right
    case ActorID::Floating_exit_sign_LEFT: // floating exit sign to left
      entity.assign<Shootable>(Health{5}, GivenScore{10000});
      entity.assign<DestructionEffects>(EXIT_SIGN_KILL_EFFECT_SPEC);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<AnimationLoop>(1);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Floating_arrow: // floating arrow
      entity.assign<Shootable>(Health{5}, GivenScore{500});
      entity.assign<DestructionEffects>(FLOATING_ARROW_KILL_EFFECT_SPEC);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<AnimationLoop>(1);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Radar_dish: // Radar dish
      entity.assign<Shootable>(Health{4}, GivenScore{2000});
      entity.assign<DestructionEffects>(RADAR_DISH_KILL_EFFECT_SPEC);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<AnimationLoop>(1);
      entity.assign<components::RadarDish>();
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Radar_computer_terminal: // Radar dish computer
      entity.assign<components::RadarComputer>();
      break;

    case ActorID::Special_hint_machine: // Special hint machine
      entity.assign<Interactable>(InteractableType::HintMachine);
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Rotating_floor_spikes: // rotating floor spikes
      entity.assign<PlayerDamaging>(1);
      entity.assign<AnimationLoop>(1);
      entity.assign<AppearsOnRadar>();
      break;

    case ActorID::Computer_Terminal_Duke_Escaped: // Computer showing "Duke escaped"
    case ActorID::Lava_fall_1: // Lava fall left
    case ActorID::Lava_fall_2: // Lava fall right
    case ActorID::Water_fall_1: // Water fall left
    case ActorID::Water_fall_2: // Water fall right
    case ActorID::Water_fall_splash_left: // Water surface splash left
    case ActorID::Water_fall_splash_center: // Water surface splash center
    case ActorID::Water_fall_splash_right: // Water surface splash right
    case ActorID::Water_on_floor_1: // Shallow water (variant 1)
    case ActorID::Water_on_floor_2: // Shallow water (variant 2)
      entity.assign<AnimationLoop>(1);
      break;

    // Flying message ships
    case ActorID::Messenger_drone_1: // "Your brain is ours!"
    case ActorID::Messenger_drone_2: // "Bring back the brain! ... Please stand by"
    case ActorID::Messenger_drone_3: // "Live from Rigel it's Saturday night!"
    case ActorID::Messenger_drone_4: // "Die!"
    case ActorID::Messenger_drone_5: // "You cannot escape us! You will get your brain sucked!"
      {
        const auto typeIndex = messengerDroneTypeIndex(actorID);

        // The original game uses the actor's "score" field to store which
        // type of message is shown. The result is that the message ships will
        // give between 0 and 4 points of score, depending on their type.
        // It's unclear whether this is intentional, it seems like it might not
        // be because this score value is assigned in the update() function,
        // not when constructing the actor.
        entity.assign<Shootable>(Health{1}, GivenScore{typeIndex});
        entity.assign<DestructionEffects>(TECH_KILL_EFFECT_SPEC);
        entity.assign<BoundingBox>(boundingBox);
        entity.component<Sprite>()->mFramesToRender.clear();

        entity.assign<ai::components::MessengerDrone>(
          MESSAGE_TYPE_BY_INDEX[typeIndex]);
        entity.assign<ActivationSettings>(
          ActivationSettings::Policy::AlwaysAfterFirstActivation);
        entity.assign<AppearsOnRadar>();
      }
      break;

    case ActorID::Lava_fountain: // Lava riser
      entity.assign<BoundingBox>(BoundingBox{{0, 0}, {2, 4}});
      entity.assign<ActivationSettings>(
        ActivationSettings::Policy::AlwaysAfterFirstActivation);
      entity.assign<BehaviorController>(behaviors::LavaFountain{});
      entity.assign<CustomRenderFunc>(&behaviors::LavaFountain::render);
      break;

    case ActorID::Flame_jet_1: // Rocket exhaust flame left
    case ActorID::Flame_jet_2: // Rocket exhaust flame right
    case ActorID::Flame_jet_3: // Small rocket exhaust flame left
    case ActorID::Flame_jet_4: // Small rocket exhaust flame right
      entity.assign<AnimationLoop>(2);
      break;

    case ActorID::Exit_trigger: // level exit
      entity.assign<Trigger>(TriggerType::LevelExit);
      entity.assign<BoundingBox>(BoundingBox{{0, 0}, {1, 1}});
      break;

    case ActorID::Dynamic_geometry_2: // shootable wall, explodes into small pieces
      entity.assign<Shootable>(Health{1});
      entity.component<Shootable>()->mAlwaysConsumeInflictor = true;
      entity.component<Shootable>()->mCanBeHitWhenOffscreen = true;
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

    case ActorID::Dynamic_geometry_3: // door, opened by blue key (slides into ground)
      interaction::configureLockedDoor(entity, mSpawnIndex, boundingBox);
      break;

    case ActorID::Dynamic_geometry_1: // dynamic geometry
      {
        const auto height = boundingBox.size.height;
        entity.assign<ActivationSettings>(
          ActivationSettings::Policy::AlwaysAfterFirstActivation);
        entity.assign<BoundingBox>(BoundingBox{{-1, -(height - 1)}, {1, 1}});
        entity.assign<BehaviorController>(behaviors::DynamicGeometryController{
          DGType::FallDownAfterDelayThenSinkIntoGround});
      }
      break;

    case ActorID::Dynamic_geometry_4: // dynamic geometry
      {
        const auto height = boundingBox.size.height;
        entity.assign<BoundingBox>(BoundingBox{{-1, -(height - 1)}, {1, 1}});
        entity.assign<BehaviorController>(behaviors::DynamicGeometryController{
          DGType::FallDownWhileEarthQuakeActiveThenExplode});
      }
      break;

    case ActorID::Dynamic_geometry_5: // dynamic geometry
      {
        const auto height = boundingBox.size.height;
        entity.assign<ActivationSettings>(ActivationSettings::Policy::Always);
        entity.assign<BoundingBox>(BoundingBox{{-1, -(height - 1)}, {1, 1}});
        entity.assign<BehaviorController>(behaviors::DynamicGeometryController{
          DGType::FallDownImmediatelyThenStayOnGround});
      }
      break;

    case ActorID::Dynamic_geometry_6: // dynamic geometry
      {
        const auto height = boundingBox.size.height;
        entity.assign<ActivationSettings>(
          ActivationSettings::Policy::AlwaysAfterFirstActivation);
        entity.assign<BoundingBox>(BoundingBox{{-1, -(height - 1)}, {1, 1}});
        entity.assign<BehaviorController>(behaviors::DynamicGeometryController{
          DGType::FallDownWhileEarthQuakeActiveThenStayOnGround});
      }
      break;

    case ActorID::Dynamic_geometry_7: // dynamic geometry
      {
        const auto height = boundingBox.size.height;
        entity.assign<ActivationSettings>(ActivationSettings::Policy::Always);
        entity.assign<BoundingBox>(BoundingBox{{-1, -(height - 1)}, {1, 1}});
        entity.assign<BehaviorController>(behaviors::DynamicGeometryController{
          DGType::FallDownImmediatelyThenExplode});
      }
      break;

    case ActorID::Dynamic_geometry_8: // dynamic geometry
      {
        const auto height = boundingBox.size.height;
        entity.assign<ActivationSettings>(
          ActivationSettings::Policy::AlwaysAfterFirstActivation);
        entity.assign<BoundingBox>(BoundingBox{{-1, -(height - 1)}, {1, 1}});
        entity.assign<BehaviorController>(behaviors::DynamicGeometryController{
          DGType::FallDownAfterDelayThenStayOnGround});
      }
      break;

    case ActorID::Water_body: // water
      entity.assign<BoundingBox>(BoundingBox{{0, 1}, {2, 2}});
      entity.assign<ActorTag>(ActorTag::Type::WaterArea);
      break;

    case ActorID::Water_drop: // water drop
      addDefaultMovingBody(entity, boundingBox);
      entity.assign<AutoDestroy>(AutoDestroy{
        AutoDestroy::Condition::OnWorldCollision});
      engine::reassign<ActivationSettings>(
        entity, ActivationSettings::Policy::Always);
      break;

    case ActorID::Water_drop_spawner: // water drop spawner
      entity.assign<BehaviorController>(WaterDropGenerator{});
      entity.assign<ActivationSettings>(ActivationSettings::Policy::Always);
      entity.assign<BoundingBox>(boundingBox);
      break;

    case ActorID::Water_surface_1: // water with animated surface
      entity.assign<BoundingBox>(BoundingBox{{0, 1}, {2, 2}});
      entity.assign<ActorTag>(ActorTag::Type::AnimatedWaterArea);
      break;

    case ActorID::Water_surface_2: // water with animated surface (double sized block)
      entity.assign<BoundingBox>(BoundingBox{{0, 3}, {4, 4}});
      entity.assign<ActorTag>(ActorTag::Type::AnimatedWaterArea);
      break;

    case ActorID::Windblown_spider_generator: // windblown-spider generator
      entity.assign<BehaviorController>(WindBlownSpiderGenerator{});
      entity.assign<ActivationSettings>(ActivationSettings::Policy::Always);
      entity.assign<BoundingBox>(boundingBox);
      break;

    case ActorID::Airlock_effect_LEFT: // airlock effect, left
    case ActorID::Airlock_effect_RIGHT: // airlock effect, right
      break;

    case ActorID::Explosion_FX_trigger: // explosion effect trigger
      entity.assign<BehaviorController>(ExplosionEffect{});
      entity.assign<BoundingBox>(BoundingBox{{}, {1, 1}});
      entity.assign<DestructionEffects>(EXPLOSION_EFFECT_EFFECT_SPEC);
      break;

    // Various projectiles. Damage, velocity etc. are assigned by the
    // projectile configurarion functions
    case ActorID::Duke_rocket_up: case ActorID::Duke_rocket_down: case ActorID::Duke_rocket_left: case ActorID::Duke_rocket_right:
    case ActorID::Duke_laser_shot_horizontal: case ActorID::Duke_laser_shot_vertical: case ActorID::Duke_regular_shot_horizontal: case ActorID::Duke_regular_shot_vertical:
    case ActorID::Duke_flame_shot_up: case ActorID::Duke_flame_shot_down: case ActorID::Duke_flame_shot_left: case ActorID::Duke_flame_shot_right:
    case ActorID::Reactor_fire_LEFT: case ActorID::Reactor_fire_RIGHT:
      entity.assign<BoundingBox>(boundingBox);
      break;

    case ActorID::Enemy_laser_shot_RIGHT:
      entity.assign<BoundingBox>(boundingBox);
      entity.assign<AppearsOnRadar>();
      break;

    default:
      break;
  }

  ++mSpawnIndex;
}
