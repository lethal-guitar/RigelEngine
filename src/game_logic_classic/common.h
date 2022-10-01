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

#ifndef COMMON_H
#define COMMON_H

#include "defs.h"


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


typedef enum {
  VT_APOGEE_LOGO = 8,
  VT_NEO_LA = 0,
  VT_RANGE_1 = 4,
  VT_RANGE_2 = 5,
  VT_RANGE_3 = 6,
  VT_UNUSED_1 = 2,
  VT_UNUSED_2 = 3,
} VideoType;


#define XY_TO_OFFSET(x, y) (x*8 + y*320)

// Convert a tile value to pixels (multiply by 8)
#define T2PX(val) (val << 3)

#define ANY_KEY_PRESSED() !(kbLastScancode & 0x80)
#define LAST_SCANCODE() (kbLastScancode & 0x7F)

#define CLEAR_SCREEN() \
  FillScreenRegion( \
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
byte pascal DN2_inportb(int address);
void pascal DN2_outportb(int address, byte value);
void pascal DN2_outport(int address, word value);

void pascal CopyStringUppercased(const char far* src, char far* dest);
bool pascal StringStartsWith(const char far* string, const char far* prefix);
word pascal DN2_strlen(char far* string);

byte RandomNumber(void);

bool MM_Init(void);
dword MM_MemAvailable(void);
void far* MM_PushChunk(word size, ChunkType type);
void pascal MM_PopChunk(ChunkType type);
void pascal MM_PopChunks(ChunkType type);

int pascal OpenFileRW(char* name);
int pascal OpenFileW(char* name);
void pascal WriteWord(word value, int fd);
word pascal ReadWord(int fd);
void pascal CloseFile(int fd);
char far* MakeFilename(char far* prefix, byte number, char far* postfix);

void LoadGroupFileDict(void);
dword pascal OpenAssetFile(const char far* name, int* pOutFd);
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
void pascal SpawnParticles(word x, word y, signed char xVelocityScale, byte color);
void pascal UpdateAndDrawParticles(void);
void pascal ClearParticles(void);

void InstallTimerInterrupt(void);
void RestoreTimerInterrupt(void);

void pascal WaitTicks(word ticks);

void pascal PlayMusic(char far* filename, void far* buffer);
void pascal StartMusicPlayback(int far* data);
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
void pascal UnfoldMessageBoxFrame(int top, int height, int width);
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
//ibool pascal IsSaveSlotEmpty(byte index);

void pascal HUD_DrawLevelNumber(word level);
void pascal HUD_DrawBackground(void);
void pascal HUD_DrawHealth(word health);
void pascal HUD_DrawLowHealthAnimation(word health);
void pascal HUD_DrawAmmo(word ammo);
void pascal HUD_DrawWeapon(int weapon);
void pascal HUD_DrawInventory(void);
void pascal GiveScore(word score);
void pascal AddInventoryItem(word item);
bool pascal RemoveFromInventory(word item);
void pascal ClearInventory(void);
void pascal HUD_UpdateInventoryAnimation(void);

void pascal SetMapSize(word width);
void pascal ParseLevelFlags(
  byte flags, byte secondaryBackdrop, byte unused1, byte unused2);
void pascal DecompressRLE(byte far* src, byte far* dest);

void ShowTextScreen(const char far* filename);
void ShowVGAScreen(const char far* filename);

bool PlayVideo(char far* filename, word videoType, int numRepetitions);


// Functions from unit2.c
int DN2_abs(int value);

void pascal PlaySound(int id);

void pascal DrawSprite(word id, word frame, word x, word y);
void pascal DrawFontSprite(word charIndex, word x, word y, word plane);

void DrawNewHighScoreEntryBackground(void);

void DrawNewsReporterTalkAnim(void);
bool pascal QueryOrToggleOption(bool toggle, byte optionId);

word Map_GetTile(word x, word y);
void Map_SetTile(word tileIndex, word x, word y);
void pascal Map_MoveSection(
  word left,
  word top,
  word right,
  word bottom,
  word distance);

void Quit(const char* quitMessage);


// Variables from unit1.c
extern byte gfxCurrentPalette[16 * 3];
extern bool jsButtonsSwapped;
extern byte far mapExtraData[8304];

#endif
