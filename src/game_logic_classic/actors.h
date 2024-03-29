/* Copyright (C) 2022, Nikolai Wuttke. All rights reserved.
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

#include "types.h"

// Gives meaningful names to the numerical indices used to reference
// entries in the actor info file (ACTRINFO.MNI).
// It's possible that this was generated by a tool in the original code,
// similar to the way sound IDs are generated by the Muse tool
// (see sounds.h).
typedef enum
{
  ACT_HOVERBOT = 0,
  ACT_EXPLOSION_FX_1 = 1,
  ACT_EXPLOSION_FX_2 = 2,
  ACT_FLAME_FX = 3,
  ACT_GREEN_CREATURE_EYE_FX_L = 4,
  ACT_DUKE_L = 5,
  ACT_DUKE_R = 6,
  ACT_DUKE_ROCKET_UP = 7,
  ACT_DUKE_ROCKET_DOWN = 8,
  ACT_DUKE_ROCKET_LEFT = 9,
  ACT_DUKE_ROCKET_RIGHT = 10,
  ACT_SMOKE_PUFF_FX = 11,
  ACT_HOVERBOT_DEBRIS_1 = 12,
  ACT_HOVERBOT_DEBRIS_2 = 13,
  ACT_NUCLEAR_WASTE_CAN_EMPTY = 14,
  ACT_NUCLEAR_WASTE_CAN_DEBRIS_1 = 15,
  ACT_NUCLEAR_WASTE_CAN_DEBRIS_2 = 16,
  ACT_NUCLEAR_WASTE_CAN_DEBRIS_3 = 17,
  ACT_NUCLEAR_WASTE_CAN_DEBRIS_4 = 18,
  ACT_ROCKET_LAUNCHER = 19,
  ACT_FLAME_THROWER = 20,
  ACT_DUKE_FLAME_SHOT_UP = 21,
  ACT_NORMAL_WEAPON = 22,
  ACT_LASER = 23,
  ACT_DUKE_LASER_SHOT_HORIZONTAL = 24,
  ACT_DUKE_LASER_SHOT_VERTICAL = 25,
  ACT_REGULAR_SHOT_HORIZONTAL = 26,
  ACT_REGULAR_SHOT_VERTICAL = 27,
  ACT_HEALTH_MOLECULE = 28,
  ACT_MENU_FONT_GRAYSCALE = 29,
  ACT_BIG_GREEN_CAT_L = 31,
  ACT_BIG_GREEN_CAT_R = 32,
  ACT_MUZZLE_FLASH_UP = 33,
  ACT_MUZZLE_FLASH_DOWN = 34,
  ACT_MUZZLE_FLASH_LEFT = 35,
  ACT_MUZZLE_FLASH_RIGHT = 36,
  ACT_CIRCUIT_CARD = 37,
  ACT_FLAME_THROWER_BOT_R = 38,
  ACT_FLAME_THROWER_BOT_L = 39,
  ACT_FLAME_THROWER_FIRE_R = 40,
  ACT_FLAME_THROWER_FIRE_L = 41,
  ACT_RED_BOX_BOMB = 42,
  ACT_NUCLEAR_EXPLOSION = 43,
  ACT_BONUS_GLOBE_SHELL = 44,
  ACT_BLUE_BONUS_GLOBE_1 = 45,
  ACT_BLUE_BONUS_GLOBE_2 = 46,
  ACT_BLUE_BONUS_GLOBE_3 = 47,
  ACT_BLUE_BONUS_GLOBE_4 = 48,
  ACT_WATCHBOT = 49,
  ACT_TELEPORTER_1 = 50,
  ACT_TELEPORTER_2 = 51,
  ACT_RAPID_FIRE_ICON = 52,
  ACT_RAPID_FIRE = 53,
  ACT_ROCKET_LAUNCHER_TURRET = 54,
  ACT_ENEMY_ROCKET_LEFT = 55,
  ACT_ENEMY_ROCKET_UP = 56,
  ACT_ENEMY_ROCKET_RIGHT = 57,
  ACT_WATCHBOT_CONTAINER_CARRIER = 58,
  ACT_WATCHBOT_CONTAINER = 59,
  ACT_WATCHBOT_CONTAINER_DEBRIS_1 = 60,
  ACT_WATCHBOT_CONTAINER_DEBRIS_2 = 61,
  ACT_BOMBER_PLANE = 62,
  ACT_MINI_NUKE = 63,
  ACT_BOUNCING_SPIKE_BALL = 64,
  ACT_FIRE_BOMB_FIRE = 65,
  ACT_ELECTRIC_REACTOR = 66,
  ACT_SLIME_BLOB = 67,
  ACT_SLIME_CONTAINER = 68,
  ACT_HOVERBOT_TELEPORT_FX = 69,
  ACT_SLIME_BLOB_2 = 70,
  ACT_DUKE_DEATH_PARTICLES = 71,
  ACT_BONUS_GLOBE_DEBRIS_1 = 72,
  ACT_BONUS_GLOBE_DEBRIS_2 = 73,
  ACT_WHITE_CIRCLE_FLASH_FX = 74,
  ACT_NUCLEAR_WASTE = 75,
  ACT_MINI_NUKE_SMALL = 76,
  ACT_HUD_FRAME_BACKGROUND = 77,
  ACT_SNAKE = 78,
  ACT_CAMERA_ON_CEILING = 79,
  ACT_CAMERA_ON_FLOOR = 80,
  ACT_CEILING_SUCKER = 81,
  ACT_META_MEDIUMHARD_ONLY = 82,
  ACT_META_HARD_ONLY = 83,
  ACT_SMOKE_CLOUD_FX = 84,
  ACT_REACTOR_FIRE_L = 85,
  ACT_REACTOR_FIRE_R = 86,
  ACT_DUKES_SHIP_R = 87,
  ACT_DUKES_SHIP_L = 88,
  ACT_DUKES_SHIP_AFTER_EXITING_L = 89,
  ACT_DUKES_SHIP_AFTER_EXITING_R = 90,
  ACT_DUKES_SHIP_LASER_SHOT = 91,
  ACT_DUKES_SHIP_EXHAUST_FLAMES = 92,
  ACT_SUPER_FORCE_FIELD_L = 93,
  ACT_BIOLOGICAL_ENEMY_DEBRIS = 94,
  ACT_MISSILE_BROKEN = 95,
  ACT_MISSILE_DEBRIS = 96,
  ACT_WALL_WALKER = 97,
  ACT_EYEBALL_THROWER_L = 98,
  ACT_EYEBALL_THROWER_R = 99,
  ACT_EYEBALL_PROJECTILE = 100,
  ACT_BOSS_EPISODE_2 = 101,
  ACT_DYNAMIC_GEOMETRY_1 = 102,
  ACT_META_DYNGEO_MARKER_1 = 103,
  ACT_META_DYNGEO_MARKER_2 = 104,
  ACT_DYNAMIC_GEOMETRY_2 = 106,
  ACT_MESSENGER_DRONE_BODY = 107,
  ACT_MESSENGER_DRONE_ENGINE_R = 108,
  ACT_MESSENGER_DRONE_ENGINE_L = 109,
  ACT_MESSENGER_DRONE_ENGINE_DOWN = 110,
  ACT_MESSENGER_DRONE_FLAME_DOWN = 111,
  ACT_MESSENGER_DRONE_FLAME_L = 112,
  ACT_MESSENGER_DRONE_FLAME_R = 113,
  ACT_CLOAKING_DEVICE = 114,
  ACT_HOVERBOT_GENERATOR = 115,
  ACT_DYNAMIC_GEOMETRY_3 = 116,
  ACT_SLIME_PIPE = 117,
  ACT_SLIME_DROP = 118,
  ACT_FORCE_FIELD = 119,
  ACT_CIRCUIT_CARD_KEYHOLE = 120,
  ACT_BLUE_KEY = 121,
  ACT_BLUE_KEY_KEYHOLE = 122,
  ACT_SCORE_NUMBER_FX_100 = 123,
  ACT_SCORE_NUMBER_FX_500 = 124,
  ACT_SCORE_NUMBER_FX_2000 = 125,
  ACT_SCORE_NUMBER_FX_5000 = 126,
  ACT_SCORE_NUMBER_FX_10000 = 127,
  ACT_SLIDING_DOOR_VERTICAL = 128,
  ACT_KEYHOLE_MOUNTING_POLE = 129,
  ACT_BLOWING_FAN = 130,
  ACT_LASER_TURRET = 131,
  ACT_SLIDING_DOOR_HORIZONTAL = 132,
  ACT_RESPAWN_CHECKPOINT = 133,
  ACT_SKELETON = 134,
  ACT_ENEMY_LASER_SHOT_R = 135,
  ACT_ENEMY_LASER_SHOT_L = 136,
  ACT_DYNAMIC_GEOMETRY_4 = 137,
  ACT_DYNAMIC_GEOMETRY_5 = 138,
  ACT_EXIT_TRIGGER = 139,
  ACT_LASER_TURRET_MOUNTING_POST = 140,
  ACT_DYNAMIC_GEOMETRY_6 = 141,
  ACT_DYNAMIC_GEOMETRY_7 = 142,
  ACT_DYNAMIC_GEOMETRY_8 = 143,
  ACT_MISSILE_INTACT = 144,
  ACT_PRESS_ANY_KEY = 146,
  ACT_ENEMY_LASER_MUZZLE_FLASH_L = 147,
  ACT_ENEMY_LASER_MUZZLE_FLASH_R = 148,
  ACT_MISSILE_EXHAUST_FLAME = 149,
  ACT_METAL_GRABBER_CLAW = 150,
  ACT_HOVERING_LASER_TURRET = 151,
  ACT_METAL_GRABBER_CLAW_DEBRIS_1 = 152,
  ACT_METAL_GRABBER_CLAW_DEBRIS_2 = 153,
  ACT_SPIDER = 154,
  ACT_N = 155,
  ACT_U = 156,
  ACT_K = 157,
  ACT_E = 158,
  ACT_BLUE_GUARD_R = 159,
  ACT_GAME_CARTRIDGE = 160,
  ACT_WHITE_BOX = 161,
  ACT_GREEN_BOX = 162,
  ACT_RED_BOX = 163,
  ACT_BLUE_BOX = 164,
  ACT_YELLOW_FIREBALL_FX = 165,
  ACT_GREEN_FIREBALL_FX = 166,
  ACT_BLUE_FIREBALL_FX = 167,
  ACT_SODA_CAN = 168,
  ACT_COKE_CAN_DEBRIS_1 = 169,
  ACT_COKE_CAN_DEBRIS_2 = 170,
  ACT_BLUE_GUARD_L = 171,
  ACT_SUNGLASSES = 172,
  ACT_PHONE = 173,
  ACT_SODA_6_PACK = 174,
  ACT_DUKE_3D_TEASER_TEXT = 175,
  ACT_UGLY_GREEN_BIRD = 176,
  ACT_BOOM_BOX = 181,
  ACT_DISK = 182,
  ACT_TV = 183,
  ACT_CAMERA = 184,
  ACT_PC = 185,
  ACT_CD = 186,
  ACT_M = 187,
  ACT_ROTATING_FLOOR_SPIKES = 188,
  ACT_GREEN_CREATURE_L = 189,
  ACT_GREEN_CREATURE_R = 190,
  ACT_GREEN_CREATURE_EYE_FX_R = 191,
  ACT_GREEN_CREATURE_SHELL_1_L = 192,
  ACT_GREEN_CREATURE_SHELL_2_L = 193,
  ACT_GREEN_CREATURE_SHELL_3_L = 194,
  ACT_GREEN_CREATURE_SHELL_4_L = 195,
  ACT_GREEN_CREATURE_SHELL_1_R = 196,
  ACT_GREEN_CREATURE_SHELL_2_R = 197,
  ACT_GREEN_CREATURE_SHELL_3_R = 198,
  ACT_GREEN_CREATURE_SHELL_4_R = 199,
  ACT_BOSS_EPISODE_1 = 200,
  ACT_RED_BOX_TURKEY = 201,
  ACT_TURKEY = 202,
  ACT_RED_BIRD = 203,
  ACT_DUKE_FLAME_SHOT_DOWN = 204,
  ACT_DUKE_FLAME_SHOT_LEFT = 205,
  ACT_DUKE_FLAME_SHOT_RIGHT = 206,
  ACT_FLOATING_EXIT_SIGN_R = 208,
  ACT_ELEVATOR = 209,
  ACT_COMPUTER_TERMINAL = 210,
  ACT_CLOAKING_DEVICE_ICON = 211,
  ACT_LAVA_PIT = 212,
  ACT_MESSENGER_DRONE_1 = 213,
  ACT_MESSENGER_DRONE_2 = 214,
  ACT_MESSENGER_DRONE_3 = 215,
  ACT_MESSENGER_DRONE_4 = 216,
  ACT_BLUE_GUARD_USING_TERMINAL = 217,
  ACT_SUPER_FORCE_FIELD_R = 218,
  ACT_SMASH_HAMMER = 219,
  ACT_MESSENGER_DRONE_5 = 220,
  ACT_WATER_BODY = 221,
  ACT_LAVA_FALL_1 = 222,
  ACT_LAVA_FALL_2 = 223,
  ACT_WATER_FALL_1 = 224,
  ACT_WATER_FALL_2 = 225,
  ACT_WATER_DROP = 226,
  ACT_WATER_DROP_SPAWNER = 227,
  ACT_WATER_FALL_SPLASH_L = 228,
  ACT_WATER_FALL_SPLASH_CENTER = 229,
  ACT_WATER_FALL_SPLASH_R = 230,
  ACT_LAVA_FOUNTAIN = 231,
  ACT_SPIDER_SHAKEN_OFF = 232,
  ACT_WATER_SURFACE = 233,
  ACT_WATER_AREA_4x4 = 234,
  ACT_GREEN_ACID_PIT = 235,
  ACT_RADAR_DISH = 236,
  ACT_RADAR_COMPUTER_TERMINAL = 237,
  ACT_SPECIAL_HINT_GLOBE_ICON = 238,
  ACT_SPECIAL_HINT_GLOBE = 239,
  ACT_SPECIAL_HINT_MACHINE = 240,
  ACT_WINDBLOWN_SPIDER_GENERATOR = 241,
  ACT_SPIDER_DEBRIS_2 = 242,
  ACT_SPIDER_BLOWING_IN_WIND = 243,
  ACT_UNICYCLE_BOT = 244,
  ACT_FLAME_JET_1 = 246,
  ACT_FLAME_JET_2 = 247,
  ACT_FLAME_JET_3 = 248,
  ACT_FLAME_JET_4 = 249,
  ACT_AIRLOCK_DEATH_TRIGGER_L = 250,
  ACT_AIRLOCK_DEATH_TRIGGER_R = 251,
  ACT_FLOATING_EXIT_SIGN_L = 252,
  ACT_AGGRESSIVE_PRISONER = 253,
  ACT_EXPLOSION_FX_TRIGGER = 254,
  ACT_PRISONER_HAND_DEBRIS = 255,
  ACT_ENEMY_ROCKET_2_UP = 256,
  ACT_WATER_ON_FLOOR_1 = 257,
  ACT_WATER_ON_FLOOR_2 = 258,
  ACT_ENEMY_ROCKET_2_DOWN = 259,
  ACT_BLOWING_FAN_THREADS_ON_TOP = 260,
  ACT_PASSIVE_PRISONER = 261,
  ACT_FIRE_ON_FLOOR_1 = 262,
  ACT_FIRE_ON_FLOOR_2 = 263,
  ACT_BOSS_EPISODE_3 = 265,
  ACT_SMALL_FLYING_SHIP_1 = 271,
  ACT_SMALL_FLYING_SHIP_2 = 272,
  ACT_SMALL_FLYING_SHIP_3 = 273,
  ACT_T_SHIRT = 274,
  ACT_VIDEOCASSETTE = 275,
  ACT_BOSS_EPISODE_4 = 279,
  ACT_BOSS_EPISODE_4_SHOT = 280,
  ACT_LETTER_INDICATOR_N = 290,
  ACT_LETTER_INDICATOR_U = 291,
  ACT_LETTER_INDICATOR_K = 292,
  ACT_LETTER_INDICATOR_E = 293,
  ACT_LETTER_INDICATOR_M = 294,
  ACT_FLOATING_ARROW = 296,
  ACT_NEWS_REPORTER_BABBLE = 297,
  ACT_RIGELATIN_SOLDIER = 299,
  ACT_RIGELATIN_SOLDIER_SHOT = 300
} ActorId;


/*
A few helper macros to access different parts of an actor info entry.

[NOTE] It would be more straightforward and also more efficient
to define a struct type and cast the actor info data to that type:

typedef struct
{
  word numFrames;
  int16_t drawIndex;
  int16_t drawOffsetX;
  int16_t drawOffsetY;
  word height;
  word width;
  dword dataFileOffset;
} ActorInfoEntry;

word offset = gfxActorInfoData[actorId];
ActorInfoEntry* info = (ActorInfoEntry*)(gfxActorInfoData + offset);

But instead, the game retrieves the data fields of each entry as individual
words. We don't know what the actual code looked like, but it's quite likely
that some sort of macros were used to make things a little easier.
*/
#define AINFO_NUM_FRAMES(offset) *(ctx->gfxActorInfoData + offset)
#define AINFO_DRAW_INDEX(offset)                                               \
  (int16_t) * (ctx->gfxActorInfoData + offset + 1)
#define AINFO_X_OFFSET(offset) (int16_t) * (ctx->gfxActorInfoData + offset + 2)
#define AINFO_Y_OFFSET(offset) (int16_t) * (ctx->gfxActorInfoData + offset + 3)
#define AINFO_HEIGHT(offset) *(ctx->gfxActorInfoData + offset + 4)
#define AINFO_WIDTH(offset) *(ctx->gfxActorInfoData + offset + 5)
#define AINFO_DATA_OFFSET(offset)                                              \
  (((dword) * (ctx->gfxActorInfoData + offset + 7) << 16) +                    \
   (dword) * (ctx->gfxActorInfoData + offset + 6))
