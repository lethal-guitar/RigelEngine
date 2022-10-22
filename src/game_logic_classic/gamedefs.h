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


#ifndef SHAREWARE
  #define REGISTERED_VERSION
#endif

#define RADAR_POS_X 288
#define RADAR_POS_Y 136

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


typedef void pascal (*ActorUpdateFunc)(word index);


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
  word far* tileBuffer;

  // score given to the player when the actor is destroyed. Not always used,
  // sometimes this is hardcoded into the damage handler code instead.
  word scoreGiven;

  ActorUpdateFunc updateFunc;
} ActorState;


#define HAS_TILE_ATTRIBUTE(tileIndex, attribute)                               \
  (int16_t)(                                                                   \
    ((tileIndex)&0x8000)                                                       \
      ? 0                                                                      \
      : (gfxTilesetAttributes[(tileIndex) >> 3] & (attribute)))

#define SHAKE_SCREEN(amount) SetScreenShift(amount)

#define FLASH_SCREEN(col)                                                      \
  {                                                                            \
    gfxFlashScreen = true;                                                     \
    gfxScreenFlashColor = col;                                                 \
  }


/** Play an explosion sound, randomly chosen between two variants */
#define PLAY_EXPLOSION_SOUND()                                                 \
  if (RandomNumber() & 1)                                                      \
  {                                                                            \
    PlaySound(SND_EXPLOSION);                                                  \
  }                                                                            \
  else                                                                         \
  {                                                                            \
    PlaySound(SND_ALTERNATE_EXPLOSION);                                        \
  }


#define UPDATE_ANIMATION_LOOP(state, from, to)                                 \
  {                                                                            \
    state->frame++;                                                            \
    if (state->frame == to + 1)                                                \
    {                                                                          \
      state->frame = from;                                                     \
    }                                                                          \
  }

#define LVL_TILESET_FILENAME() (levelHeaderData)
#define LVL_BACKDROP_FILENAME() (levelHeaderData + 13)
#define LVL_MUSIC_FILENAME() (levelHeaderData + 26)

#define READ_LVL_HEADER_WORD(offset)                                           \
  ((*(levelHeaderData + offset + 1) << 8) | *(levelHeaderData + offset))

// Utility macros for reading actor descriptions in the level header
//
// 45 is the offset of the first actor description's first word within the level
// header. Each actor description consists of 3 words (id, x, y).
#define READ_LVL_ACTOR_DESC_ID(index) READ_LVL_HEADER_WORD(45 + index)
#define READ_LVL_ACTOR_DESC_X(index) READ_LVL_HEADER_WORD(47 + index)
#define READ_LVL_ACTOR_DESC_Y(index) READ_LVL_HEADER_WORD(49 + index)


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
  VT_APOGEE_LOGO = 8,
  VT_NEO_LA = 0,
  VT_RANGE_1 = 4,
  VT_RANGE_2 = 5,
  VT_RANGE_3 = 6,
  VT_UNUSED_1 = 2,
  VT_UNUSED_2 = 3,
} VideoType;


#define XY_TO_OFFSET(x, y) (x * 8 + y * 320)

// Convert a tile value to pixels (multiply by 8)
#define T2PX(val) (val << 3)

#define ANY_KEY_PRESSED() !(kbLastScancode & 0x80)
#define LAST_SCANCODE() (kbLastScancode & 0x7F)

#define CLEAR_SCREEN()                                                         \
  FillScreenRegion(                                                            \
    SFC_BLACK, 0, 0, SCREEN_WIDTH_TILES - 1, SCREEN_HEIGHT_TILES - 1);


typedef char SaveSlotName[SAVE_SLOT_NAME_MAX_LEN + 1];

typedef struct
{
  word timeAlive;
  word x;
  word y;
  word color;
} ParticleGroup;


// Functions from unit1.c
byte pascal DN2_inportb(int16_t address);
void pascal DN2_outportb(int16_t address, byte value);
void pascal DN2_outport(int16_t address, word value);

void pascal CopyStringUppercased(const char far* src, char far* dest);
bool pascal StringStartsWith(const char far* string, const char far* prefix);
word pascal DN2_strlen(char far* string);

byte RandomNumber(void);

bool MM_Init(void);
dword MM_MemAvailable(void);
void far* MM_PushChunk(word size, ChunkType type);
void pascal MM_PopChunk(ChunkType type);
void pascal MM_PopChunks(ChunkType type);

int16_t pascal OpenFileRW(char* name);
int16_t pascal OpenFileW(char* name);
void pascal WriteWord(word value, int16_t fd);
word pascal ReadWord(int16_t fd);
void pascal CloseFile(int16_t fd);
char far* MakeFilename(char far* prefix, byte number, char far* postfix);

void LoadGroupFileDict(void);
dword pascal OpenAssetFile(const char far* name, int16_t* pOutFd);
word pascal GetAssetFileSize(const char far* name);
void pascal LoadAssetFile(const char far* name, void far* buffer);
void pascal LoadAssetFilePart(
  const char far* name,
  dword offset,
  void far* buffer,
  word size);

void pascal UploadTileset(byte far* data, word size, word targetOffset);

void FadeOutScreen(void);
void Duke3dTeaserFadeIn(byte step);
void FadeInScreen(void);

void pascal InitParticleSystem(void);
void pascal
  SpawnParticles(word x, word y, signed char xVelocityScale, byte color);
void pascal UpdateAndDrawParticles(void);
void pascal ClearParticles(void);

void InstallTimerInterrupt(void);
void RestoreTimerInterrupt(void);

void pascal WaitTicks(word ticks);

void pascal PlayMusic(char far* filename, void far* buffer);
void pascal StartMusicPlayback(int16_t far* data);
void StopMusic(void);
void ResetAdLibMusicChannels(void);

void pascal PollJoystick(void);
void interrupt KeyboardHandler(void);

byte pascal AwaitInput(void);
byte AwaitInputOrTimeout(word ticksToWait);
byte pascal GetTextInput(word x, word y);
void pascal ToggleCheckbox(byte index, byte* checkboxData);

word pascal FindNextToken(char far* str);
word pascal FindTokenForwards(char far* token, char far* str);
word pascal FindTokenBackwards(char far* token, char far* str);
char pascal TerminateStrAfterToken(char far* str);
char pascal TerminateStrAtEOL(char far* str);
void pascal SetUpParameterRead(char far** pText, char* outOriginalEnd);
void pascal UnterminateStr(char far* str, char newEnd);

void pascal DrawStatusIcon_1x1(word srcOffset, word x, word y);
void pascal DrawStatusIcon_1x2(word srcOffset, word x, word y);
void pascal DrawStatusIcon_2x1(word srcOffset, word x, word y);
void pascal DrawStatusIcon_2x2(word srcOffset, word x, word y);

void pascal FillScreenRegion(
  word fillTileIndex,
  word left,
  word bottom,
  word right,
  word top);

void SetScreenShift(byte amount);

void pascal DrawText(word x, word y, char far* text);
void pascal DrawFullscreenImage(char far* filename);

void pascal DrawSaveSlotNames(word selectedIndex);
void pascal DrawKeyBindings(void);
void pascal DrawCheckboxes(byte x, byte* checkboxData);
void pascal UnfoldMessageBoxFrame(int16_t top, int16_t height, int16_t width);
void ShowBonusScreen(void);
bool pascal RunSaveGameNameEntry(word index);
void pascal DrawHighScoreList(byte episode);
void pascal RunRebindKeyDialog(byte index);
bool pascal RunJoystickCalibration(void);
void pascal TryAddHighScore(byte episode);

// This function seems to have been redeclared in unit2.c, with a
// mismatching return type. The function itself returns a word-sized
// value, but calling code treats it as if returning a byte-sized one.
// So we don't declare it here, and instead declare it (with the wrong
// return type) at the top of unit2.c
//
// ibool pascal IsSaveSlotEmpty(byte index);

void pascal HUD_DrawLevelNumber(word level);
void pascal HUD_DrawBackground(void);
void pascal HUD_DrawHealth(word health);
void pascal HUD_DrawLowHealthAnimation(word health);
void pascal HUD_DrawAmmo(word ammo);
void pascal HUD_DrawWeapon(int16_t weapon);
void pascal HUD_DrawInventory(void);
void pascal GiveScore(word score);
void pascal AddInventoryItem(word item);
bool pascal RemoveFromInventory(word item);
void pascal ClearInventory(void);
void pascal HUD_UpdateInventoryAnimation(void);

void pascal SetMapSize(word width);
void pascal ParseLevelFlags(
  byte flags,
  byte secondaryBackdrop,
  byte unused1,
  byte unused2);
void pascal DecompressRLE(byte far* src, byte far* dest);

void ShowTextScreen(const char far* filename);
void ShowVGAScreen(const char far* filename);

bool PlayVideo(char far* filename, word videoType, int16_t numRepetitions);


// Functions from unit2.c
int16_t DN2_abs(int16_t value);

void pascal PlaySound(int16_t id);

void pascal DrawSprite(word id, word frame, word x, word y);
void pascal DrawFontSprite(word charIndex, word x, word y, word plane);

void DrawNewHighScoreEntryBackground(void);

void DrawNewsReporterTalkAnim(void);
bool pascal QueryOrToggleOption(bool toggle, byte optionId);

word Map_GetTile(word x, word y);
void Map_SetTile(word tileIndex, word x, word y);
void pascal
  Map_MoveSection(word left, word top, word right, word bottom, word distance);

void Quit(const char* quitMessage);


// Variables from unit1.c
extern byte gfxCurrentPalette[16 * 3];
extern bool jsButtonsSwapped;
extern byte far mapExtraData[8304];
