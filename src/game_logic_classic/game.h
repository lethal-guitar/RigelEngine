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

#pragma once

#include "types.h"


//
// Definitions
//

// Pixels
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

// Tiles
#define SCREEN_WIDTH_TILES 40
#define SCREEN_HEIGHT_TILES 25
#define VIEWPORT_WIDTH 32
#define VIEWPORT_HEIGHT 20

#define TIMER_FREQUENCY 280

#define CLOAK_TIME 700
#define RAPID_FIRE_TIME 700
#define MAX_AMMO 32
#define MAX_AMMO_FLAMETHROWER 64
#define PLAYER_MAX_HEALTH 9
#define INITIAL_MERCY_FRAMES 20
#define NUM_INVENTORY_SLOTS 6

#define MM_TOTAL_SIZE 390000
#define MM_MAX_NUM_CHUNKS 1150

#define NUM_HIGH_SCORE_ENTRIES 10
#define HIGH_SCORE_NAME_MAX_LEN 15

#define NUM_SAVE_SLOTS 8
#define SAVE_SLOT_NAME_MAX_LEN 18

#define NUM_PARTICLE_GROUPS 5
#define PARTICLES_PER_GROUP 64

#define MAX_NUM_ACTORS 448
#define MAX_NUM_EFFECTS 18
#define MAX_NUM_PLAYER_SHOTS 6
#define MAX_NUM_MOVING_MAP_PARTS 70


// Types of effect movement patterns
#define EM_SCORE_NUMBER 100
#define EM_BURN_FX 99
#define EM_NONE 98
#define EM_RISE_UP 97
#define EM_FLY_RIGHT 0
#define EM_FLY_UPPER_RIGHT 1
#define EM_FLY_UP 2
#define EM_FLY_UPPER_LEFT 3
#define EM_FLY_LEFT 4
#define EM_FLY_DOWN 5
#define EM_BLOW_IN_WIND 6

#define ORIENTATION_LEFT 0
#define ORIENTATION_RIGHT 1

#define DIFFICULTY_EASY 1
#define DIFFICULTY_MEDIUM 2
#define DIFFICULTY_HARD 3

#define WPN_DAMAGE_REGULAR 1
#define WPN_DAMAGE_LASER 2
#define WPN_DAMAGE_ROCKET_LAUNCHER 8
#define WPN_DAMAGE_FLAME_THROWER 2
#define WPN_DAMAGE_SHIP_LASER 5


//
// Enums
//

typedef enum
{
  CLR_BLACK = 0,
  CLR_DARK_GREY = 1,
  CLR_GREY = 2,
  CLR_LIGHT_GREY = 3,
  CLR_DARK_RED = 4,
  CLR_RED = 5,
  CLR_ORANGE = 6,
  CLR_YELLOW = 7,
  CLR_DARK_GREEN = 8,
  CLR_DARK_BLUE = 9,
  CLR_BLUE = 10,
  CLR_LIGHT_BLUE = 11,
  CLR_GREEN = 12,
  CLR_LIGHT_GREEN = 13,
  CLR_BROWN = 14,
  CLR_WHITE = 15
} PaletteColor;


typedef enum
{
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


typedef enum
{
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


typedef enum
{
  MID_DESTROYED_EVERYTHING,
  MID_OH_WELL,
  MID_ACCESS_GRANTED,
  MID_OPENING_DOOR,
  MID_INVINCIBLE,
  MID_HINT_GLOBE,
  MID_CLOAK_DISABLING,
  MID_RAPID_FIRE_DISABLING,
  MID_SECTOR_SECURE,
  MID_FORCE_FIELD_DESTROYED
} MessageId;


typedef enum
{
  SFC_BLACK,
  SFC_WHITE,
  SFC_YELLOW,
  SFC_BLACK2,
  SFC_DEBUG1,
  SFC_DEBUG2,
  SFC_DEBUG3
} ScreenFillColor;


typedef enum
{
  CT_COMMON,
  CT_SPRITE,
  CT_MAP_DATA,
  CT_INGAME_MUSIC,
  CT_TEMPORARY,
  CT_CZONE,
  CT_MASKED_TILES = 9,
  CT_MENU_MUSIC = 12,
  CT_INTRO_SOUND_FX = 13,
} ChunkType;


typedef enum
{
  DS_INVISIBLE, // actor is invisible, and won't collide with the
                // player/projectiles
  DS_NORMAL,
  DS_WHITEFLASH, // used when an actor takes damage
  DS_IN_FRONT, // actor appears in front of map foreground tiles
  DS_TRANSLUCENT // used for Duke when having the cloaking device
} DrawStyle;


typedef enum
{
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


//
// Structures
//

typedef struct
{
  word timeAlive;
  word x;
  word y;
  word color;
} ParticleGroup;


typedef struct
{
  word active;
  word id;
  word framesToLive;
  word x;
  word y;
  word type;
  word movementStep;
  word spawnDelay;
} EffectState;


// WARNING: The exact memory layout is important here. See
// UpdateAndDrawPlayerShots().
typedef struct
{
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


struct Context_;


typedef void (*ActorUpdateFunc)(struct Context_* ctx, word index);


typedef struct
{
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
  ibool alwaysUpdate : 1;

  // Once the actor has appeared on screen, keep updating it even if it goes
  // off-screen
  ibool remainActive : 1;

  // Affects ApplyWorldCollision() in game3.c. When true, the actor can move
  // past 1 tile high walls (i.e., stairs) and can move off ledges.
  ibool allowStairStepping : 1;

  // Actor is affected by gravity and conveyor belts
  ibool gravityAffected : 1;

  // Actor is marked as deleted, will be skipped during update and its state
  // can be reused when spawning a new actor
  ibool deleted : 1;

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
  int16_t health;

  // Actor-specific variables. What exactly these represent is up to the
  // interpretation of the behavior code, i.e. update function, damage handler
  // etc.
  word var1, var2, var3, var4, var5;

  // Used by actors that act like solid level geometry
  word* tileBuffer;

  // score given to the player when the actor is destroyed. Not always used,
  // sometimes this is hardcoded into the damage handler code instead.
  word scoreGiven;

  ActorUpdateFunc updateFunc;
} ActorState;


//
// Utility macros
//

#define HAS_TILE_ATTRIBUTE(tileIndex, attribute)                               \
  (int16_t)(                                                                   \
    ((tileIndex)&0x8000)                                                       \
      ? 0                                                                      \
      : (ctx->gfxTilesetAttributes[(tileIndex) >> 3] & (attribute)))

#define SHAKE_SCREEN(amount) SetScreenShift(ctx, amount)

#define FLASH_SCREEN(col)                                                      \
  {                                                                            \
    ctx->gfxFlashScreen = true;                                                \
    ctx->gfxScreenFlashColor = col;                                            \
  }


/** Play an explosion sound, randomly chosen between two variants */
#define PLAY_EXPLOSION_SOUND()                                                 \
  if (RandomNumber(ctx) & 1)                                                   \
  {                                                                            \
    PlaySound(ctx, SND_EXPLOSION);                                             \
  }                                                                            \
  else                                                                         \
  {                                                                            \
    PlaySound(ctx, SND_ALTERNATE_EXPLOSION);                                   \
  }


#define UPDATE_ANIMATION_LOOP(state, from, to)                                 \
  {                                                                            \
    state->frame++;                                                            \
    if (state->frame == to + 1)                                                \
    {                                                                          \
      state->frame = from;                                                     \
    }                                                                          \
  }

#define READ_LVL_HEADER_WORD(offset)                                           \
  ((*(ctx->levelHeaderData + offset + 1) << 8) |                               \
   *(ctx->levelHeaderData + offset))

// Utility macros for reading actor descriptions in the level header
//
// 45 is the offset of the first actor description's first word within the level
// header. Each actor description consists of 3 words (id, x, y).
#define READ_LVL_ACTOR_DESC_ID(index) READ_LVL_HEADER_WORD(45 + index)
#define READ_LVL_ACTOR_DESC_X(index) READ_LVL_HEADER_WORD(47 + index)
#define READ_LVL_ACTOR_DESC_Y(index) READ_LVL_HEADER_WORD(49 + index)

// Convert a tile value to pixels (multiply by 8)
#define T2PX(val) (val << 3)


//
// Global game state
//

typedef struct Context_
{
  bool sysTecMode;
  byte retConveyorBeltCheckResult;
  byte mapViewportHeight;
  bool gfxFlashScreen;
  byte gfxScreenFlashColor;
  bool gmIsTeleporting;
  int16_t plCollectedLetters;
  word gmTeleportTargetPosX;
  word gmTeleportTargetPosY;
  byte retPlayerShotDirection;
  bool gmPlayerTookDamage;
  word* gfxTilesetAttributes;
  word mapBottom;
  word mapWidthShift;
  word mapWidth;
  word gmCameraPosX;
  word gmCameraPosY;
  word* mapData;
  word gmTileDebrisStates[700 * 5];
  word* gfxActorInfoData;
  word gmNumActors;
  bool gmBossActivated;
  bool plRapidFireIsActiveFrame;
  bool gmRequestUnlockNextDoor;
  byte gmCurrentEpisode;
  byte gmCurrentLevel;
  ActorState gmActorStates[MAX_NUM_ACTORS];
  word levelActorListSize;
  word gfxCurrentDisplayPage;
  byte gmGameState;
  EffectState gmEffectStates[MAX_NUM_EFFECTS];
  PlayerShot gmPlayerShotStates[MAX_NUM_PLAYER_SHOTS];
  bool inputMoveUp;
  bool inputMoveDown;
  bool inputMoveLeft;
  bool inputMoveRight;
  bool inputJump;
  bool inputFire;
  word plRapidFireTimeLeft;
  dword plScore;
  bool mapParallaxHorizontal;
  bool mapHasReactorDestructionEvent;
  bool mapSwitchBackdropOnTeleport;
  byte gmRngIndex;
  bool plOnElevator;
  byte plAirlockDeathStep;
  byte plBodyExplosionStep;
  byte plFallingSpeed;
  byte plDeathAnimationStep;
  byte plState;
  byte plJumpStep;
  byte plMercyFramesLeft;
  word plPosX;
  word plPosY;
  word gmBeaconPosX;
  word gmBeaconPosY;
  byte plActorId;
  byte plAnimationFrame;
  bool plKilledInShip;
  word gmPlayerEatingActor;
  bool gmRequestUnlockNextForceField;
  byte plInteractAnimTicks;
  bool plBlockLookingUp;
  bool mapHasEarthquake;
  byte gmEarthquakeCountdown;
  byte gmEarthquakeThreshold;
  byte gmReactorDestructionStep;
  byte gmNumMovingMapParts;
  word plCloakTimeLeft;
  MovingMapPartState gmMovingMapParts[MAX_NUM_MOVING_MAP_PARTS];
  word gmCamerasDestroyed;
  word gmCamerasInLevel;
  word gmWeaponsCollected;
  word gmWeaponsInLevel;
  word gmMerchCollected;
  word gmMerchInLevel;
  word gmTurretsDestroyed;
  word gmTurretsInLevel;
  word gmOrbsLeft;
  word gmBombBoxesLeft;
  word plAttachedSpider1;
  word plAttachedSpider2;
  word plAttachedSpider3;
  word gmBossHealth;
  byte gmRadarDishesLeft;
  word gmCloakPickupPosX;
  word gmCloakPickupPosY;
  word gmExplodingSectionLeft;
  word gmExplodingSectionTop;
  word gmExplodingSectionRight;
  word gmExplodingSectionBottom;
  byte gmExplodingSectionTicksElapsed;
  word gmActiveFanIndex;
  bool plBlockJumping;
  bool plWalkAnimTicksDue;
  bool plBlockShooting;
  byte levelHeaderData[3002];
  word mmChunkSizes[MM_MAX_NUM_CHUNKS];
  ChunkType mmChunkTypes[MM_MAX_NUM_CHUNKS];
  dword mmMemTotal;
  dword mmMemUsed;
  word mmChunksUsed;
  word* psParticleData[NUM_PARTICLE_GROUPS];
  ParticleGroup psParticleGroups[NUM_PARTICLE_GROUPS];
  bool bdUseSecondary;
  byte gmDifficulty;
  byte plWeapon;
  byte plAmmo;
  byte plHealth;
  bool gmBeaconActivated;
  byte mmRawMem[MM_TOTAL_SIZE];
} Context;


//
// Function declarations
//

int16_t Sign(int16_t val);

byte RandomNumber(Context* ctx);

bool MM_Init(Context* ctx);
void* MM_PushChunk(Context* ctx, word size, ChunkType type);

void InitParticleSystem(Context* ctx);
void SpawnParticles(
  Context* ctx,
  word x,
  word y,
  signed char xVelocityScale,
  byte color);
void UpdateAndDrawParticles(Context* ctx);
void ClearParticles(Context* ctx);

void GiveScore(Context* ctx, word score);

void Map_DestroySection(
  Context* ctx,
  word left,
  word top,
  word right,
  word bottom);

void DamagePlayer(Context* ctx);

int16_t CheckWorldCollision(
  Context* ctx,
  word direction,
  word actorId,
  word frame,
  word x,
  word y);
bool AreSpritesTouching(
  Context* ctx,
  word id1,
  word frame1,
  word x1,
  word y1,
  word id2,
  word frame2,
  word x2,
  word y2);

int16_t ApplyWorldCollision(Context* ctx, word handle, word direction);
bool FindPlayerShotInRect(
  Context* ctx,
  word left,
  word top,
  word right,
  word bottom);

void PlaySoundIfOnScreen(Context* ctx, word handle, byte soundId);
bool IsActorOnScreen(Context* ctx, word handle);
bool IsSpriteOnScreen(Context* ctx, word id, word frame, word x, word y);
bool Boss3_IsTouchingPlayer(Context* ctx, word handle);
bool PlayerInRange(Context* ctx, word handle, word distance);

bool SpawnEffect(
  Context* ctx,
  word id,
  word x,
  word y,
  word type,
  word spawnDelay);
void SpawnDestructionEffects(
  Context* ctx,
  word handle,
  int16_t* spec,
  word actorId);
void SpawnBurnEffect(
  Context* ctx,
  word effectId,
  word sourceId,
  word x,
  word y);

void SpawnPlayerShot(Context* ctx, word id, word x, word y, word direction);

void SpawnActor(Context* ctx, word id, word x, word y);
bool SpawnActorInSlot(Context* ctx, word slot, word id, word x, word y);
void InitActorState(
  Context* ctx,
  word listIndex,
  ActorUpdateFunc updateFunc,
  word id,
  word x,
  word y,
  word alwaysUpdate,
  word remainActive,
  word allowStairStepping,
  word gravityAffected,
  word health,
  word var1,
  word var2,
  word var3,
  word var4,
  word var5,
  word scoreGiven);

void UpdatePlayer(Context* ctx);
void UpdateMovingMapParts(Context* ctx);
void UpdateAndDrawActors(Context* ctx);
void UpdateAndDrawPlayerShots(Context* ctx);
void UpdateAndDrawEffects(Context* ctx);
void UpdateAndDrawTileDebris(Context* ctx);
void UpdateAndDrawGame(Context* ctx);

byte TestShotCollision(Context* ctx, word handle);
void HandleActorShotCollision(Context* ctx, int16_t damage, word handle);

void ResetEffectsAndPlayerShots(Context* ctx);
void ResetGameState(Context* ctx);
void CenterViewOnPlayer(Context* ctx);
void SpawnLevelActors(Context* ctx);


//
// Hook functions (implemented by RigelEngine)
//

word Map_GetTile(Context* ctx, word x, word y);
void Map_SetTile(Context* ctx, word tileIndex, word x, word y);

void RaiseError(Context* ctx, const char* msg);
void ShowInGameMessage(Context* ctx, MessageId id);
void ShowLevelSpecificHint(Context* ctx);
void ShowTutorial(Context* ctx, TutorialId index);

void DrawActor(
  Context* ctx,
  word id,
  word frame,
  word x,
  word y,
  word drawStyle);
void DrawWaterArea(Context* ctx, word left, word top, word animStep);
void DrawTileDebris(Context* ctx, word tileIndex, word x, word y);
void SetPixel(Context* ctx, word x, word y, byte color);
void HUD_ShowOnRadar(Context* ctx, word x, word y);

void AddInventoryItem(Context* ctx, word item);
bool RemoveFromInventory(Context* ctx, word item);

void SetScreenShift(Context* ctx, byte amount);

void PlaySound(Context* ctx, int16_t id);
void StopMusic(Context* ctx);
