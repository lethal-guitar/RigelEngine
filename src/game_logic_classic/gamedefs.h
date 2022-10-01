/* Copyright (C) 2022, Nikolai Wuttke. All rights reserved.
 *
 * This project is based on disassembly of NUKEM2.EXE from the game
 * Duke Nukem II, Copyright (C) 1993 Apogee Software, Ltd.
 *
 * Some parts of the code are based on or have been adapted from the Cosmore
 * project, Copyright (c) 2020-2022 Scott Smitelli.
 * See LICENSE_Cosmore file at the root of the repository, or refer to
 * https://github.com/smitelli/cosmore/blob/master/LICENSE.
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GAMEDEFS_H
#define GAMEDEFS_H

#include "defs.h"

#define RADAR_POS_X 288
#define RADAR_POS_Y 136

#define ORIENTATION_LEFT  0
#define ORIENTATION_RIGHT 1

#define DIFFICULTY_EASY   1
#define DIFFICULTY_MEDIUM 2
#define DIFFICULTY_HARD   3

#define WPN_DAMAGE_REGULAR           1
#define WPN_DAMAGE_LASER             2
#define WPN_DAMAGE_ROCKET_LAUNCHER   8
#define WPN_DAMAGE_FLAME_THROWER     2
#define WPN_DAMAGE_SHIP_LASER        5

typedef enum {
  TUT_RAPID_FIRE,
  TUT_HEALTH_MOLECULE,
  TUT_WPN_REGULAR,
  TUT_WPN_LASER,
  TUT_WPN_FLAMETHROWER,
  TUT_WPN_ROCKETLAUNCHER,
  TUT_EARTHQUAKE,
  TUT_KEY,
  TUT_CARD,
  TUT_SHIP,
  TUT_N,
  TUT_U,
  TUT_K,
  TUT_E,
  TUT_KEY_NEEDED,
  TUT_CARD_NEEDED,
  TUT_CLOAK_NEEDED,
  TUT_RADARS_LEFT,
  TUT_HINT_MACHINE,
  TUT_ELEVATOR,
  TUT_TELEPORTER,
  TUT_LETTERS_COLLECTED,
  TUT_SODA,
  TUT_FOUND_FORCE_FIELD,
  TUT_FOUND_KEYHOLE,

  // 5 possible ID numbers are unused

  NUM_TUTORIAL_IDS = 30
} TutorialId;


typedef enum {
  PS_NORMAL,
  PS_JUMPING,
  PS_FALLING,
  PS_RECOVERING,
  PS_HANGING,
  PS_DYING,
  PS_CLIMBING_LADDER,
  PS_USING_JETPACK,
  PS_GETTING_EATEN,
  PS_USING_SHIP,
  PS_BLOWN_BY_FAN,
  PS_RIDING_ELEVATOR,
  PS_AIRLOCK_DEATH_L,
  PS_AIRLOCK_DEATH_R,
} PlayerState;


typedef enum
{
  GS_RUNNING,
  GS_EPISODE_FINISHED,
  GS_PLAYER_DIED,
  GS_LEVEL_FINISHED,
  GS_QUIT
} GameState;


typedef enum
{
  WPN_REGULAR,
  WPN_LASER,
  WPN_ROCKETLAUNCHER,
  WPN_FLAMETHROWER,
} Weapon;


typedef enum
{
  SD_UP = 7,
  SD_DOWN = 8,
  SD_LEFT = 9,
  SD_RIGHT = 10
} ShotDirection;


enum MovementDirection
{
  MD_UP,
  MD_DOWN,
  MD_LEFT,
  MD_RIGHT,
  MD_PROJECTILE
};


enum CollisionResult
{
  CR_NONE,
  CR_COLLISION,
  CR_CLIMBABLE,
  CR_LADDER
};


enum ConveyorBeltCheckResult
{
  CB_NONE,
  CB_LEFT,
  CB_RIGHT
};


// Types of effect movement patterns
#define EM_SCORE_NUMBER    100
#define EM_BURN_FX          99
#define EM_NONE             98
#define EM_RISE_UP          97
#define EM_FLY_RIGHT         0
#define EM_FLY_UPPER_RIGHT   1
#define EM_FLY_UP            2
#define EM_FLY_UPPER_LEFT    3
#define EM_FLY_LEFT          4
#define EM_FLY_DOWN          5
#define EM_BLOW_IN_WIND      6


typedef struct {
  word active;
  word id;
  word framesToLive;
  word x;
  word y;
  word type;
  word movementStep;
  word spawnDelay;
  word unk2, unk3, unk4, unk5; // Padding?
} EffectState;


// WARNING: The exact memory layout is important here. See
// UpdateAndDrawPlayerShots().
typedef struct {
  word active;
  word id;
  word numFrames;
  word x;
  word y;
  word direction;
} PlayerShot;


typedef struct
{
  word left;
  word top;
  word right;
  word bottom;
  word type;
} MovingMapPartState;


typedef enum {
  DS_INVISIBLE,  // actor is invisible, and won't collide with the
                 // player/projectiles
  DS_NORMAL,
  DS_WHITEFLASH, // used when an actor takes damage
  DS_IN_FRONT,   // actor appears in front of map foreground tiles
  DS_TRANSLUCENT // used for Duke when having the cloaking device
} DrawStyle;


typedef enum {
  TA_SOLID_TOP = 0x1,
  TA_SOLID_BOTTOM = 0x2,
  TA_SOLID_RIGHT = 0x4,
  TA_SOLID_LEFT = 0x8,
  TA_ANIMATED = 0x10,
  TA_FOREGROUND = 0x20,
  TA_FLAMMABLE = 0x40,
  TA_CLIMBABLE = 0x80,
  TA_CONVEYOR_L = 0x100,
  TA_CONVEYOR_R = 0x200,
  TA_SLOW_ANIMATION = 0x400,
  TA_LADDER = 0x4000,
} TileAttributes;


typedef void pascal (*ActorUpdateFunc)(word index);


typedef struct {
  // Actor ID. Determines which sprite is drawn for the actor. Also
  // determines the actor's collision box, which is always identical to the
  // sprite's size.
  word id;

  // Animation frame to draw for the actor's sprite
  byte frame;

  // Position on the game map, in tiles. Determines where the sprite is drawn.
  word x;
  word y;

  // Update the actor even if not on screen (by default, only on-screen actors
  // are updated)
  ibool alwaysUpdate:1;

  // Once the actor has appeared on screen, keep updating it even if it goes
  // off-screen
  ibool remainActive:1;

  // Affects ApplyWorldCollision() in game3.c. When true, the actor can move
  // past 1 tile high walls (i.e., stairs) and can move off ledges.
  ibool allowStairStepping:1;

  // Actor is affected by gravity and conveyor belts
  ibool gravityAffected:1;

  // Actor is marked as deleted, will be skipped during update and its state
  // can be reused when spawning a new actor
  ibool deleted:1;

  // Current gravity state. If gravity is enabled (gravity bit set in flags),
  // this determines how quickly the actor is falling down. See
  // UpdateAndDrawActors().
  byte gravityState;

  // How to draw the actor's sprite. When set to anything except DS_NORMAL, the
  // engine will reset it back to that on the next frame. This means you can
  // make an actor flash white for one frame by setting it to DS_WHITEFLASH, and
  // it will reset back to normal on the next frame.
  byte drawStyle;

  // How much damage the actor can take before being destroyed
  int health;

  // Actor-specific variables. What exactly these represent is up to the
  // interpretation of the behavior code, i.e. update function, damage handler
  // etc.
  word var1, var2, var3, var4, var5;

  // Used by actors that act like solid level geometry
  word far* tileBuffer;

  // score given to the player when the actor is destroyed. Not always used,
  // sometimes this is hardcoded into the damage handler code instead.
  word scoreGiven;

  ActorUpdateFunc updateFunc;
} ActorState;


#define HAS_TILE_ATTRIBUTE(tileIndex, attribute)          \
  (int)(((tileIndex) & 0x8000) ? 0 :                      \
  (gfxTilesetAttributes[(tileIndex) >> 3] & (attribute)))

#define SHAKE_SCREEN(amount) SetScreenShift(amount)

#define FLASH_SCREEN(col) { gfxFlashScreen = true; gfxScreenFlashColor = col; }


/** Play an explosion sound, randomly chosen between two variants */
#define PLAY_EXPLOSION_SOUND()                 \
  if (RandomNumber() & 1)                      \
  { PlaySound(SND_EXPLOSION); }                \
  else { PlaySound(SND_ALTERNATE_EXPLOSION); }


#define UPDATE_ANIMATION_LOOP(state, from, to) \
  {                                            \
    state->frame++;                            \
    if (state->frame == to + 1)                \
    {                                          \
      state->frame = from;                     \
    }                                          \
  }


#endif
