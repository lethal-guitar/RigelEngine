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

#ifndef VARS_H
#define VARS_H

#include "defs.h"


extern bool mapParallaxBoth;
extern bool mapParallaxHorizontal;
extern bool mapBackdropAutoScrollX;
extern bool mapBackdropAutoScrollY;
extern byte mapSecondaryBackdrop;
extern bool mapHasReactorDestructionEvent;
extern bool mapSwitchBackdropOnTeleport;
extern bool mapHasEarthquake;
extern byte kbLastScancode;
extern byte kbBindingUp;
extern byte kbBindingDown;
extern byte kbBindingLeft;
extern byte kbBindingRight;
extern byte kbBindingJump;
extern byte kbBindingFire;
extern bool jsCalibrated;
extern bool jsButton3;
extern bool jsButton4;
extern bool inputMoveUp;
extern bool inputMoveDown;
extern bool inputMoveLeft;
extern bool inputMoveRight;
extern bool inputJump;
extern bool inputFire;
extern far word sndCurrentMusicFileSize;
extern far bool sndMusicEnabled;
extern far word sysTicksElapsed;
extern bool gmPlayerTookDamage;
extern word gmCamerasDestroyed;
extern word gmCamerasInLevel;
extern word gmWeaponsCollected;
extern word gmWeaponsInLevel;
extern word gmMerchCollected;
extern word gmMerchInLevel;
extern word gmTurretsDestroyed;
extern word gmTurretsInLevel;
extern word gmOrbsLeft;
extern word gmBombBoxesLeft;
extern byte gmRngIndex;
extern byte gmCurrentEpisode;
extern byte gmCurrentLevel;
extern dword plScore;
extern byte plWeapon;
extern byte plWeapon_hud;
extern byte scriptPageIndex;
extern byte uiReporterTalkAnimTicksLeft;
extern word uiDemoTimeoutTime;
extern word uiMenuCursorPos;
extern word gfxCurrentDisplayPage;
extern word hudMessageCharsPrinted;
extern word hudMessageDelay;
extern word uiMessageBoxShift;
extern int uiProgressBarState;
extern byte uiProgressBarTicksElapsed;
extern byte uiProgressBarStepDelay;
extern bool hudShowingHintMachineMsg;
extern word mapBottom;
extern word mapWidthShift;
extern word mapWidth;
extern word gmCameraPosX;
extern word gmCameraPosY;
extern word plInventory[NUM_INVENTORY_SLOTS + 1];
extern bool hackStopApogeeLogo;
extern byte far* gfxLoadedSprites[MM_MAX_NUM_CHUNKS];
extern byte levelHeaderData[3002];
extern word tempTileBuffer[3000];
extern bool kbKeyState[128];
extern byte fsGroupFileDict[4000];
extern int flicFrameDelay;
extern char* fsNameForErrorReport;
extern dword sndPackageHeader[412];
extern byte sndAudioData[6069];
extern dword sysFastTicksElapsed;
extern int jsThresholdRight;
extern int jsThresholdLeft;
extern int jsThresholdDown;
extern int jsThresholdUp;
extern word gfxPaletteForFade[16 * 3];
extern byte hudInventoryBlinkTimeLeft[NUM_INVENTORY_SLOTS + 1];
extern SaveSlotName saveSlotNames[NUM_SAVE_SLOTS];
extern char gmHighScoreNames[NUM_HIGH_SCORE_ENTRIES][HIGH_SCORE_NAME_MAX_LEN + 1];
extern dword gmHighScoreList[NUM_HIGH_SCORE_ENTRIES];
extern word mmChunkSizes[MM_MAX_NUM_CHUNKS];
extern ChunkType mmChunkTypes[MM_MAX_NUM_CHUNKS];
extern byte far* mmRawMem;
extern dword mmMemTotal;
extern dword mmMemUsed;
extern word mmChunksUsed;
extern word far* psParticleData[NUM_PARTICLE_GROUPS];
extern ParticleGroup psParticleGroups[NUM_PARTICLE_GROUPS];
extern long musicNextEventTime;
extern bool musicIsPlaying;
extern int far* musicDataStart;
extern int far* musicData;
extern int musicDataLeft;
extern int musicDataSize;
extern long musicTicksElapsed;
extern InterruptHandler sysSavedTimerIntHandler;
extern char tempFilename[20];
extern int flicNextDelay;
extern bool sysIsSecondTick;

#endif
