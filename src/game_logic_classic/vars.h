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

#include "game.h"


extern bool sysTecMode;
extern byte retConveyorBeltCheckResult;
extern byte mapViewportHeight;
extern bool gfxFlashScreen;
extern byte gfxScreenFlashColor;
extern bool gmIsTeleporting;
extern int16_t plCollectedLetters;
extern word gmTeleportTargetPosX;
extern word gmTeleportTargetPosY;
extern byte retPlayerShotDirection;
extern bool gmPlayerTookDamage;
extern word far* gfxTilesetAttributes;
extern word mapBottom;
extern word mapWidthShift;
extern word mapWidth;
extern word gmCameraPosX;
extern word gmCameraPosY;
extern word far* mapData;
extern word far gmTileDebrisStates[700 * 5];
extern word far* gfxActorInfoData;
extern word gmNumActors;
extern bool gmBossActivated;
extern bool plRapidFireIsActiveFrame;
extern bool gmRequestUnlockNextDoor;
extern byte gmCurrentEpisode;
extern byte gmCurrentLevel;
extern ActorState gmActorStates[MAX_NUM_ACTORS];
extern word levelActorListSize;
extern word gfxCurrentDisplayPage;
extern byte gmGameState;
extern EffectState gmEffectStates[MAX_NUM_EFFECTS];
extern PlayerShot gmPlayerShotStates[MAX_NUM_PLAYER_SHOTS];
extern bool inputMoveUp;
extern bool inputMoveDown;
extern bool inputMoveLeft;
extern bool inputMoveRight;
extern bool inputJump;
extern bool inputFire;
extern word plRapidFireTimeLeft;
extern dword plScore;
extern bool mapParallaxHorizontal;
extern bool mapHasReactorDestructionEvent;
extern bool mapSwitchBackdropOnTeleport;
extern byte gmRngIndex;
extern bool plOnElevator;
extern byte plAirlockDeathStep;
extern byte plBodyExplosionStep;
extern byte plFallingSpeed;
extern byte plDeathAnimationStep;
extern byte plState;
extern byte plJumpStep;
extern byte plMercyFramesLeft;
extern word plPosX;
extern word plPosY;
extern word gmBeaconPosX;
extern word gmBeaconPosY;
extern byte plActorId;
extern byte plAnimationFrame;
extern bool plKilledInShip;
extern word gmPlayerEatingActor;
extern bool gmRequestUnlockNextForceField;
extern bool plInteractAnimTicks;
extern bool plBlockLookingUp;
extern bool mapHasEarthquake;
extern byte gmEarthquakeCountdown;
extern byte gmEarthquakeThreshold;
extern byte gmReactorDestructionStep;
extern byte gmNumMovingMapParts;
extern word plCloakTimeLeft;
extern MovingMapPartState gmMovingMapParts[MAX_NUM_MOVING_MAP_PARTS];
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
extern word plAttachedSpider1;
extern word plAttachedSpider2;
extern word plAttachedSpider3;
extern word gmBossHealth;
extern byte gmRadarDishesLeft;
extern word gmCloakPickupPosX;
extern word gmCloakPickupPosY;
extern word gmExplodingSectionLeft;
extern word gmExplodingSectionTop;
extern word gmExplodingSectionRight;
extern word gmExplodingSectionBottom;
extern byte gmExplodingSectionTicksElapsed;
extern word gmActiveFanIndex;
extern bool plBlockJumping;
extern bool plWalkAnimTicksDue;
extern bool plBlockShooting;
extern byte levelHeaderData[3002];
extern word mmChunkSizes[MM_MAX_NUM_CHUNKS];
extern ChunkType mmChunkTypes[MM_MAX_NUM_CHUNKS];
extern dword mmMemTotal;
extern dword mmMemUsed;
extern word mmChunksUsed;
extern word far* psParticleData[NUM_PARTICLE_GROUPS];
extern ParticleGroup psParticleGroups[NUM_PARTICLE_GROUPS];
extern bool bdUseSecondary;
extern byte plLadderAnimationStep;
extern byte gmDifficulty;
extern byte plWeapon;
extern byte plAmmo;
extern byte plHealth;
extern bool gmBeaconActivated;
extern byte mmRawMem[MM_TOTAL_SIZE];
