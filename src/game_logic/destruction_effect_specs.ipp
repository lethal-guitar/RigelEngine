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

// This file is meant to be included into entity_configuration.ipp. It's only
// a separate file to make the amount of code in one file more manageable.

using EffectMovement = effects::EffectSprite::Movement;


const effects::EffectSpec HOVER_BOT_KILL_EFFECT_SPEC[] = {
  {effects::SpriteCascade{rigel::data::ActorID::Shot_impact_FX}, 0},
  {effects::EffectSprite{{0, -2}, rigel::data::ActorID::Hoverbot_debris_1, EffectMovement::FlyUp}, 0},
  {effects::EffectSprite{{}, rigel::data::ActorID::Hoverbot_debris_2, EffectMovement::FlyDown}, 0},
  {effects::Particles{{1, 0}, loader::INGAME_PALETTE[3]}, 0},
  {effects::RandomExplosionSound{}, 0}
};


const effects::EffectSpec SIMPLE_TECH_KILL_EFFECT_SPEC[] = {
  {effects::SpriteCascade{rigel::data::ActorID::Shot_impact_FX}, 0},
  {effects::Particles{{1, 0}}, 0},
  {effects::RandomExplosionSound{}, 0}
};


const effects::EffectSpec NAPALM_BOMB_KILL_EFFECT_SPEC[] = {
  {effects::RandomExplosionSound{}, 0},
  {effects::Particles{{1, 0}, loader::INGAME_PALETTE[15]}, 0}
};


const effects::EffectSpec SPIDER_KILL_EFFECT_SPEC[] = {
  {effects::EffectSprite{{-1, 1}, rigel::data::ActorID::Explosion_FX_1, EffectMovement::None}, 0},
  {effects::RandomExplosionSound{}, 0}
};


const effects::EffectSpec RED_BIRD_KILL_EFFECT_SPEC[] = {
  {effects::Particles{{}, loader::INGAME_PALETTE[5]}, 0},
  {effects::EffectSprite{{}, rigel::data::ActorID::Explosion_FX_1, EffectMovement::None}, 0},
  {effects::RandomExplosionSound{}, 0}
};


const effects::EffectSpec SKELETON_KILL_EFFECT_SPEC[] = {
  {effects::SpriteCascade{rigel::data::ActorID::Shot_impact_FX}, 0},
  {effects::RandomExplosionSound{}, 0},
  {effects::Particles{{1, 0}}, 0}
};


const effects::EffectSpec RIGELATIN_KILL_EFFECT_SPEC[] = {
  {effects::SpriteCascade{rigel::data::ActorID::Explosion_FX_1}, 0},
  {effects::Particles{{1, 0}}, 0},
  {effects::RandomExplosionSound{}, 0},
  {effects::RandomExplosionSound{}, 1},
  {effects::RandomExplosionSound{}, 3},
  {effects::RandomExplosionSound{}, 5},
  {effects::RandomExplosionSound{}, 7},
  {effects::RandomExplosionSound{}, 9},
  {effects::RandomExplosionSound{}, 11},
  {effects::RandomExplosionSound{}, 13},
  {effects::RandomExplosionSound{}, 15},
  {effects::RandomExplosionSound{}, 17},
};


const effects::EffectSpec SODA_CAN_ROCKET_KILL_EFFECT_SPEC[] = {
  {effects::RandomExplosionSound{}, 0},
  {effects::EffectSprite{{0, -1}, rigel::data::ActorID::Coke_can_debris_1, EffectMovement::FlyLeft}, 0},
  {effects::EffectSprite{{0, -1}, rigel::data::ActorID::Coke_can_debris_2, EffectMovement::FlyRight}, 0},
};


const effects::EffectSpec SODA_SIX_PACK_KILL_EFFECT_SPEC[] = {
  {effects::RandomExplosionSound{}, 0},
  {effects::EffectSprite{{0, 0}, rigel::data::ActorID::Coke_can_debris_1, EffectMovement::FlyRight}, 0},
  {effects::EffectSprite{{0, 1}, rigel::data::ActorID::Coke_can_debris_1, EffectMovement::FlyUpperRight}, 0},
  {effects::EffectSprite{{1, 0}, rigel::data::ActorID::Coke_can_debris_1, EffectMovement::FlyUp}, 0},
  {effects::EffectSprite{{0, 1}, rigel::data::ActorID::Coke_can_debris_1, EffectMovement::FlyUpperLeft}, 0},
  {effects::EffectSprite{{1, 0}, rigel::data::ActorID::Coke_can_debris_1, EffectMovement::FlyLeft}, 0},
  {effects::EffectSprite{{0, 1}, rigel::data::ActorID::Coke_can_debris_1, EffectMovement::FlyDown}, 0},
  {effects::EffectSprite{{0, 0}, rigel::data::ActorID::Coke_can_debris_2, EffectMovement::FlyRight}, 0},
  {effects::EffectSprite{{0, 1}, rigel::data::ActorID::Coke_can_debris_2, EffectMovement::FlyUpperRight}, 0},
  {effects::EffectSprite{{1, 0}, rigel::data::ActorID::Coke_can_debris_2, EffectMovement::FlyUp}, 0},
  {effects::EffectSprite{{0, 1}, rigel::data::ActorID::Coke_can_debris_2, EffectMovement::FlyUpperLeft}, 0},
  {effects::EffectSprite{{1, 0}, rigel::data::ActorID::Coke_can_debris_2, EffectMovement::FlyLeft}, 0},
  {effects::EffectSprite{{0, 1}, rigel::data::ActorID::Coke_can_debris_2, EffectMovement::FlyDown}, 0},
  {effects::ScoreNumber{{}, ScoreNumberType::S10000}, 0}
};


const effects::EffectSpec EYE_BALL_THROWER_KILL_EFFECT_SPEC[] = {
  {effects::Sound{data::SoundId::BiologicalEnemyDestroyed}, 0},
  {effects::EffectSprite{{0, -6}, rigel::data::ActorID::Eyeball_projectile, EffectMovement::FlyUp}, 0},
  {effects::EffectSprite{{0, -5}, rigel::data::ActorID::Eyeball_projectile, EffectMovement::FlyLeft}, 1},
  {effects::EffectSprite{{0, -4}, rigel::data::ActorID::Eyeball_projectile, EffectMovement::FlyRight}, 1},
  {effects::EffectSprite{{0, -3}, rigel::data::ActorID::Eyeball_projectile, EffectMovement::FlyUpperLeft}, 1},
  {effects::EffectSprite{{0, -1}, rigel::data::ActorID::Eyeball_projectile, EffectMovement::FlyUp}, 0},
  {effects::Particles{{}, loader::INGAME_PALETTE[13]}, 0}
};


const effects::EffectSpec LIVING_TURKEY_KILL_EFFECT_SPEC[] = {
  {effects::Sound{data::SoundId::BiologicalEnemyDestroyed}, 0},
  {effects::EffectSprite{{}, rigel::data::ActorID::Smoke_cloud_FX, EffectMovement::None}, 0}
};


const effects::EffectSpec BOSS4_PROJECTILE_KILL_EFFECT_SPEC[] = {
  {effects::SpriteCascade{rigel::data::ActorID::Shot_impact_FX}, 0},
  {effects::Particles{{1, 0}, loader::INGAME_PALETTE[3]}, 0},
  {effects::RandomExplosionSound{}, 0}
};


#define M_TECH_KILL_EFFECT_SPEC_DEFINITION \
  {effects::RandomExplosionSound{}, 0}, \
  {effects::EffectSprite{{}, rigel::data::ActorID::Explosion_FX_1, EffectMovement::None}, 0}, \
  {effects::RandomExplosionSound{}, 2}, \
  {effects::EffectSprite{{-1, -2}, rigel::data::ActorID::Explosion_FX_1, EffectMovement::None}, 2}, \
  {effects::RandomExplosionSound{}, 4}, \
  {effects::EffectSprite{{1, -3}, rigel::data::ActorID::Explosion_FX_1, EffectMovement::None}, 4}, \
  {effects::Particles{{}}, 0}, \
  {effects::Particles{{-1, -1}, -1}, 0}, \
  {effects::Particles{{1, -2}, 1}, 0}


const effects::EffectSpec TECH_KILL_EFFECT_SPEC[] = {
  M_TECH_KILL_EFFECT_SPEC_DEFINITION
};


const effects::EffectSpec RADAR_DISH_KILL_EFFECT_SPEC[] = {
  M_TECH_KILL_EFFECT_SPEC_DEFINITION,
  {effects::ScoreNumber{{}, ScoreNumberType::S2000}, 0}
};


const effects::EffectSpec EXIT_SIGN_KILL_EFFECT_SPEC[] = {
  M_TECH_KILL_EFFECT_SPEC_DEFINITION,
  {effects::ScoreNumber{{}, ScoreNumberType::S10000}, 0}
};


const effects::EffectSpec FLOATING_ARROW_KILL_EFFECT_SPEC[] = {
  M_TECH_KILL_EFFECT_SPEC_DEFINITION,
  {effects::ScoreNumber{{}, ScoreNumberType::S500}, 0}
};


#undef M_TECH_KILL_EFFECT_SPEC_DEFINITION


const effects::EffectSpec SPIKE_BALL_KILL_EFFECT_SPEC[] = {
  {effects::Particles{{1, 0}, loader::INGAME_PALETTE[15]}, 0},
  {effects::EffectSprite{{-1, 1}, rigel::data::ActorID::Explosion_FX_1, EffectMovement::None}, 0},
  {effects::RandomExplosionSound{}, 0}
};


// The bonus globes have one additional destruction effect, which is handled
// separately - see configureBonusGlobe() in entity_configuration.ipp
const effects::EffectSpec BONUS_GLOBE_KILL_EFFECT_SPEC[] = {
  {effects::EffectSprite{{}, rigel::data::ActorID::Bonus_globe_debris_1, EffectMovement::FlyLeft}, 0},
  {effects::EffectSprite{{}, rigel::data::ActorID::Bonus_globe_debris_2, EffectMovement::FlyRight}, 0},
  {effects::ScoreNumber{{}, ScoreNumberType::S100}, 0},
  {effects::Sound{data::SoundId::GlassBreaking}, 0},
  {effects::Particles{{1, 0}, loader::INGAME_PALETTE[15]}, 0}
};


const effects::EffectSpec BIOLOGICAL_ENEMY_KILL_EFFECT_SPEC[] = {
  {effects::SpriteCascade{rigel::data::ActorID::Shot_impact_FX}, 0},
  {effects::Particles{{1, 0}}, 0},
  {effects::Sound{data::SoundId::BiologicalEnemyDestroyed}, 0},
  {effects::EffectSprite{{1, 2}, rigel::data::ActorID::Biological_enemy_debris, EffectMovement::FlyUp}, 0},
  {effects::EffectSprite{{}, rigel::data::ActorID::Biological_enemy_debris, EffectMovement::FlyUpperRight}, 1},
  {effects::EffectSprite{{-1, 1}, rigel::data::ActorID::Biological_enemy_debris, EffectMovement::FlyUpperLeft}, 2},
  {effects::EffectSprite{{1, -1}, rigel::data::ActorID::Biological_enemy_debris, EffectMovement::FlyDown}, 3},
  {effects::EffectSprite{{-1, 2}, rigel::data::ActorID::Biological_enemy_debris, EffectMovement::FlyUpperRight}, 4},
  {effects::EffectSprite{{1, 2}, rigel::data::ActorID::Biological_enemy_debris, EffectMovement::FlyUpperLeft}, 5}
};


const effects::EffectSpec EXTENDED_BIOLOGICAL_ENEMY_KILL_EFFECT_SPEC[] = {
  {effects::SpriteCascade{rigel::data::ActorID::Shot_impact_FX}, 0},
  {effects::Particles{{1, 0}}, 0},
  {effects::Sound{data::SoundId::BiologicalEnemyDestroyed}, 0},
  {effects::EffectSprite{{1, 2}, rigel::data::ActorID::Biological_enemy_debris, EffectMovement::FlyUp}, 0},
  {effects::EffectSprite{{}, rigel::data::ActorID::Biological_enemy_debris, EffectMovement::FlyUpperRight}, 1},
  {effects::EffectSprite{{-1, 1}, rigel::data::ActorID::Biological_enemy_debris, EffectMovement::FlyUpperLeft}, 2},
  {effects::EffectSprite{{1, -1}, rigel::data::ActorID::Biological_enemy_debris, EffectMovement::FlyDown}, 3},
  {effects::EffectSprite{{-1, 2}, rigel::data::ActorID::Biological_enemy_debris, EffectMovement::FlyUpperRight}, 4},
  {effects::EffectSprite{{1, 2}, rigel::data::ActorID::Biological_enemy_debris, EffectMovement::FlyUpperLeft}, 5},

  {effects::EffectSprite{{-1, 0}, rigel::data::ActorID::Biological_enemy_debris, EffectMovement::FlyUp}, 0},
  {effects::EffectSprite{{-2, -2}, rigel::data::ActorID::Biological_enemy_debris, EffectMovement::FlyUpperRight}, 1},
  {effects::EffectSprite{{-3, -1}, rigel::data::ActorID::Biological_enemy_debris, EffectMovement::FlyUpperLeft}, 2},
  {effects::EffectSprite{{-1, -3}, rigel::data::ActorID::Biological_enemy_debris, EffectMovement::FlyDown}, 3},
  {effects::EffectSprite{{-3, 0}, rigel::data::ActorID::Biological_enemy_debris, EffectMovement::FlyUpperRight}, 4},
  {effects::EffectSprite{{-1, 0}, rigel::data::ActorID::Biological_enemy_debris, EffectMovement::FlyUpperLeft}, 5},
  {effects::EffectSprite{{3, -2}, rigel::data::ActorID::Biological_enemy_debris, EffectMovement::FlyUp}, 0},
  {effects::EffectSprite{{2, -4}, rigel::data::ActorID::Biological_enemy_debris, EffectMovement::FlyUpperRight}, 1},
  {effects::EffectSprite{{1, -3}, rigel::data::ActorID::Biological_enemy_debris, EffectMovement::FlyUpperLeft}, 2},
  {effects::EffectSprite{{3, -5}, rigel::data::ActorID::Biological_enemy_debris, EffectMovement::FlyDown}, 3},
  {effects::EffectSprite{{1, -2}, rigel::data::ActorID::Biological_enemy_debris, EffectMovement::FlyUpperRight}, 4},
  {effects::EffectSprite{{3, -2}, rigel::data::ActorID::Biological_enemy_debris, EffectMovement::FlyUpperLeft}, 5}
};


const effects::EffectSpec CAMERA_KILL_EFFECT_SPEC[] = {
  {effects::Particles{{}}, 0},
  {effects::ScoreNumber{{}, ScoreNumberType::S100}, 0},
  {effects::RandomExplosionSound{}, 0}
};


const effects::EffectSpec BLUE_GUARD_KILL_EFFECT_SPEC[] = {
  {effects::Particles{{1, 0}, loader::INGAME_PALETTE[11]}, 0},
  {effects::RandomExplosionSound{}, 0},
  {effects::SpriteCascade{rigel::data::ActorID::Shot_impact_FX}, 0}
};


const effects::EffectSpec NUCLEAR_WASTE_BARREL_KILL_EFFECT_SPEC[] = {
  {effects::RandomExplosionSound{}, 0},
  {effects::Particles{{1, 0}, loader::INGAME_PALETTE[4]}, 0},
  {effects::Particles{{1, 0}, loader::INGAME_PALETTE[15]}, 0},
  {effects::Particles{{1, 0}, loader::INGAME_PALETTE[11]}, 0},
  {effects::EffectSprite{{}, rigel::data::ActorID::Nuclear_waste_can_debris_4, EffectMovement::FlyUp}, 2},
  {effects::EffectSprite{{}, rigel::data::ActorID::Nuclear_waste_can_debris_3, EffectMovement::FlyDown}, 2},
  {effects::EffectSprite{{}, rigel::data::ActorID::Nuclear_waste_can_debris_2, EffectMovement::FlyUpperLeft}, 2},
  {effects::EffectSprite{{}, rigel::data::ActorID::Nuclear_waste_can_debris_1, EffectMovement::FlyUpperRight}, 2},
  {effects::EffectSprite{{}, rigel::data::ActorID::Smoke_cloud_FX, EffectMovement::None}, 2}
};


const effects::EffectSpec CONTAINER_BOX_KILL_EFFECT_SPEC[] = {
  {effects::RandomExplosionSound{}, 0},
  {effects::Particles{{1, 0}, loader::INGAME_PALETTE[4]}, 0},
  {effects::Particles{{1, 0}, loader::INGAME_PALETTE[15]}, 0},
  {effects::Particles{{1, 0}, loader::INGAME_PALETTE[11]}, 0},

  {effects::EffectSprite{{}, rigel::data::ActorID::Yellow_fireball_FX, EffectMovement::FlyUp}, 1},
  {effects::EffectSprite{{}, rigel::data::ActorID::Green_fireball_FX, EffectMovement::FlyUpperLeft}, 1},
  {effects::EffectSprite{{}, rigel::data::ActorID::Blue_fireball_FX, EffectMovement::FlyUpperRight}, 1},
  {effects::EffectSprite{{}, rigel::data::ActorID::Green_fireball_FX, EffectMovement::FlyDown}, 1},
};


const effects::EffectSpec SLIME_CONTAINER_KILL_EFFECT_SPEC[] = {
  {effects::Sound{data::SoundId::GlassBreaking}, 0},
  {effects::Particles{{1, 0}, loader::INGAME_PALETTE[15]}, 0}
};


const effects::EffectSpec REACTOR_KILL_EFFECT_SPEC[] = {
  {effects::SpriteCascade{rigel::data::ActorID::White_circle_flash_FX}, 0},
  {effects::EffectSprite{{}, rigel::data::ActorID::Electric_reactor, EffectMovement::None}, 0},

  // Note: The original game's binary has code to spawn 12 of these,
  // but only 7 actually show up due to the limitations on the number
  // of simultaneous effects. I've decided to show 9 out of 12, as it
  // looks good visually.
  {effects::EffectSprite{{1, -9}, rigel::data::ActorID::Shot_impact_FX, EffectMovement::FlyRight}, 0},
  {effects::EffectSprite{{1, -8}, rigel::data::ActorID::Shot_impact_FX, EffectMovement::FlyRight}, 3},
  {effects::EffectSprite{{1, -7}, rigel::data::ActorID::Shot_impact_FX, EffectMovement::FlyLeft}, 6},
  {effects::EffectSprite{{1, -6}, rigel::data::ActorID::Shot_impact_FX, EffectMovement::FlyLeft}, 9},
  {effects::EffectSprite{{1, -5}, rigel::data::ActorID::Shot_impact_FX, EffectMovement::FlyRight}, 12},
  {effects::EffectSprite{{1, -4}, rigel::data::ActorID::Shot_impact_FX, EffectMovement::FlyRight}, 15},
  {effects::EffectSprite{{1, -3}, rigel::data::ActorID::Shot_impact_FX, EffectMovement::FlyLeft}, 18},
  {effects::EffectSprite{{1, -2}, rigel::data::ActorID::Shot_impact_FX, EffectMovement::FlyLeft}, 21},
  {effects::EffectSprite{{1, -1}, rigel::data::ActorID::Shot_impact_FX, EffectMovement::FlyRight}, 24},

  // Same as above.
  {effects::EffectSprite{{-1, -9}, rigel::data::ActorID::Smoke_cloud_FX, EffectMovement::None}, 0},
  {effects::EffectSprite{{-1, -8}, rigel::data::ActorID::Smoke_cloud_FX, EffectMovement::None}, 2},
  {effects::EffectSprite{{-1, -7}, rigel::data::ActorID::Smoke_cloud_FX, EffectMovement::None}, 4},
  {effects::EffectSprite{{-1, -6}, rigel::data::ActorID::Smoke_cloud_FX, EffectMovement::None}, 6},
  {effects::EffectSprite{{-1, -5}, rigel::data::ActorID::Smoke_cloud_FX, EffectMovement::None}, 8},
  {effects::EffectSprite{{-1, -4}, rigel::data::ActorID::Smoke_cloud_FX, EffectMovement::None}, 10},
  {effects::EffectSprite{{-1, -3}, rigel::data::ActorID::Smoke_cloud_FX, EffectMovement::None}, 12},
  {effects::EffectSprite{{-1, -2}, rigel::data::ActorID::Smoke_cloud_FX, EffectMovement::None}, 14},
  {effects::EffectSprite{{-1, -1}, rigel::data::ActorID::Smoke_cloud_FX, EffectMovement::None}, 16},

  {effects::RandomExplosionSound{}, 0},
};


const effects::EffectSpec MISSILE_DETONATE_EFFECT_SPEC[] = {
  {effects::RandomExplosionSound{}, 0},
  {effects::EffectSprite{{0, -8}, rigel::data::ActorID::Missile_debris, EffectMovement::FlyLeft}, 0},
  {effects::EffectSprite{{0, -8}, rigel::data::ActorID::Missile_debris, EffectMovement::FlyUp}, 0},
  {effects::EffectSprite{{2, -8}, rigel::data::ActorID::Missile_debris, EffectMovement::FlyDown}, 1},
  {effects::EffectSprite{{2, -8}, rigel::data::ActorID::Missile_debris, EffectMovement::FlyRight}, 1},
  {effects::EffectSprite{{4, -8}, rigel::data::ActorID::Missile_debris, EffectMovement::FlyLeft}, 2},
  {effects::EffectSprite{{4, -8}, rigel::data::ActorID::Missile_debris, EffectMovement::FlyUp}, 2},
  {effects::EffectSprite{{6, -8}, rigel::data::ActorID::Missile_debris, EffectMovement::FlyDown}, 3},
  {effects::EffectSprite{{6, -8}, rigel::data::ActorID::Missile_debris, EffectMovement::FlyRight}, 3}
};


const effects::EffectSpec BROKEN_MISSILE_DETONATE_EFFECT_SPEC[] = {
  {effects::RandomExplosionSound{}, 0},
  {effects::EffectSprite{{0, 0}, rigel::data::ActorID::Missile_debris, EffectMovement::FlyUpperRight}, 0},
  {effects::EffectSprite{{2, 0}, rigel::data::ActorID::Missile_debris, EffectMovement::FlyUpperLeft}, 1},
  {effects::EffectSprite{{4, 0}, rigel::data::ActorID::Missile_debris, EffectMovement::FlyUpperRight}, 2},
  {effects::EffectSprite{{6, 0}, rigel::data::ActorID::Missile_debris, EffectMovement::FlyUpperLeft}, 3},
};


const effects::EffectSpec BIG_BOMB_DETONATE_EFFECT_SPEC[] = {
  {effects::EffectSprite{{  0, 0}, rigel::data::ActorID::Nuclear_explosion, EffectMovement::None}, 0},
  {effects::EffectSprite{{ -4, 2}, rigel::data::ActorID::Nuclear_explosion, EffectMovement::None}, 2},
  {effects::EffectSprite{{ +4, 2}, rigel::data::ActorID::Nuclear_explosion, EffectMovement::None}, 2},
  {effects::EffectSprite{{ -8, 2}, rigel::data::ActorID::Nuclear_explosion, EffectMovement::None}, 4},
  {effects::EffectSprite{{ +8, 2}, rigel::data::ActorID::Nuclear_explosion, EffectMovement::None}, 4},
  {effects::EffectSprite{{-12, 2}, rigel::data::ActorID::Nuclear_explosion, EffectMovement::None}, 6},
  {effects::EffectSprite{{+12, 2}, rigel::data::ActorID::Nuclear_explosion, EffectMovement::None}, 6},
  {effects::EffectSprite{{-16, 2}, rigel::data::ActorID::Nuclear_explosion, EffectMovement::None}, 8},
  {effects::EffectSprite{{+16, 2}, rigel::data::ActorID::Nuclear_explosion, EffectMovement::None}, 8},
};


const effects::EffectSpec SMALL_BOMB_DETONATE_EFFECT_SPEC[] = {
  {effects::EffectSprite{{0, 0}, rigel::data::ActorID::Nuclear_explosion, EffectMovement::None}, 0},
};


const effects::EffectSpec EXPLOSION_EFFECT_EFFECT_SPEC[] = {
  {effects::RandomExplosionSound{}, 0},
  {effects::EffectSprite{{0, 0}, rigel::data::ActorID::Explosion_FX_1, EffectMovement::None}, 0},
  {effects::RandomExplosionSound{}, 1},
  {effects::EffectSprite{{-1, -2}, rigel::data::ActorID::Explosion_FX_1, EffectMovement::None}, 1},
  {effects::RandomExplosionSound{}, 2},
  {effects::EffectSprite{{1, -3}, rigel::data::ActorID::Explosion_FX_1, EffectMovement::None}, 2},
};
