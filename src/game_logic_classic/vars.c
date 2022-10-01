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


/*******************************************************************************

This file contains all global variables without an initial value, which are
stored in the BSS section of the executable. We don't know how these variables
were declared in the original code, they might have been spread across both
translation units, interspersed between function definitions, etc.

But unfortunately, the linker changes the order of some variables if I declare
them across mulitple translation units, based on rules that are as of now still
unclear to me. Sticking all variables into one translation unit makes it easy
to exactly recreate the original memory layout of these variables, which I
wasn't able to achieve otherwise.

It's also possible that some of these variables are actually function-local
static variables, but again, that would make it harder to match the memory
layout, so I made all of them globals.

*******************************************************************************/


word uiMessageBoxShift;
int uiProgressBarState;
word uiProgressBarTicksElapsed;
word uiProgressBarStepDelay;

byte demoData[134];

byte retConveyorBeltCheckResult;

bool hudShowingHintMachineMsg;

// [NOTE] This has a constant value in the shipping game, but it might
// have been variable at some point during development - otherwise, the code
// would most likely not use a run-time variable.
byte mapViewportHeight;

bool gfxFlashScreen;
byte gfxScreenFlashColor;

byte debugSelectedFunction;
byte debugLevelToWarpTo;

bool gmIsTeleporting;
int plCollectedLetters;
word gmTeleportTargetPosX;
word gmTeleportTargetPosY;

byte retPlayerShotDirection;

bool gmPlayerTookDamage;

InterruptHandler kbSavedIntHandler;

void far* sndInGameMusicBuffer;
void far* sndMenuMusicBuffer;

word far* bdOffsetTable;
word far* gfxTilesetAttributes;

word mapBottom;
word mapWidthShift;
word mapWidth;

word gmCameraPosX;
word gmCameraPosY;

word far* mapData;

word far gmTileDebrisStates[700 * 5];

word far* gfxActorInfoData;

word sysTicksElapsed;

word gmNumActors;

byte scriptPageIndex;

bool gmBossActivated;

bool jsButton3;
bool jsButton4;

word bdAutoScrollStep;
word bdAddress;

// These variables are only used within UpdateAndDrawGame().
word destOffset;
word srcOffsetEnd;
word far* pCurrentTile;

word far* bdOffsetTablePtr;

byte far* gfxMaskedTileData;

bool plRapidFireIsActiveFrame;

bool gmRequestUnlockNextDoor;

bool demoIsRecording;

byte kbLastScancode;

bool sndMusicEnabled;
bool sndSoundEnabled;
byte gmSpeedIndex;
bool sndUseSbSounds;
bool sndUseAdLibSounds;
bool sndUsePcSpeakerSounds;

bool uiDisplayPageChanged;

// There are only 6 slots in the inventory. The 7th array element serves as a
// sentinel value, to indicate the end of the inventory in case the player
// has 6 items.
word plInventory[NUM_INVENTORY_SLOTS + 1];

byte gmCurrentEpisode;
byte gmCurrentLevel;

// [NOTE] The game only uses 448 out of the 450 available slots.
// It's theoretically possible that this array only holds 448 elements,
// and is followed by 64 bytes of unused variables, but that seems
// unlikely.
ActorState gmActorStates[MAX_NUM_ACTORS + 2];

word levelActorListSize;

word gfxCurrentDisplayPage;

byte gmGameState;

byte unused1;

EffectState gmEffectStates[MAX_NUM_EFFECTS];
PlayerShot gmPlayerShotStates[MAX_NUM_PLAYER_SHOTS];

bool inputMoveUp;
bool inputMoveDown;
bool inputMoveLeft;
bool inputMoveRight;
bool inputJump;
bool inputFire;
byte kbBindingUp;
byte kbBindingDown;
byte kbBindingLeft;
byte kbBindingRight;
byte kbBindingJump;
byte kbBindingFire;

word plRapidFireTimeLeft;

dword plScore;

bool jsCalibrated;

bool mapParallaxBoth;
bool mapParallaxHorizontal;
bool mapBackdropAutoScrollX;
bool mapBackdropAutoScrollY;
byte mapSecondaryBackdrop;
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

byte plWeapon_hud;

char* hudCurrentMessage;

bool gmTutorialsShown[NUM_TUTORIAL_IDS];

byte gmReactorDestructionStep;
byte gmNumMovingMapParts;
word plCloakTimeLeft;

MovingMapPartState gmMovingMapParts[MAX_NUM_MOVING_MAP_PARTS];

byte far* sndDigitizedSounds[50];

word bdAddressAdjust;

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

word hudMessageCharsPrinted;
word hudMessageDelay;

bool gmWaterAreasPresent;

word plAttachedSpider1;
word plAttachedSpider2;
word plAttachedSpider3;

word gmBossHealth;

byte gmRadarDishesLeft;

char uiHintMessageBuffer[128];

word sndCurrentMusicFileSize;
byte sndCurrentPriority;
byte sndCurrentPriorityFallback;

dword unused2;

word gmCloakPickupPosX;
word gmCloakPickupPosY;

int far* sndPreBossMusicData;
word sndOriginalMusicSize;

word gmExplodingSectionLeft;
word gmExplodingSectionTop;
word gmExplodingSectionRight;
word gmExplodingSectionBottom;
byte gmExplodingSectionTicksElapsed;

word gmActiveFanIndex;

bool demoPlaybackAborted;

byte uiReporterTalkAnimTicksLeft;
word uiDemoTimeoutTime;
word uiMenuCursorPos;
byte uiMenuState;
byte uiCurrentMenuId;

int demoFileFd;
word demoFramesProcessed;
bool demoIsPlaying;

bool plBlockJumping;
bool plWalkAnimTicksDue;
bool plBlockShooting;

bool unused3;

bool hackStopApogeeLogo;

byte far* gfxLoadedSprites[MM_MAX_NUM_CHUNKS];

byte levelHeaderData[3002];

word tempTileBuffer[3000];

bool kbKeyState[128];

byte fsGroupFileDict[4000];

char* fsNameForErrorReport;

int flicFrameDelay;

// [PERF] only 412 bytes are needed, but the array is of type dword and thus
// uses 412*4 = 1648 bytes.  Most likely done by accident, maybe the type was
// initially byte and was then changed to dword without adjusting the size.
dword sndPackageHeader[412];

byte sndAudioData[6069];

dword sysFastTicksElapsed;

int jsThresholdRight;
int jsThresholdLeft;
int jsThresholdDown;
int jsThresholdUp;

word gfxPaletteForFade[16 * 3];

// See plInventory above
byte hudInventoryBlinkTimeLeft[NUM_INVENTORY_SLOTS + 1];

SaveSlotName saveSlotNames[NUM_SAVE_SLOTS];

// +1 for zero terminator
char gmHighScoreNames[NUM_HIGH_SCORE_ENTRIES][HIGH_SCORE_NAME_MAX_LEN + 1];

dword gmHighScoreList[NUM_HIGH_SCORE_ENTRIES];

word mmChunkSizes[MM_MAX_NUM_CHUNKS];
ChunkType mmChunkTypes[MM_MAX_NUM_CHUNKS];
byte far* mmRawMem;
dword mmMemTotal;
dword mmMemUsed;
word mmChunksUsed;

word far* psParticleData[NUM_PARTICLE_GROUPS];
ParticleGroup psParticleGroups[NUM_PARTICLE_GROUPS];

long musicTicksElapsed;
bool musicIsPlaying;
int far* musicDataStart;
int far* musicData;
int musicDataLeft;
int musicDataSize;
long musicNextEventTime;
InterruptHandler sysSavedTimerIntHandler;

char tempFilename[20];
int flicNextDelay;
bool sysIsSecondTick;
