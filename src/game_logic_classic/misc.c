/* Copyright (C) 2022, Nikolai Wuttke. All rights reserved.
 *
 * This project is based on disassembly of NUKEM2.EXE from the game
 * Duke Nukem II, Copyright (C) 1993 Apogee Software, Ltd.
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

#include "actors.h"
#include "game.h"


/** Returns 1, 0, or -1 depending on val's sign/value */
int16_t Sign(int16_t val)
{
  if (val < 0)
  {
    return -1;
  }
  else if (val > 0)
  {
    return 1;
  }

  return 0;
}


void ResetGameState(Context* ctx)
{
  int16_t i;

  ctx->gmGameState = GS_RUNNING;
  ctx->gmBossActivated = false;
  ctx->plBodyExplosionStep = 0;
  ctx->plAttachedSpider1 = 0;
  ctx->plAttachedSpider2 = 0;
  ctx->plAttachedSpider3 = 0;
  ctx->plOnElevator = false;
  ctx->gfxFlashScreen = false;
  ctx->plKilledInShip = false;
  ctx->gfxCurrentDisplayPage = 1;
  ctx->gmRngIndex = 0;
  ctx->plAnimationFrame = 0;
  ctx->plState = PS_NORMAL;
  ctx->plMercyFramesLeft = INITIAL_MERCY_FRAMES;
  ctx->gmIsTeleporting = false;
  ctx->gmExplodingSectionTicksElapsed = 0;
  ctx->plInteractAnimTicks = 0;
  ctx->plBlockLookingUp = false;
  ctx->gmEarthquakeCountdown = 0;
  ctx->gmEarthquakeThreshold = 0;

  ResetEffectsAndPlayerShots(ctx);
  ClearParticles(ctx);

  if (!ctx->gmBeaconActivated)
  {
    ctx->gmPlayerTookDamage = false;

    ctx->gmNumMovingMapParts = 0;
    for (i = 0; i < MAX_NUM_MOVING_MAP_PARTS; i++)
    {
      ctx->gmMovingMapParts[i].type = 0;
    }

    ctx->gmRequestUnlockNextDoor = false;
    ctx->plAirlockDeathStep = 0;
    ctx->gmRequestUnlockNextForceField = false;
    ctx->gmRadarDishesLeft = 0;
    ctx->plCollectedLetters = 0;
    ctx->plRapidFireTimeLeft = 0;
    ctx->gmReactorDestructionStep = 0;
    ctx->bdUseSecondary = false;
    ctx->plCloakTimeLeft = 0;
    ctx->gmCamerasDestroyed = ctx->gmCamerasInLevel = 0;
    ctx->gmWeaponsCollected = ctx->gmWeaponsInLevel = 0;
    ctx->gmMerchCollected = ctx->gmMerchInLevel = 0;
    ctx->gmTurretsDestroyed = ctx->gmTurretsInLevel = 0;
    ctx->gmNumActors = 0;
    ctx->plHealth = PLAYER_MAX_HEALTH;
    ctx->gmOrbsLeft = 0;
    ctx->gmBombBoxesLeft = 0;
  }
}


void CenterViewOnPlayer(Context* ctx)
{
  ctx->gmCameraPosX = ctx->plPosX - (VIEWPORT_WIDTH / 2 - 1);

  if ((int16_t)ctx->gmCameraPosX < 0)
  {
    ctx->gmCameraPosX = 0;
  }
  else if (ctx->gmCameraPosX > ctx->mapWidth - VIEWPORT_WIDTH)
  {
    ctx->gmCameraPosX = ctx->mapWidth - VIEWPORT_WIDTH;
  }

  ctx->gmCameraPosY = ctx->plPosY - (VIEWPORT_HEIGHT - 1);

  if ((int16_t)ctx->gmCameraPosY < 0)
  {
    ctx->gmCameraPosY = 0;
  }
  else if (ctx->gmCameraPosY > ctx->mapBottom - (VIEWPORT_HEIGHT + 1))
  {
    ctx->gmCameraPosY = ctx->mapBottom - (VIEWPORT_HEIGHT + 1);
  }
}


static bool CheckDifficultyMarker(Context* ctx, word id)
{
  if (
    (id == ACT_META_MEDIUMHARD_ONLY && ctx->gmDifficulty == DIFFICULTY_EASY) ||
    (id == ACT_META_HARD_ONLY && ctx->gmDifficulty != DIFFICULTY_HARD))
  {
    return true;
  }

  return false;
}


/** Spawn actors that appear in the current level */
void SpawnLevelActors(Context* ctx)
{
  int16_t i;
  int16_t currentDrawIndex;
  int16_t drawIndex;
  word offset;
  word x;
  word y;
  word actorId;

  // The draw index is a means to make certain actors always appear in front of
  // or behind other types of actors, regardless of their position in the actor
  // list (which normally defines the order in which actors are drawn).
  //
  // The way it works is that we do multiple passes over the actor list in the
  // level file, and only spawn the actors during each pass which match the
  // draw index for that pass.
  //
  // Notably, this only works for actors that appear at the start of the level.
  // Any actors that are spawned during gameplay will be placed at wherever a
  // free slot in the actor list can be found, so their draw order is basically
  // random (it's still deterministic but depends on what has happened so far
  // during gameplay, so in practice, it very much appears to be random).
  for (currentDrawIndex = -1; currentDrawIndex < 4; currentDrawIndex++)
  {
    // ctx->levelActorListSize is the number of words, hence we multiply by 2.
    // Each actor specification is 3 words long, hence we add 6 to i on each
    // iteration.
    for (i = 0; i < ctx->levelActorListSize * 2; i += 6)
    {
      actorId = READ_LVL_ACTOR_DESC_ID(i);

      // Skip actors that don't appear in the currently chosen difficulty
      if (CheckDifficultyMarker(ctx, actorId))
      {
        i += 6;
        continue;
      }

      offset = ctx->gfxActorInfoData[actorId];

      drawIndex = AINFO_DRAW_INDEX(offset);

      if (drawIndex == currentDrawIndex)
      {
        x = READ_LVL_ACTOR_DESC_X(i);
        y = READ_LVL_ACTOR_DESC_Y(i);

        if (SpawnActorInSlot(ctx, ctx->gmNumActors, actorId, x, y))
        {
          ctx->gmNumActors++;
        }
      }
    }
  }
}
