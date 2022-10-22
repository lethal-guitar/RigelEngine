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

#include "defs.h"


extern bool sysTecMode;
extern bool mapParallaxHorizontal;
extern bool mapHasReactorDestructionEvent;
extern bool mapSwitchBackdropOnTeleport;
extern bool mapHasEarthquake;
extern bool inputMoveUp;
extern bool inputMoveDown;
extern bool inputMoveLeft;
extern bool inputMoveRight;
extern bool inputJump;
extern bool inputFire;
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
extern word gfxCurrentDisplayPage;
extern word mapBottom;
extern word mapWidthShift;
extern word mapWidth;
extern word gmCameraPosX;
extern word gmCameraPosY;
extern byte levelHeaderData[3002];
extern word mmChunkSizes[MM_MAX_NUM_CHUNKS];
extern ChunkType mmChunkTypes[MM_MAX_NUM_CHUNKS];
extern dword mmMemTotal;
extern dword mmMemUsed;
extern word mmChunksUsed;
extern word far* psParticleData[NUM_PARTICLE_GROUPS];
extern ParticleGroup psParticleGroups[NUM_PARTICLE_GROUPS];
extern byte mmRawMem[MM_TOTAL_SIZE];
