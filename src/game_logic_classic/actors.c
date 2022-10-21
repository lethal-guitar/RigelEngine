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

#include "base/warnings.hpp"

#include "actors.h"
#include "game.h"
#include "sounds.h"


/*******************************************************************************

Actor control logic and actor property definitions

This file contains the behavior control functions for all actors, and a huge
switch statement which assigns these functions to actors based on their IDs
as well as defining various properties like amount of health, score given
when destroyed, etc.

This represents the largest part of the game logic by far.

*******************************************************************************/

RIGEL_DISABLE_CLASSIC_CODE_WARNINGS

/** Semi-generic utility actor
 *
 * This function is used for a variety of actors which feature animated sprites
 * but don't need any other behavior otherwise.
 *
 * In general, the animation repeats from frame 0 to the value of var1,
 * advancing by one animation frame each game frame. But there are also a few
 * special cases for specific types of actors.
 */
static void Act_AnimatedProp(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  if (state->id == ACT_PASSIVE_PRISONER)
  {
    state->frame = !((word)RandomNumber(ctx) & 4);
  }
  else if (state->id == ACT_SPECIAL_HINT_GLOBE)
  {
    // [PERF] Missing `static` causes a copy operation here
    const byte HINT_GLOBE_ANIMATION[] = {0, 1, 2, 3, 4, 5, 4, 5, 4, 5, 4, 5, 4,
                                         3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    state->frame = HINT_GLOBE_ANIMATION[state->var1];

    state->var1++;
    if (state->var1 == sizeof(HINT_GLOBE_ANIMATION))
    {
      state->var1 = 0;
    }
  }
  else
  {
    if (
      state->id == ACT_WATER_ON_FLOOR_1 || state->id == ACT_WATER_ON_FLOOR_2 ||
      state->id == ACT_ROTATING_FLOOR_SPIKES)
    {
      // Advance one frame every other game frame (half speed)
      state->frame = (byte)(state->frame + ctx->gfxCurrentDisplayPage);
    }
    else
    {
      // Advance one frame every game frame (full speed)
      state->frame++;
    }

    if (state->frame == state->var1)
    {
      state->frame = 0;
    }
  }
}


static void Act_Hoverbot(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  if (state->var5) // wait before starting to move (after teleport animation)
  {
    state->var5--;

    // Draw the eye
    DrawActor(
      ctx, ACT_HOVERBOT, state->var4 + 6, state->x, state->y, DS_NORMAL);
  }
  else if (state->var2 <= 9 && state->var2 > 1) // teleport animation
  {
    if (state->var2 == 8)
    {
      // The effect is player-damaging
      SpawnEffect(
        ctx, ACT_HOVERBOT_TELEPORT_FX, state->x, state->y, EM_NONE, 0);
    }

    state->var2--;
    if (state->var2 == 1)
    {
      // Switch to the initial wait state
      state->var5 = 10;
    }

    state->drawStyle = DS_INVISIBLE;
    return;
  }

  // Animate the body
  UPDATE_ANIMATION_LOOP(state, 0, 5);

  if (state->var5)
  {
    return;
  } // if in initial wait state, we're done here

  if (state->var3 == 0) // moving
  {
    if (state->var1) // moving right
    {
      state->x++;
      ApplyWorldCollision(ctx, handle, MD_RIGHT);

      if (state->x > ctx->plPosX)
      {
        // switch to "turning left"
        state->var3 = 1;
        state->var4 = 5;
      }
    }
    else // moving left
    {
      --state->x;
      ApplyWorldCollision(ctx, handle, MD_LEFT);

      if (state->x < ctx->plPosX)
      {
        // switch to "turning right"
        state->var3 = 2;
        state->var4 = 0;
      }
    }
  }

  if (state->var3 == 1) // turning left
  {
    if (ctx->gfxCurrentDisplayPage)
    {
      state->var4--;
    }

    if (state->var4 == 0)
    {
      state->var3 = 0;
      state->var1 = false;
    }
  }

  if (state->var3 == 2) // turning right
  {
    if (ctx->gfxCurrentDisplayPage)
    {
      state->var4++;
    }

    if (state->var4 == 5)
    {
      state->var3 = 0;
      state->var1 = true;
    }
  }

  // Draw the eye
  DrawActor(ctx, ACT_HOVERBOT, state->var4 + 6, state->x, state->y, DS_NORMAL);
}


static void Act_PlayerSprite(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  // Synchronize player sprite to player state
  state->x = ctx->plPosX;
  state->y = ctx->plPosY;
  state->id = ctx->plActorId;
  state->frame = ctx->plAnimationFrame;

  if (ctx->plState == PS_AIRLOCK_DEATH_L || ctx->plState == PS_AIRLOCK_DEATH_R)
  {
    return;
  }

  // Kill the player if fallen out of the map (bottom-less pit)
  if (ctx->plPosY - 4 > ctx->mapBottom && ctx->plPosY - 4 < ctx->mapBottom + 10)
  {
    ctx->gmGameState = GS_PLAYER_DIED;
    PlaySound(ctx, SND_DUKE_DEATH);
    return;
  }

  //
  // Cloaking device effect and timer
  //
  if (ctx->plCloakTimeLeft)
  {
    state->drawStyle = DS_TRANSLUCENT;

    --ctx->plCloakTimeLeft;

    if (ctx->plCloakTimeLeft == 30)
    {
      ShowInGameMessage(ctx, MID_CLOAK_DISABLING);
    }

    // Make player flash when cloak is disabling
    if (ctx->plCloakTimeLeft < 30 && ctx->gfxCurrentDisplayPage)
    {
      state->drawStyle = DS_WHITEFLASH;
    }

    if (ctx->plCloakTimeLeft == 0)
    {
      RemoveFromInventory(ctx, ACT_CLOAKING_DEVICE_ICON);
      SpawnActor(
        ctx,
        ACT_CLOAKING_DEVICE,
        ctx->gmCloakPickupPosX,
        ctx->gmCloakPickupPosY);
    }
  }

  //
  // Rapid fire powerup timer
  //
  if (ctx->plRapidFireTimeLeft)
  {
    --ctx->plRapidFireTimeLeft;

    if (ctx->plRapidFireTimeLeft == 30)
    {
      ShowInGameMessage(ctx, MID_RAPID_FIRE_DISABLING);
    }

    if (ctx->plRapidFireTimeLeft == 0)
    {
      RemoveFromInventory(ctx, ACT_RAPID_FIRE_ICON);
    }
  }

  //
  // Mercy frames (period of invincibility after getting hit)
  //
  if (ctx->plMercyFramesLeft)
  {
    if (ctx->plMercyFramesLeft & 1) // % 2
    {
      if (ctx->plMercyFramesLeft > 10)
      {
        state->drawStyle = DS_INVISIBLE;
      }
      else
      {
        state->drawStyle = DS_WHITEFLASH;
      }
    }

    ctx->plMercyFramesLeft--;
  }

  if (ctx->plState == PS_GETTING_EATEN || ctx->plAnimationFrame == 0xFF)
  {
    state->drawStyle = DS_INVISIBLE;
    ctx->plAttachedSpider1 = ctx->plAttachedSpider2 = ctx->plAttachedSpider3 =
      0;
  }

  //
  // Additional animation logic
  //

  // Draw exhaust flames when the ship is moving
  if (ctx->plState == PS_USING_SHIP && state->drawStyle != 0)
  {
    if (ctx->inputMoveLeft && ctx->inputMoveRight)
    {
      ctx->inputMoveLeft = ctx->inputMoveRight = 0;
    }

    if (ctx->inputMoveLeft && ctx->plActorId == ACT_DUKES_SHIP_L)
    {
      DrawActor(
        ctx,
        ACT_DUKES_SHIP_EXHAUST_FLAMES,
        ctx->gfxCurrentDisplayPage + 4,
        ctx->plPosX,
        ctx->plPosY,
        DS_NORMAL);
    }

    if (ctx->inputMoveRight && ctx->plActorId == ACT_DUKES_SHIP_R)
    {
      DrawActor(
        ctx,
        ACT_DUKES_SHIP_EXHAUST_FLAMES,
        ctx->gfxCurrentDisplayPage + 2,
        ctx->plPosX,
        ctx->plPosY,
        DS_NORMAL);
    }

    if (ctx->inputMoveUp && !ctx->inputMoveDown)
    {
      if (ctx->plActorId == ACT_DUKES_SHIP_L)
      {
        DrawActor(
          ctx,
          ACT_DUKES_SHIP_EXHAUST_FLAMES,
          ctx->gfxCurrentDisplayPage,
          ctx->plPosX + 1,
          ctx->plPosY,
          DS_NORMAL);
      }

      if (ctx->plActorId == ACT_DUKES_SHIP_R)
      {
        DrawActor(
          ctx,
          ACT_DUKES_SHIP_EXHAUST_FLAMES,
          ctx->gfxCurrentDisplayPage,
          ctx->plPosX,
          ctx->plPosY,
          DS_NORMAL);
      }
    }
  }
  else if (ctx->plInteractAnimTicks)
  {
    if (ctx->plState == PS_NORMAL)
    {
      state->frame = 33;
    }

    ctx->plInteractAnimTicks++;
    if (ctx->plInteractAnimTicks == 9)
    {
      ctx->plInteractAnimTicks = 0;
    }
  }
  else if (ctx->plState == PS_RIDING_ELEVATOR)
  {
    state->frame = 33;
  }
  else if (ctx->plState == PS_BLOWN_BY_FAN)
  {
    state->frame = 6;
  }
}


/** Item boxes and nucelar waste barrels
 *
 * A bit counterintuitively, this function implements the behavior not only for
 * the item box, but also the item within the box once it's been released.
 * Basically, the box turns into the item within when it's shot, instead of
 * spawning a new actor into the world. The only exception is the turkey, which
 * is implemented as a dedicated actor.
 *
 * Notably, this function implements the fire bomb and the different types of
 * soda cans.
 *
 * Part of the behavior that's common to all items is a brief fly up and fall
 * down sequence, with a short bounce when hitting the ground.
 */
static void Act_ItemBox(Context* ctx, word handle)
{
  register ActorState* state = ctx->gmActorStates + handle;

  // [PERF] Missing `static` causes a copy operation here
  const sbyte FLY_UP_ARC[] = {-3, -2, -1, 0, 1, 2, 3, -1, 1};

  if (!state->var1)
  {
    return;
  } // container hasn't been shot yet, stop here

  if (state->var1 == 1) // first step of getting shot sequence
  {
    // advance to step 2
    state->var1++;

    if (state->id == ACT_NUCLEAR_WASTE_CAN_EMPTY)
    {
      // The nuclear waste barrel has a brief animation where it
      // bulges up before exploding
      state->frame++;
      return;
    }
    else
    {
      // [PERF] Missing `static` causes a copy operation here
      // clang-format off
      const byte FX_LIST[] = {
        ACT_YELLOW_FIREBALL_FX, EM_FLY_UP,
        ACT_GREEN_FIREBALL_FX, EM_FLY_UPPER_LEFT,
        ACT_BLUE_FIREBALL_FX, EM_FLY_UPPER_RIGHT,
        ACT_GREEN_FIREBALL_FX, EM_FLY_DOWN
      };
      // clang-format on

      register int16_t i;

      for (i = 0; i < 8; i += 2)
      {
        SpawnEffect(ctx, FX_LIST[i], state->x, state->y, FX_LIST[i + 1], 0);
      }

      if (state->var2 == 0xFF) // box is empty
      {
        state->deleted = true;
        return;
      }
    }
  }

  if (state->var1 == 2) // step 2
  {
    if (state->id == ACT_NUCLEAR_WASTE_CAN_EMPTY)
    {
      // [PERF] Missing `static` causes a copy operation here
      // clang-format off
      const byte FX_LIST[] = {
        ACT_NUCLEAR_WASTE_CAN_DEBRIS_4, EM_FLY_UP,
        ACT_NUCLEAR_WASTE_CAN_DEBRIS_3, EM_FLY_DOWN,
        ACT_NUCLEAR_WASTE_CAN_DEBRIS_1, EM_FLY_UPPER_LEFT,
        ACT_NUCLEAR_WASTE_CAN_DEBRIS_2, EM_FLY_UPPER_RIGHT,
        ACT_SMOKE_CLOUD_FX, EM_NONE
      };
      // clang-format on

      register int16_t i;

      for (i = 0; i < 10; i += 2)
      {
        SpawnEffect(ctx, FX_LIST[i], state->x, state->y, FX_LIST[i + 1], 0);
      }
    }

    state->var1++;

    // Disable gravity for the fly up sequence
    state->gravityAffected = false;
  }

  if (state->var1 == 3) // step 3
  {
    if (state->var2 == ACT_NUCLEAR_WASTE)
    {
      // If the barrel has sludge inside, release it. The effect handles
      // damaging the player, so we don't need the barrel's actor anymore.
      state->deleted = true;
      SpawnEffect(ctx, ACT_NUCLEAR_WASTE, state->x, state->y, EM_NONE, 1);
      return;
    }

    state->id = state->var2;
    state->frame = 0;

    switch (state->id)
    {
      case ACT_RED_BOX_BOMB:
      case ACT_SODA_CAN:
      case ACT_SODA_6_PACK:
        // Make actor shootable again
        state->health = 1;
        break;

      case 0xFF:
        // Empty nuclear waste barrel
        state->deleted = true;
        return;

      case ACT_TURKEY:
        state->deleted = true;
        SpawnActor(ctx, ACT_TURKEY, state->x, state->y);
        return;
    }
  }

  // Fly-up sequence
  if (state->var1 < 12)
  {
    state->alwaysUpdate = true;

    state->y += FLY_UP_ARC[state->var1 - 3];

    state->var1++;

    if (
      state->var1 == 12 ||
      (state->var1 == 9 &&
       !CheckWorldCollision(
         ctx, MD_DOWN, state->id, state->frame, state->x, state->y + 1)))
    {
      state->gravityAffected = true;
    }
  }

  //
  // Item-specific behavior
  //
  switch (state->id)
  {
    case ACT_PC:
      state->frame = (byte)ctx->gfxCurrentDisplayPage;
      break;

    case ACT_RAPID_FIRE:
    case ACT_CLOAKING_DEVICE:
      UPDATE_ANIMATION_LOOP(state, 0, 3);
      break;

    case ACT_HEALTH_MOLECULE:
      UPDATE_ANIMATION_LOOP(state, 0, 8);
      break;

    case ACT_RED_BOX_BOMB:
      UPDATE_ANIMATION_LOOP(state, 0, 7);

      state->var3++;
      if (state->var3 > 24 && ctx->gfxCurrentDisplayPage)
      {
        state->drawStyle = DS_WHITEFLASH;
      }

      if (state->var3 == 32)
      {
        // [NOTE] This code is basically the same as in
        // HandleActorShotCollision(), a dedicated function would've been good
        // to reduce code duplication.
        register int16_t i;
        bool spawnFailedLeft = false;
        bool spawnFailedRight = false;

        ctx->gmBombBoxesLeft--;

        PLAY_EXPLOSION_SOUND();

        SpawnParticles(ctx, state->x + 1, state->y, 0, CLR_WHITE);

        for (i = 0; i < 12; i += 2)
        {
          if (!spawnFailedLeft)
          {
            spawnFailedLeft += SpawnEffect(
              ctx, ACT_FIRE_BOMB_FIRE, state->x - 2 - i, state->y, EM_NONE, i);
          }

          if (!spawnFailedRight)
          {
            spawnFailedRight += SpawnEffect(
              ctx, ACT_FIRE_BOMB_FIRE, state->x + i + 2, state->y, EM_NONE, i);
          }
        }

        state->deleted = true;
      }
      break;

    case ACT_SODA_CAN:
      UPDATE_ANIMATION_LOOP(state, 0, 5);

      if (state->var3) // has the coke can been shot?
      {
        state->gravityAffected = false;
        state->var3++;

        state->y--;

        if (CheckWorldCollision(
              ctx, MD_UP, ACT_SODA_CAN, 0, state->x, state->y))
        {
          SpawnEffect(
            ctx, ACT_COKE_CAN_DEBRIS_1, state->x, state->y, EM_FLY_LEFT, 0);
          SpawnEffect(
            ctx, ACT_COKE_CAN_DEBRIS_2, state->x, state->y, EM_FLY_RIGHT, 0);
          PLAY_EXPLOSION_SOUND();
          state->deleted = true;
          return;
        }

        // Draw the rocket exhaust flame
        DrawActor(
          ctx,
          ACT_SODA_CAN,
          ctx->gfxCurrentDisplayPage + 6,
          state->x,
          state->y,
          DS_NORMAL);
      }
      return;

    case ACT_SODA_6_PACK:
      if (state->var3) // has the 6-pack been shot?
      {
        register int16_t i;

        PLAY_EXPLOSION_SOUND();
        state->deleted = true;

        for (i = 0; i < 6; i++)
        {
          SpawnEffect(
            ctx,
            ACT_COKE_CAN_DEBRIS_1,
            state->x + (i & 2),
            state->y + (i & 1),
            i,
            0);
          SpawnEffect(
            ctx,
            ACT_COKE_CAN_DEBRIS_2,
            state->x + (i & 2),
            state->y + (i & 1),
            i,
            0);
        }

        GiveScore(ctx, 10000);
        SpawnEffect(
          ctx,
          ACT_SCORE_NUMBER_FX_10000,
          state->x,
          state->y,
          EM_SCORE_NUMBER,
          0);
      }
  }
}


static void Act_FlameThrowerBot(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  // Randomly decide to stop and shoot fire
  if (!(((int16_t)RandomNumber(ctx)) & 127))
  {
    state->var2 = 16;
  }

  if (state->var2)
  {
    // Count down waiting time
    state->var2--;

    if (state->var2 == 8)
    {
      // Shoot fire
      if (state->id == ACT_FLAME_THROWER_BOT_R)
      {
        SpawnEffect(
          ctx,
          ACT_FLAME_THROWER_FIRE_R,
          state->x + 7,
          state->y - 3,
          EM_NONE,
          0);
      }
      else
      {
        SpawnEffect(
          ctx,
          ACT_FLAME_THROWER_FIRE_L,
          state->x - 7,
          state->y - 3,
          EM_NONE,
          0);
      }
    }
  }
  else
  {
    if (state->var1) // moving up
    {
      if (ctx->gfxCurrentDisplayPage)
      {
        state->y--;
        if (ApplyWorldCollision(ctx, handle, MD_UP))
        {
          // Start moving down
          state->var1 = 0;
        }
      }
    }
    else // moving down
    {
      state->y++;
      if (ApplyWorldCollision(ctx, handle, MD_DOWN))
      {
        // Start moving up
        state->var1 = 1;
      }
    }
  }
}


static void Act_BonusGlobe(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  // Animate and draw the content inside the shell
  state->var2++;
  if (state->var2 == 4)
  {
    state->var2 = 0;
  }

  DrawActor(ctx, state->var1, state->var2, state->x, state->y, DS_NORMAL);
}


static void Act_WatchBot(Context* ctx, word handle)
{
  register ActorState* state = ctx->gmActorStates + handle;

  // [PERF] Missing `static` causes a copy operation here
  const word HIDE_HEAD_ANIM[] = {1, 2, 1, 0};

  if (state->var4)
  {
    // [PERF] Missing `static` causes a copy operation here
    const byte LOOK_AROUND_ANIMS[2][32] = {
      {1, 1, 1, 3, 3, 1, 6, 6, 7, 8, 7, 6, 6, 6, 7, 8,
       7, 6, 6, 6, 1, 1, 3, 3, 3, 1, 1, 1, 6, 6, 1, 1},

      {1, 1, 6, 6, 7, 8, 7, 6, 6, 1, 1, 3, 3, 1, 6, 6,
       1, 1, 1, 3, 4, 5, 4, 3, 3, 3, 4, 5, 4, 3, 1, 1}};

    state->frame = LOOK_AROUND_ANIMS[state->var5][state->var4 - 1];

    if (ctx->gfxCurrentDisplayPage)
    {
      state->var4++;
    }

    if (state->var4 == 33)
    {
      state->var4 = 0;
    }
  }
  else
  {
    if (state->var2 && !state->frame)
    {
      if (state->var1)
      {
        state->x++;
        ApplyWorldCollision(ctx, handle, MD_RIGHT);
      }
      else
      {
        state->x--;
        ApplyWorldCollision(ctx, handle, MD_LEFT);
      }
    }

  doMovementOrWait:
    if (!state->var2)
    {
      state->frame = (byte)HIDE_HEAD_ANIM[state->var3];
      state->var3++;

      if (
        (RandomNumber(ctx) & 33) && state->var3 == 2 &&
        !ApplyWorldCollision(ctx, handle, MD_DOWN))
      {
        state->var4 = 1;
        state->var5 = (word)RandomNumber(ctx) % 2;
      }
      else if (state->var3 == 4)
      {
        state->var2++;

        if (state->x > ctx->plPosX)
        {
          state->var1 = 0;
        }
        else
        {
          state->var1 = 1;
        }
      }
    }
    else
    {
      if (state->var2 < 6)
      {
        state->y--;

        if (ApplyWorldCollision(ctx, handle, MD_UP))
        {
          state->var2 = 5;
        }

        if (state->var2 < 3)
        {
          state->y--;

          if (ApplyWorldCollision(ctx, handle, MD_UP))
          {
            state->var2 = 5;
          }
        }

        state->var2++;

        if (state->var2 > 5)
        {
          state->gravityAffected = true;
          state->gravityState = 0;
          return;
        }
      }

      if (!state->gravityState && state->var2 == 6)
      {
        state->var2 = 0;
        state->gravityAffected = false;
        state->var3 = 0;

        PlaySoundIfOnScreen(ctx, handle, SND_DUKE_JUMPING);

        goto doMovementOrWait;
      }
    }
  }
}


static void Act_RocketTurret(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  if (!state->var1)
  {
    if (state->x - 3 > ctx->plPosX)
    {
      state->frame = 0;
    }
    else if (state->x + 3 < ctx->plPosX)
    {
      state->frame = 2;
    }
    else if (state->y > ctx->plPosY)
    {
      state->frame = 1;
    }

    state->var1 = 1;
  }
  else if (state->var1 < 25)
  {
    state->var1++;
    return;
  }

  if (state->var1 == 25)
  {
    switch (state->frame)
    {
      case 0:
        SpawnActor(ctx, ACT_ENEMY_ROCKET_LEFT, state->x - 2, state->y - 1);
        break;

      case 1:
        SpawnActor(ctx, ACT_ENEMY_ROCKET_UP, state->x + 1, state->y - 2);
        break;

      case 2:
        SpawnActor(ctx, ACT_ENEMY_ROCKET_RIGHT, state->x + 2, state->y - 1);
        break;
    }

    state->var1 = 0;
  }
}


static void Act_EnemyRocket(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  if (!IsActorOnScreen(ctx, handle))
  {
    state->deleted = true;
    return;
  }

  state->var1++;

  if (state->var1 == 1)
  {
    PlaySound(ctx, SND_FLAMETHROWER_SHOT);
  }

  if (state->id == ACT_ENEMY_ROCKET_LEFT)
  {
    state->x--;

    if (state->var1 > 4)
    {
      state->x--;
    }

    DrawActor(
      ctx,
      ACT_ENEMY_ROCKET_LEFT,
      ctx->gfxCurrentDisplayPage + 1,
      state->x,
      state->y,
      DS_NORMAL);

    if (ApplyWorldCollision(ctx, handle, MD_LEFT))
    {
      state->deleted = true;
    }
  }
  else if (state->id == ACT_ENEMY_ROCKET_RIGHT)
  {
    state->x++;

    if (state->var1 > 4)
    {
      state->x++;
    }

    DrawActor(
      ctx,
      ACT_ENEMY_ROCKET_RIGHT,
      ctx->gfxCurrentDisplayPage + 1,
      state->x,
      state->y,
      DS_NORMAL);

    if (ApplyWorldCollision(ctx, handle, MD_RIGHT))
    {
      state->deleted = true;
    }
  }
  else if (state->id == ACT_ENEMY_ROCKET_UP)
  {
    state->y--;

    if (state->var1 > 4)
    {
      state->y--;
    }

    DrawActor(
      ctx,
      ACT_ENEMY_ROCKET_UP,
      ctx->gfxCurrentDisplayPage + 1,
      state->x,
      state->y,
      DS_NORMAL);

    if (ApplyWorldCollision(ctx, handle, MD_UP))
    {
      state->deleted = true;
    }
  }
  else if (state->id == ACT_ENEMY_ROCKET_2_UP)
  {
    state->y--;

    if (state->var1 > 4)
    {
      state->y--;
    }

    DrawActor(
      ctx,
      ACT_ENEMY_ROCKET_2_UP,
      ctx->gfxCurrentDisplayPage + 1,
      state->x,
      state->y,
      DS_NORMAL);

    if (ApplyWorldCollision(ctx, handle, MD_UP))
    {
      state->deleted = true;
    }
  }
  else if (state->id == ACT_ENEMY_ROCKET_2_DOWN)
  {
    state->y++;

    if (state->var1 > 4)
    {
      state->y++;
    }

    DrawActor(
      ctx,
      ACT_ENEMY_ROCKET_2_DOWN,
      ctx->gfxCurrentDisplayPage + 1,
      state->x,
      state->y,
      DS_NORMAL);

    // [BUG] Should be MD_DOWN
    if (ApplyWorldCollision(ctx, handle, MD_UP))
    {
      state->deleted = true;
    }
  }

  if (state->deleted)
  {
    SpawnEffect(ctx, ACT_EXPLOSION_FX_1, state->x, state->y, EM_NONE, 0);
  }

  if (!IsActorOnScreen(ctx, handle))
  {
    state->deleted = true;
  }
}


static void Act_WatchBotContainerCarrier(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  if (!state->var1)
  {
    state->frame = 0;

    if (!PlayerInRange(ctx, handle, 5))
    {
      if (state->x > ctx->plPosX)
      {
        state->x--;

        if (ApplyWorldCollision(ctx, handle, MD_LEFT))
        {
          state->var1 = 1;
        }
      }
      else if (state->x + 3 < ctx->plPosX)
      {
        state->x++;

        if (ApplyWorldCollision(ctx, handle, MD_RIGHT))
        {
          state->var1 = 1;
        }
      }
    }
    else
    {
      state->var1 = 1;
    }
  }

  if (state->var1)
  {
    if (state->var1 < 35)
    {
      state->var1++;
    }

    if (state->var1 == 7)
    {
      state->frame = 1;
      state->var2 = 1;

      SpawnActor(ctx, ACT_WATCHBOT_CONTAINER, state->x, state->y - 2);
    }
    else if (state->var1 > 20 && state->var1 < 35)
    {
      state->frame = 0;
    }
    else if (state->var1 == 35)
    {
      state->deleted = true;

      PLAY_EXPLOSION_SOUND();
      SpawnBurnEffect(
        ctx, ACT_FLAME_FX, ACT_WATCHBOT_CONTAINER_CARRIER, state->x, state->y);
    }
  }

  if (!state->var2)
  {
    DrawActor(
      ctx, ACT_WATCHBOT_CONTAINER, 0, state->x, state->y - 2, DS_NORMAL);
  }
}


static void Act_WatchBotContainer(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  UPDATE_ANIMATION_LOOP(state, 1, 5);

  if (
    state->var1 < 10 &&
    !CheckWorldCollision(
      ctx, MD_UP, ACT_WATCHBOT_CONTAINER, 0, state->x, state->y - 1))
  {
    state->y--;
  }

  state->var1++;

  if (state->var1 == 25)
  {
    state->drawStyle = DS_WHITEFLASH;
    state->deleted = true;

    SpawnEffect(
      ctx, ACT_WATCHBOT_CONTAINER_DEBRIS_1, state->x, state->y, EM_FLY_LEFT, 0);
    SpawnEffect(
      ctx,
      ACT_WATCHBOT_CONTAINER_DEBRIS_2,
      state->x,
      state->y,
      EM_FLY_RIGHT,
      0);
    PlaySound(ctx, SND_ATTACH_CLIMBABLE);

    SpawnActor(ctx, ACT_WATCHBOT, state->x + 1, state->y + 3);
  }
  else
  {
    DrawActor(
      ctx, ACT_WATCHBOT_CONTAINER, 0, state->x, state->y, state->drawStyle);
  }
}


static void Act_BomberPlane(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  if (state->var1 == 0) // Fly towards player
  {
    state->x--;
    if (
      ApplyWorldCollision(ctx, handle, MD_LEFT) ||
      (state->x <= ctx->plPosX && state->x + 6 >= ctx->plPosX))
    {
      state->var1 = 1;
    }
  }
  else
  {
    // Wait
    if (state->var1 < 30)
    {
      state->var1++;
    }

    if (state->var1 == 10)
    {
      // Drop bomb
      SpawnActor(ctx, ACT_MINI_NUKE, state->x + 2, state->y + 1);
    }

    if (state->var1 == 30)
    {
      // Fly away
      state->y--;
      state->x -= 2;

      if (!IsActorOnScreen(ctx, handle))
      {
        state->deleted = true;
        return;
      }
    }
  }

  // Draw bomb if not dropped yet
  if (state->var1 < 10)
  {
    DrawActor(ctx, ACT_MINI_NUKE, 0, state->x + 2, state->y, DS_NORMAL);
  }

  // Draw exhaust flame
  DrawActor(
    ctx,
    ACT_BOMBER_PLANE,
    ctx->gfxCurrentDisplayPage + 1,
    state->x,
    state->y,
    DS_NORMAL);
}


static void Act_MiniNuke(Context* ctx, word handle)
{
  register ActorState* state = ctx->gmActorStates + handle;

  if (state->gravityState == 1)
  {
    state->frame++;
  }
  else if (state->gravityState == 3)
  {
    state->frame++;
  }
  else if (!state->gravityState)
  {
    state->deleted = true;

    PlaySound(ctx, SND_BIG_EXPLOSION);
    SpawnEffect(ctx, ACT_NUCLEAR_EXPLOSION, state->x, state->y, EM_NONE, 0);
    FLASH_SCREEN(SFC_WHITE);

    if (state->id != ACT_MINI_NUKE_SMALL)
    {
      register int16_t i;

      for (i = 4; i < 20; i += 4)
      {
        SpawnEffect(
          ctx,
          ACT_NUCLEAR_EXPLOSION,
          state->x - i,
          state->y + 2,
          EM_NONE,
          i >> 1); // i / 2
        SpawnEffect(
          ctx,
          ACT_NUCLEAR_EXPLOSION,
          state->x + i,
          state->y + 2,
          EM_NONE,
          i >> 1); // i / 2
      }
    }
  }
}


static void Act_SpikeBall(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  if (state->var1 == 2)
  {
    state->x++;

    if (ApplyWorldCollision(ctx, handle, MD_RIGHT))
    {
      state->var1 = 1;
    }
  }
  else if (state->var1 == 1)
  {
    state->x--;

    if (ApplyWorldCollision(ctx, handle, MD_LEFT))
    {
      state->var1 = 2;
    }
  }

  if (state->var2 < 5)
  {
    state->y--;

    if (ApplyWorldCollision(ctx, handle, MD_UP))
    {
      state->var2 = 5;
      PlaySoundIfOnScreen(ctx, handle, SND_DUKE_JUMPING);
    }

    if (state->var2 < 2)
    {
      state->y--;

      if (ApplyWorldCollision(ctx, handle, MD_UP))
      {
        state->var2 = 5;
        PlaySoundIfOnScreen(ctx, handle, SND_DUKE_JUMPING);
      }
    }

    state->var2++;

    if (state->var2 > 4)
    {
      state->gravityAffected = true;
      state->gravityState = 0;
      return;
    }
  }

  if (state->var2 >= 5 && state->var2 < 8)
  {
    state->var2++;
  }
  else if (!state->gravityState && state->var2 == 8)
  {
    state->var2 = 0;
    state->gravityAffected = false;
    PlaySoundIfOnScreen(ctx, handle, SND_DUKE_JUMPING);
  }
}


static void Act_Reactor(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  UPDATE_ANIMATION_LOOP(state, 0, 3);
}


static void Act_SlimeContainer(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  // Draw roof
  DrawActor(ctx, ACT_SLIME_CONTAINER, 8, state->x, state->y, DS_NORMAL);

  if (state->frame != 7)
  {
    if (state->var1 && state->frame < 7) // slime blob release animation
    {
      state->var1++;

      if (state->var1 == 4)
      {
        state->var1 = 1;
        state->frame++;
      }

      if (state->frame == 7)
      {
        SpawnActor(ctx, ACT_SLIME_BLOB, state->x + 2, state->y);
      }
    }
    else // still intact
    {
      // Draw bottom part
      DrawActor(ctx, ACT_SLIME_CONTAINER, 2, state->x, state->y, DS_NORMAL);

      // Animate slime blob moving around inside
      state->frame = RandomNumber(ctx) & 1; // % 2
    }
  }
}


static void Act_SlimeBlob(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  if (state->id == ACT_SLIME_BLOB_2) // on ceiling or flying
  {
    if (state->var1 == 100)
    {
      if (state->frame == 4)
      {
        state->y++;
      }

      if (state->frame == 3)
      {
        state->y++;

        if (!CheckWorldCollision(
              ctx, MD_DOWN, state->id, state->frame, state->x, state->y + 3))
        {
          return;
        }
      }

      state->frame--;

      if (!state->frame)
      {
        state->id = ACT_SLIME_BLOB;
        state->var1 = 0;
        state->var2 = 0;
      }
    }
    else
    {
      if (state->frame == 3)
      {
        state->y--;

        if (!CheckWorldCollision(
              ctx, MD_UP, state->id, state->frame, state->x, state->y))
        {
          return;
        }
      }

      if (state->frame < 6)
      {
        state->frame++;
      }
      else
      {
        if (state->x == ctx->plPosX)
        {
          state->var1 = 100;
          return;
        }

        state->var2 = !state->var2;
        state->frame =
          (byte)(state->x > ctx->plPosX ? state->var2 + 7 : state->var2 + 9);

        if (state->var2 % 2)
        {
          if (state->x > ctx->plPosX)
          {
            state->x--;

            if (
              CheckWorldCollision(
                ctx, MD_LEFT, state->id, state->frame, state->x, state->y) ||
              !CheckWorldCollision(
                ctx,
                MD_UP,
                state->id,
                state->frame,
                state->x - 4,
                state->y - 1))
            {
              state->var1 = 100;
            }
          }
          else
          {
            state->x++;

            if (
              CheckWorldCollision(
                ctx, MD_RIGHT, state->id, state->frame, state->x, state->y) ||
              !CheckWorldCollision(
                ctx,
                MD_UP,
                state->id,
                state->frame,
                state->x + 4,
                state->y - 1))
            {
              state->var1 = 100;
            }
          }
        }
      }
    }
  }
  else // on ground
  {
    if (state->var1 < 10)
    {
      state->var1++;

      if (!((word)RandomNumber(ctx) % 32))
      {
        // Start flying up
        state->id = ACT_SLIME_BLOB_2;
        state->frame = 0;
        state->var2 = 0;
      }
      else
      {
        state->frame = (byte)(state->var2 + (RandomNumber(ctx) & 3));

        if (state->var1 == 10)
        {
          if (state->x > ctx->plPosX)
          {
            state->var2 = 0;
          }
          else
          {
            state->var2 = 5;
          }
        }
      }
    }
    else
    {
      state->var3++;

      state->frame = (byte)(state->var2 + state->var3 % 2 + 3);

      if (
        (state->x > ctx->plPosX && state->var2) ||
        (state->x < ctx->plPosX && !state->var2))
      {
        state->var1 = 0;
      }
      else
      {
        if ((state->frame & 1) == 0)
        {
          if (state->var2)
          {
            state->x++;

            if (ApplyWorldCollision(ctx, handle, MD_RIGHT))
            {
              state->var1 = 0;
            }
          }
          else
          {
            state->x--;

            if (ApplyWorldCollision(ctx, handle, MD_LEFT))
            {
              state->var1 = 0;
            }
          }
        }
      }
    }
  }
}


static void Act_Snake(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  if (state->id == ctx->gmPlayerEatingActor && ctx->plState == PS_DYING)
  {
    // [PERF] Missing `static` causes a copy operation here
    // clang-format off
    int16_t DEBRIS_SPEC[] = { 3,
       0,  0, EM_NONE, 0,
      -1, -2, EM_NONE, 2,
       1, -3, EM_NONE, 4
    };
    // clang-format on

    SpawnDestructionEffects(ctx, handle, DEBRIS_SPEC, ACT_EXPLOSION_FX_1);
    state->deleted = true;
    ctx->gmPlayerEatingActor = 0;
    return;
  }

  if (state->var2)
  {
    ctx->gmPlayerEatingActor = state->id;
    ctx->plState = PS_GETTING_EATEN;

    if (state->var3 == 2)
    {
      ctx->plAnimationFrame = 0xFF;
    }

    if (!state->var4)
    {
      state->var3++;

      if (state->var3 == 7)
      {
        ctx->plPosX += 2;
        state->var4 = 1;

        if (state->var1 < 2)
        {
          state->var1 = 0;
        }
        else
        {
          state->var1 = 9;
        }
      }
    }

    if (state->var4)
    {
      DamagePlayer(ctx);

      if (ctx->inputFire && ctx->plState != PS_DYING)
      {
        state->health = 1;
        HandleActorShotCollision(ctx, state->health, handle);
        ctx->plState = PS_NORMAL;
        return;
      }
    }

    state->frame =
      (byte)(state->var1 + state->var3 + ctx->gfxCurrentDisplayPage);

    if (!state->var4)
    {
      return;
    }
  }

  state->var5++;

  if (state->var1 == 9) // facing right
  {
    if (!state->var2)
    {
      if (!ctx->gfxCurrentDisplayPage)
      {
        state->x++;
        state->frame = (byte)(state->var1 + (state->x & 1));
      }
    }
    else
    {
      if (ctx->gfxCurrentDisplayPage)
      {
        ctx->plPosX++;
        state->x++;
      }
    }

    if (ApplyWorldCollision(ctx, handle, MD_RIGHT))
    {
      state->var1 = 0;
      state->x += 2;
    }
  }
  else // facing left
  {
    if (!state->var2)
    {
      if (!ctx->gfxCurrentDisplayPage)
      {
        state->x--;
        state->frame = (byte)(state->x & 1);
      }
    }
    else
    {
      if (ctx->gfxCurrentDisplayPage)
      {
        ctx->plPosX--;
        state->x--;
      }
    }

    if (ApplyWorldCollision(ctx, handle, MD_LEFT))
    {
      state->var1 = 9;
      state->x -= 2;
    }
  }
}


static void Act_SecurityCamera(Context* ctx, word handle)
{
  register ActorState* state = ctx->gmActorStates + handle;
  word savedY = 0;

  if (ctx->plCloakTimeLeft)
  {
    state->frame = 0;
    return;
  }

  if (state->id == ACT_CAMERA_ON_CEILING)
  {
    savedY = state->y;
    state->y++;
  }

  if (state->x + 1 < ctx->plPosX)
  {
    if (state->y - 1 > ctx->plPosY)
    {
      state->var1 = 3;
    }
    else if (state->y < ctx->plPosY)
    {
      state->var1 = 1;
    }
    else
    {
      state->var1 = 2;
    }
  }
  else if (state->x > ctx->plPosX)
  {
    if (state->y - 1 > ctx->plPosY)
    {
      state->var1 = 5;
    }
    else if (state->y < ctx->plPosY)
    {
      state->var1 = 7;
    }
    else
    {
      state->var1 = 6;
    }
  }
  else
  {
    if (state->y >= ctx->plPosY)
    {
      state->var1 = 4;
    }
    else
    {
      state->var1 = 0;
    }
  }

  state->frame = (byte)state->var1;

  if (state->id == ACT_CAMERA_ON_CEILING)
  {
    state->y = savedY;
  }
}


static void Act_CeilingSucker(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  // [PERF] Missing `static` causes a copy operation here
  const byte GRAB_ANIM_SEQ[] = {0, 0, 1, 2, 3, 4, 5, 4, 3, 2, 1, 0};

  // [PERF] Missing `static` causes a copy operation here
  const byte EAT_PLAYER_ANIM_SEQ[] = {0, 0, 0,  0, 0, 0, 10, 9, 8, 7, 6,
                                      0, 6, 0,  6, 0, 6, 0,  6, 0, 6, 7,
                                      8, 9, 10, 5, 4, 3, 2,  1, 0};

  if (
    !state->var1 && ctx->plPosX + 4 >= state->x && state->x + 4 >= ctx->plPosX)
  {
    state->var1 = 1;
  }

  if (state->var1 < 100 && state->var1)
  {
    if (!state->var2)
    {
      state->frame = (byte)GRAB_ANIM_SEQ[state->var1];
    }
    else
    {
      state->frame = (byte)EAT_PLAYER_ANIM_SEQ[state->var1];
    }

    state->var1++;

    if (state->var2 && state->var1 == 25)
    {
      ctx->plState = PS_NORMAL;
      ctx->plAnimationFrame = 0;
      ctx->plPosX = state->x;
      DamagePlayer(ctx);
    }

    if ((state->var1 == 11 && !state->var2) || state->var1 == 31)
    {
      state->var1 = 100;
      state->var2 = 0;
    }
  }

  if (state->var1 > 99)
  {
    state->var1++;

    if (state->var1 == 140)
    {
      state->var1 = 0;
      state->var2 = 0;
    }
  }
}


static void Act_PlayerShip(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  // Update the cooldown timer - the ship can't be picked up again right after
  // exiting it. This is to prevent the player from immediately picking it up
  // right after jumping out of it, which would make it impossible to actually
  // exit the ship. See UpdateActorPlayerCollision()
  if (state->var1 != 0)
  {
    state->var1--;
  }
}


static void Act_BrokenMissile(Context* ctx, word handle)
{
  static const byte ANIM_SEQ[] = {1, 2, 3, 2, 3, 4, 3};

  register ActorState* state = ctx->gmActorStates + handle;
  register int16_t i;

  if (!state->var1)
  {
    return;
  } // Hasn't been shot yet

  if (state->var2 >= 12)
  {
    return;
  }

  // Fall over animation
  if (state->var2 < 7)
  {
    if (state->var1 == 1)
    {
      state->frame = (byte)ANIM_SEQ[state->var2];
    }
    else
    {
      state->frame = (byte)ANIM_SEQ[state->var2] + 4;
    }

    if (ANIM_SEQ[state->var2] == 3)
    {
      PlaySound(ctx, SND_ATTACH_CLIMBABLE);
    }
  }

  state->var2++;

  // Explode
  if (state->var2 == 12)
  {
    state->deleted = true;

    FLASH_SCREEN(SFC_WHITE);
    PLAY_EXPLOSION_SOUND();

    SpawnEffect(
      ctx,
      ACT_NUCLEAR_EXPLOSION,
      state->x - (state->var2 == 1 ? 4 : 0),
      state->y,
      EM_NONE,
      0);

    for (i = 0; i < 4; i++)
    {
      SpawnEffect(
        ctx,
        ACT_MISSILE_DEBRIS,
        (word)(state->x + (i << 1)), // * 2
        state->y,
        i & 1 ? EM_FLY_UPPER_LEFT : EM_FLY_UPPER_RIGHT,
        i);
    }
  }
}


static void Act_EyeBallThrower(Context* ctx, word handle)
{
  static const byte RISE_UP_ANIM[] = {0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5};

  ActorState* state = ctx->gmActorStates + handle;

  if (state->var1 == 0) // Turn towards player
  {
    if (state->x > ctx->plPosX)
    {
      state->id = ACT_EYEBALL_THROWER_L;
    }
    else
    {
      state->id = ACT_EYEBALL_THROWER_R;
    }

    // Start rising up
    state->var1 = 1;
  }
  else if (state->var1 && state->var1 < 11) // Rise up
  {
    state->frame = (byte)RISE_UP_ANIM[state->var1];
    state->var1++;
  }
  else if (state->var1 == 11) // Walk, decide to attack
  {
    // [PERF] Missing `static` causes a copy operation here
    const byte ANIM_SEQ[] = {5, 6};

    state->var3++;

    // Do we want to attack?
    if (
      ((state->id == ACT_EYEBALL_THROWER_L && state->x > ctx->plPosX) ||
       (state->id == ACT_EYEBALL_THROWER_R && state->x < ctx->plPosX)) &&
      PlayerInRange(ctx, handle, 14) && !PlayerInRange(ctx, handle, 9))
    {
      // Start attacking
      state->var1 = 12;
      state->var2 = 0;
    }

    if (state->var1 != 12 && state->var3 % 4 == 0)
    {
      // Animte walking
      state->frame = (byte)ANIM_SEQ[state->var2 = !state->var2];

      // Move
      if (state->id == ACT_EYEBALL_THROWER_L)
      {
        state->x--;

        if (ApplyWorldCollision(ctx, handle, MD_LEFT))
        {
          // Start reorienting
          state->var1 = 0;
          state->frame = 1;
        }
      }

      if (state->id == ACT_EYEBALL_THROWER_R)
      {
        state->x++;

        if (ApplyWorldCollision(ctx, handle, MD_RIGHT))
        {
          // Start reorienting
          state->var1 = 0;
          state->frame = 1;
        }
      }
    }
  }
  else if (state->var1 == 12) // Attack
  {
    // [PERF] Missing `static` causes a copy operation here
    const byte ANIM_SEQ[] = {7, 7, 8, 8, 9, 9};

    state->frame = ANIM_SEQ[state->var2++];

    // Throw eyeball
    if (state->var2 == 4)
    {
      if (state->id == ACT_EYEBALL_THROWER_L)
      {
        SpawnEffect(
          ctx,
          ACT_EYEBALL_PROJECTILE,
          state->x,
          state->y - 6,
          EM_FLY_UPPER_LEFT,
          0);
      }
      else
      {
        SpawnEffect(
          ctx,
          ACT_EYEBALL_PROJECTILE,
          state->x + 3,
          state->y - 6,
          EM_FLY_UPPER_RIGHT,
          0);
      }
    }

    if (state->var2 == 6)
    {
      // Back to walking
      state->var1 = 11;
    }
  }
}


static word FindActorDesc(
  Context* ctx,
  word startIndex,
  word neededId,
  word neededX,
  word neededY,
  word handle)
{
  register word i;
  register ActorState* state = ctx->gmActorStates + handle;
  word x;
  word y;

  for (i = startIndex; i < ctx->levelActorListSize * 2; i += 6)
  {
    word id = READ_LVL_ACTOR_DESC_ID(i);

    if (id == neededId)
    {
      x = READ_LVL_ACTOR_DESC_X(i);
      y = READ_LVL_ACTOR_DESC_Y(i);

      if (neededX == 0x8000)
      {
        if (neededY == y)
        {
          return i;
        }
      }
      else if (neededY == 0x8000)
      {
        if (neededX == x)
        {
          return i;
        }
      }
      else
      {
        if (neededX == x && neededY == y)
        {
          return i;
        }
      }
    }
  }

  state->deleted = true;

  return 0;
}


static void Act_MovingMapPartTrigger(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;
  word descIndex;
  word right;

  if (ctx->gmNumMovingMapParts == MAX_NUM_MOVING_MAP_PARTS - 1)
  {
    RaiseError(
      ctx,
      "Too many dynamic map parts for Classic mode - play this level in Enhanced mode");
    return;
  }

  MovingMapPartState* mapPart =
    ctx->gmMovingMapParts + ctx->gmNumMovingMapParts;

  state->drawStyle = DS_INVISIBLE;

  if (ctx->gmRequestUnlockNextDoor)
  {
    state->y += 5;

    if (!IsActorOnScreen(ctx, handle))
    {
      state->y -= 5;
      return;
    }
    else
    {
      state->y -= 5;
    }
  }

  if (state->var2 == 2)
  {
    if (ctx->gmRequestUnlockNextDoor)
    {
      ctx->gmRequestUnlockNextDoor = false;
    }
    else
    {
      return;
    }
  }

  if (
    (state->var2 == 3 || state->var2 == 5) &&
    (ctx->mapHasEarthquake == false ||
     ctx->gmEarthquakeCountdown >= ctx->gmEarthquakeThreshold ||
     ctx->gmEarthquakeCountdown == 0))
  {
    return;
  }

  if (state->var1)
  {
    state->var1--;

    if (state->var1 == 0)
    {
      PlaySound(ctx, SND_FALLING_ROCK);
    }

    return;
  }

  descIndex = FindActorDesc(ctx, 0, state->id, state->x, state->y, handle);
  mapPart->left = state->x;
  mapPart->top = state->y;

  descIndex = FindActorDesc(
    ctx, descIndex, ACT_META_DYNGEO_MARKER_1, 0x8000, state->y, handle);
  right = mapPart->right = READ_LVL_ACTOR_DESC_X(descIndex);

  descIndex = FindActorDesc(
    ctx, descIndex, ACT_META_DYNGEO_MARKER_2, right, 0x8000, handle);
  mapPart->bottom = READ_LVL_ACTOR_DESC_Y(descIndex);

  if (state->var2)
  {
    mapPart->type = state->var2 + 98;
  }

  if (!state->deleted)
  {
    ctx->gmNumMovingMapParts++;
    state->deleted = true;
  }
}


static void Act_HoverBotGenerator(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  // Spawn up to 30 robots
  if (state->var1 < 30)
  {
    state->var2++;

    if (state->var2 == 36)
    {
      SpawnActor(ctx, ACT_HOVERBOT, state->x + 1, state->y);

      state->var1++;
      state->var2 = 0;
    }
  }

  // Animate
  UPDATE_ANIMATION_LOOP(state, 0, 3);

  // Draw the lower part
  DrawActor(ctx, ACT_HOVERBOT_GENERATOR, 4, state->x, state->y, DS_NORMAL);
}


static void Act_MessengerDrone(Context* ctx, word handle)
{
  register ActorState* state = ctx->gmActorStates + handle;
  register byte* screenData;

  // Orient towards player on first update
  if (!state->var1)
  {
    if (state->x < ctx->plPosX)
    {
      state->var1 = 2;
    }
    else
    {
      state->var1 = 1;
    }

    // Set array index for the message animation, misusing the score field
    switch (state->var4)
    {
      case ACT_MESSENGER_DRONE_1:
        state->scoreGiven = 0;
        break;

      case ACT_MESSENGER_DRONE_2:
        state->scoreGiven = 1;
        break;

      case ACT_MESSENGER_DRONE_3:
        state->scoreGiven = 2;
        break;

      case ACT_MESSENGER_DRONE_4:
        state->scoreGiven = 3;
        break;

      case ACT_MESSENGER_DRONE_5:
        state->scoreGiven = 4;
        break;
    }
  }

  if (state->var1 == 1) // facing left
  {
    // Move while not showing message
    if (state->var2 == 0)
    {
      state->x -= 2;

      DrawActor(
        ctx,
        ACT_MESSENGER_DRONE_FLAME_R,
        ctx->gfxCurrentDisplayPage,
        state->x,
        state->y,
        DS_NORMAL);
    }

    DrawActor(
      ctx, ACT_MESSENGER_DRONE_ENGINE_R, 0, state->x, state->y, DS_NORMAL);
  }

  if (state->var1 == 2) // facing right
  {
    // Move while not showing message
    if (state->var2 == 0)
    {
      state->x += 2;

      DrawActor(
        ctx,
        ACT_MESSENGER_DRONE_FLAME_L,
        ctx->gfxCurrentDisplayPage,
        state->x,
        state->y,
        DS_NORMAL);
    }

    DrawActor(
      ctx, ACT_MESSENGER_DRONE_ENGINE_L, 0, state->x, state->y, DS_NORMAL);
  }

  DrawActor(
    ctx, ACT_MESSENGER_DRONE_ENGINE_DOWN, 0, state->x, state->y, DS_NORMAL);
  DrawActor(ctx, ACT_MESSENGER_DRONE_BODY, 0, state->x, state->y, DS_NORMAL);

  // Start showing the message when close enough to the player
  if (!state->var2 && !state->var3 && PlayerInRange(ctx, handle, 6))
  {
    state->var2 = 1;
  }

  // Show the message
  if (state->var2)
  {
    // Each array is a list of pairs of (animation frame, duration). The 1st
    // array element is not used, it's only there because the index starts at 1.
    // A value of 0xFF marks the end of the sequence.
    //
    // [NOTE] Not sure why they didn't simply subtract 1 from var2 before
    // indexing the array, that would seem easier/clearer?
    //
    // [PERF] Missing `static` causes a copy operation here
    byte SCREEN_CONTENT_ANIM_SEQS[5][50] = {
      // "Your brain is ours!"
      {0, 0, 10, 1, 10, 2, 10, 3, 14, 0, 10, 1, 10, 2, 10, 3, 14, 0xFF},

      // "Bring back the brain! ... Please stand by"
      {0, 0, 8, 1, 8, 2, 8, 3, 14, 4, 1, 5, 1, 6, 1, 7,   1,
       4, 1, 5, 1, 6, 1, 7, 1, 4,  1, 5, 1, 6, 1, 7, 1,   4,
       1, 5, 1, 6, 1, 7, 1, 8, 4,  9, 1, 8, 4, 9, 1, 0xFF},

      // "Live from Rigel it's Saturday night!"
      {0, 0, 4, 1, 4, 2, 3, 3, 6, 4, 3, 5, 5, 6, 15, 0xFF},

      // "Die!"
      {0, 0, 1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 15, 0xFF},

      // "You cannot escape us! You will get your brain sucked!"
      {0, 0, 8, 1, 8, 2, 8, 3, 8, 4, 8, 5, 8, 6, 8, 0xFF}};

    DrawActor(
      ctx,
      ACT_MESSENGER_DRONE_FLAME_DOWN,
      ctx->gfxCurrentDisplayPage,
      state->x,
      state->y,
      DS_NORMAL);

    screenData = SCREEN_CONTENT_ANIM_SEQS[state->scoreGiven] + state->var2;

    if (*screenData == 0xFF)
    {
      // Done
      state->var3 = true;
      state->var2 = 0;
    }
    else
    {
      // Count down current frame's delay, or advance to next frame
      if (state->var5)
      {
        state->var5--;

        if (state->var5 == 0)
        {
          state->var2 += 2;
        }
      }
      else
      {
        state->var5 = *(screenData + 1);
      }

      DrawActor(ctx, state->var4, *screenData, state->x, state->y, DS_NORMAL);
    }
  }

  // Delete ourselves once off screen if we're done showing the message
  if (state->var3 && !IsActorOnScreen(ctx, handle))
  {
    state->deleted = true;
  }

  // A drawStyle of DS_INVISIBLE means that this actor is excluded from
  // collision detection against player shots. To make the actor shootable
  // despite that, we need to manually invoke the collision check here.
  HandleActorShotCollision(ctx, TestShotCollision(ctx, handle), handle);
  state->drawStyle = DS_INVISIBLE;
}


static void Act_SlimePipe(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  state->drawStyle = DS_IN_FRONT;

  state->var1++;
  if (state->var1 % 25 == 0)
  {
    SpawnActor(ctx, ACT_SLIME_DROP, state->x + 1, state->y + 1);
    PlaySound(ctx, SND_WATER_DROP);
  }

  state->frame = !state->frame;
}


static void Act_SlimeDrop(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  if (!state->gravityState) // landed on ground?
  {
    state->frame++;

    if (state->frame == 2)
    {
      state->deleted = true;
    }
  }
}


static void Act_ForceField(Context* ctx, word handle)
{
  register ActorState* state = ctx->gmActorStates + handle;
  register word drawStyle;

  if (
    !IsSpriteOnScreen(ctx, ACT_FORCE_FIELD, 1, state->x, state->y) &&
    !IsSpriteOnScreen(ctx, ACT_FORCE_FIELD, 0, state->x, state->y))
  {
    return;
  }

  // Draw emitter on top
  DrawActor(ctx, ACT_FORCE_FIELD, 1, state->x, state->y, DS_NORMAL);

  if (state->var2)
  {
    return;
  } // If turned off, we're done here

  // Handle unlocking
  if (ctx->gmRequestUnlockNextForceField)
  {
    ctx->gmRequestUnlockNextForceField = false;
    state->var2 = true;
    return;
  }

  // Handle player collision
  if (AreSpritesTouching(
        ctx,
        ACT_FORCE_FIELD,
        2,
        state->x,
        state->y,
        ctx->plActorId,
        ctx->plAnimationFrame,
        ctx->plPosX,
        ctx->plPosY))
  {
    // Insta-kill player
    ctx->plHealth = 1;
    ctx->plMercyFramesLeft = 0;
    ctx->plCloakTimeLeft = 0;
    DamagePlayer(ctx);

    // [BUG] The cloak doesn't reappear if the player dies while cloaked
    // and then respawns at a checkpoint, potentially making the level
    // unwinnable.  This should use the same cloak respawning code here as
    // in Act_PlayerSprite().
  }

  //
  // Animate and draw the force field itself
  //
  state->var1++;

  if (RandomNumber(ctx) & 32)
  {
    drawStyle = DS_WHITEFLASH;
    PlaySound(ctx, SND_FORCE_FIELD_FIZZLE);
  }
  else
  {
    drawStyle = DS_NORMAL;
  }

  DrawActor(
    ctx, ACT_FORCE_FIELD, state->var1 % 3 + 2, state->x, state->y, drawStyle);
}


static void Act_KeyCardSlot(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  if (state->var1 == 0)
  {
    state->frame = 0;
  }
  else
  {
    UPDATE_ANIMATION_LOOP(state, 0, 3);
  }
}


static void Act_KeyHole(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  // [PERF] Missing `static` causes a copy operation here
  const byte KEY_HOLE_ANIMATION[] = {0, 1, 2, 3, 4, 3, 2, 1};

  if (state->var1 == 0)
  {
    state->frame = 5;
  }
  else
  {
    state->var2++;
    if (state->var2 == sizeof(KEY_HOLE_ANIMATION))
    {
      state->var2 = 0;
    }

    state->frame = KEY_HOLE_ANIMATION[state->var2];
  }
}


/** Returns index of the 1st fully blocking solid tile */
static word FindFullySolidTileIndex(Context* ctx)
{
  word i;

  for (i = 0; i < 1000; i++)
  {
    if (ctx->gfxTilesetAttributes[i] == 0x0F)
    {
      return i * 8;
    }
  }

  return 0;
}


static void Act_SlidingDoorVertical(Context* ctx, word handle)
{
  register ActorState* state = ctx->gmActorStates + handle;
  register word i;

  state->drawStyle = DS_INVISIBLE;

  if (!state->scoreGiven)
  {
    state->tileBuffer = MM_PushChunk(ctx, 9 * sizeof(word), CT_TEMPORARY);
    if (!state->tileBuffer)
    {
      return;
    }

    for (i = 0; i < 8; i++)
    {
      state->tileBuffer[i] = Map_GetTile(ctx, state->x, state->y - i + 1);
    }

    state->scoreGiven = FindFullySolidTileIndex(ctx);

    for (i = 1; i < 8; i++)
    {
      Map_SetTile(ctx, state->scoreGiven, state->x, state->y - i + 1);
    }
  }

  if (
    PlayerInRange(ctx, handle, 7) && state->y >= ctx->plPosY &&
    state->y - 7 < ctx->plPosY)
  {
    if (state->health == 1)
    {
      PlaySound(ctx, SND_SLIDING_DOOR);
    }

    for (i = state->health; i < 9; i++)
    {
      if (i)
      {
        DrawActor(
          ctx,
          ACT_SLIDING_DOOR_VERTICAL,
          i - state->health,
          state->x,
          state->y - i,
          DS_NORMAL);
      }

      if (state->health == 1 && i < 8)
      {
        Map_SetTile(ctx, state->tileBuffer[i], state->x, state->y - i + 1);
      }
    }

    if (state->health < 7)
    {
      state->health++;
    }
    else
    {
      Map_SetTile(ctx, state->scoreGiven, state->x, state->y - 7);
    }
  }
  else // player not in range
  {
    if (state->health == 7)
    {
      PlaySound(ctx, SND_SLIDING_DOOR);
    }

    for (i = state->health; i < 9; i++)
    {
      if (i)
      {
        DrawActor(
          ctx,
          ACT_SLIDING_DOOR_VERTICAL,
          i - state->health,
          state->x,
          state->y - i,
          DS_NORMAL);
      }

      if (state->health == 1 && i)
      {
        Map_SetTile(ctx, state->scoreGiven, state->x, state->y - i + 1);
      }
    }

    if (state->health != 0)
    {
      state->health--;
    }
  }
}


static void Act_SlidingDoorHorizontal(Context* ctx, word handle)
{
  register ActorState* state = ctx->gmActorStates + handle;
  register int16_t i;

  if (!state->scoreGiven)
  {
    state->tileBuffer = MM_PushChunk(ctx, 6 * sizeof(word), CT_TEMPORARY);
    if (!state->tileBuffer)
    {
      return;
    }

    for (i = 0; i < 5; i++)
    {
      state->tileBuffer[i] = Map_GetTile(ctx, state->x + i, state->y);
    }

    state->scoreGiven = FindFullySolidTileIndex(ctx);

    for (i = 0; i < 5; i++)
    {
      Map_SetTile(ctx, state->scoreGiven, state->x + i, state->y);
    }
  }

  if (
    state->x - 2 <= ctx->plPosX && state->x + 6 > ctx->plPosX &&
    state->y - 3 < ctx->plPosY && state->y + 7 > ctx->plPosY)
  {
    if (!state->frame)
    {
      for (i = 0; i < 5; i++)
      {
        Map_SetTile(ctx, state->tileBuffer[i], state->x + i, state->y);
      }

      PlaySound(ctx, SND_SLIDING_DOOR);
    }

    if (state->frame < 2)
    {
      state->frame++;
    }

    if (state->frame == 2)
    {
      Map_SetTile(ctx, state->scoreGiven, state->x, state->y);
      Map_SetTile(ctx, state->scoreGiven, state->x + 5, state->y);
    }
  }
  else
  {
    if (state->frame)
    {
      if (state->frame == 2)
      {
        PlaySound(ctx, SND_SLIDING_DOOR);
      }

      state->frame--;

      if (!state->frame)
      {
        for (i = 0; i < 5; i++)
        {
          Map_SetTile(ctx, state->scoreGiven, state->x + i, state->y);
        }
      }
    }
  }
}


static void Act_RespawnBeacon(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  if (state->var2)
  {
    if (IsActorOnScreen(ctx, handle))
    {
      state->var3 = 1;
      state->var1 = 2;
      state->frame = 5;
    }

    state->var2 = 0;
  }

  if (state->var3 > 1)
  {
    state->var3--;

    if (state->var3 % 2)
    {
      state->drawStyle = DS_WHITEFLASH;
    }

    if (state->var3 == 10)
    {
      ctx->gmBeaconPosX = state->x;
      ctx->gmBeaconPosY = state->y;
      ctx->gmBeaconActivated = true;

      ShowInGameMessage(ctx, MID_SECTOR_SECURE);
    }

    if (state->var3 == 1)
    {
      state->var1 = 1;
    }
  }
  else
  {
    if (state->var1)
    {
      UPDATE_ANIMATION_LOOP(state, 5, 8);
    }
  }
}


static void Act_Skeleton(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  if (!state->var5)
  {
    state->var5 = true;

    if (state->x > ctx->plPosX)
    {
      state->var1 = ORIENTATION_LEFT;
      state->frame = 0;
    }
    else
    {
      state->var1 = ORIENTATION_RIGHT;
      state->frame = 4;
    }
  }

  if (!ctx->gfxCurrentDisplayPage)
  {
    if (state->var1 != ORIENTATION_LEFT) // walking right
    {
      state->x++;

      if (ApplyWorldCollision(ctx, handle, MD_RIGHT))
      {
        state->var1 = ORIENTATION_LEFT;
        state->frame = 0;
      }
      else
      {
        UPDATE_ANIMATION_LOOP(state, 4, 7);
      }
    }
    else // walking left
    {
      state->x--;

      if (ApplyWorldCollision(ctx, handle, MD_LEFT))
      {
        state->var1 = ORIENTATION_RIGHT;
        state->frame = 4;
      }
      else
      {
        UPDATE_ANIMATION_LOOP(state, 0, 3);
      }
    }
  }
}


static void Act_BlowingFan(Context* ctx, word handle)
{
  // [PERF] Missing `static` causes a copy operation here
  const byte ANIM_SEQ[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2,
                           2, 2, 3, 3, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2,
                           3, 0, 1, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2,
                           0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2};

  // [PERF] Missing `static` causes a copy operation here
  const byte THREADS_ANIM_SEQ[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3,
    2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2,
    3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2};

  ActorState* state = ctx->gmActorStates + handle;

  if (state->var1) // slow down
  {
    state->var2--;

    if (state->var2 == 0)
    {
      state->var1 = false;
    }
  }
  else // spin up
  {
    state->var2++;

    if (state->var2 == 60)
    {
      state->var1 = true;
    }
  }

  state->frame = ANIM_SEQ[state->var2];

  DrawActor(
    ctx,
    ACT_BLOWING_FAN_THREADS_ON_TOP,
    THREADS_ANIM_SEQ[state->var2],
    state->x,
    state->y,
    DS_NORMAL);

  // Attach player if in range and fan at speed
  if (
    state->var2 > 24 && ctx->plPosY + 25 > state->y && state->y > ctx->plPosY &&
    state->x <= ctx->plPosX && state->x + 5 > ctx->plPosX &&
    ctx->plState != PS_DYING)
  {
    ctx->plState = PS_BLOWN_BY_FAN;
    ctx->gmActiveFanIndex = handle;

    if (
      state->frame == 3 || ctx->plPosY + 24 == state->y ||
      ctx->plPosY + 25 == state->y)
    {
      PlaySound(ctx, SND_SWOOSH);
    }
  }

  // Detach player if out of range, or fan too slow
  if (
    ctx->plState == PS_BLOWN_BY_FAN &&
    (state->var2 < 25 || state->x > ctx->plPosX ||
     state->x + 5 <= ctx->plPosX || state->y > ctx->plPosY + 25) &&
    handle == ctx->gmActiveFanIndex)
  {
    ctx->plState = PS_JUMPING;
    ctx->plJumpStep = 5;
    ctx->plAnimationFrame = 6;
  }

  // [NOTE] Could be simplified using PlaySoundIfOnScreen()
  if (state->frame == 2 && IsActorOnScreen(ctx, handle))
  {
    PlaySound(ctx, SND_SWOOSH);
  }
}


static void Act_LaserTurret(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  if (state->var1) // spinning
  {
    state->var1--;

    if (state->var1 == 0)
    {
      // Done spinning, become shootable again
      state->health = 1;
      state->var2 = 40;
    }
    else
    {
      // Make actor invincible while spinning
      state->health = 0;
    }

    if (state->var1 > 20)
    {
      state->frame++;
    }
    else if (state->var1 % 2 && state->var1 < 10)
    {
      state->frame--;
    }

    state->frame++;

    if (state->frame >= 8)
    {
      state->frame = 0;
    }

    if (state->frame == 5 || state->frame == 6)
    {
      PlaySound(ctx, SND_SWOOSH);
    }
  }
  else // not spinning
  {
    if (state->var2 < 7 && state->var2 % 2)
    {
      state->drawStyle = DS_WHITEFLASH;
    }

    if (state->x > ctx->plPosX) // player on the right
    {
      if (state->frame)
      {
        state->frame--;
      }
      else
      {
        state->var2--;

        if (!state->var2)
        {
          SpawnActor(ctx, ACT_ENEMY_LASER_SHOT_L, state->x - 3, state->y);
          state->var2 = 40;
        }
      }
    }
    else // player on the left
    {
      if (state->frame < 4)
      {
        state->frame++;
      }
      else if (state->frame > 4)
      {
        state->frame--;
      }
      else
      {
        state->var2--;

        if (!state->var2)
        {
          SpawnActor(ctx, ACT_ENEMY_LASER_SHOT_R, state->x + 2, state->y);
          state->var2 = 40;
        }
      }
    }
  }
}


static void Act_EnemyLaserShot(Context* ctx, word handle)
{
  register ActorState* state = ctx->gmActorStates + handle;

  if (!IsActorOnScreen(ctx, handle))
  {
    state->deleted = true;
    return;
  }

  if (!state->var2)
  {
    register word muzzleSprite;

    state->var2 = true;

    if (state->var1 == ACT_ENEMY_LASER_SHOT_R)
    {
      muzzleSprite = ACT_ENEMY_LASER_MUZZLE_FLASH_R;
    }
    else
    {
      muzzleSprite = ACT_ENEMY_LASER_MUZZLE_FLASH_L;
    }

    DrawActor(ctx, muzzleSprite, 0, state->x, state->y, DS_NORMAL);
    PlaySound(ctx, SND_ENEMY_LASER_SHOT);
  }

  if (state->var1 == ACT_ENEMY_LASER_SHOT_R)
  {
    state->x += 2;

    if (CheckWorldCollision(
          ctx, MD_RIGHT, ACT_ENEMY_LASER_SHOT_L, 0, state->x, state->y))
    {
      state->deleted = true;
    }
  }
  else
  {
    state->x -= 2;

    if (CheckWorldCollision(
          ctx, MD_LEFT, ACT_ENEMY_LASER_SHOT_L, 0, state->x, state->y))
    {
      state->deleted = true;
    }
  }
}


static void Act_LevelExitTrigger(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  state->drawStyle = DS_INVISIBLE;

  if (
    state->y >= ctx->plPosY && state->x >= ctx->plPosX &&
    state->x - 2 <= ctx->plPosX)
  {
    if (!ctx->gmRadarDishesLeft)
    {
      ctx->gmGameState = GS_LEVEL_FINISHED;
    }
    else
    {
      ShowTutorial(ctx, TUT_RADARS_LEFT);
    }
  }
}


static void Act_SuperForceField(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  // Animation when shot or touched by the player (see
  // UpdateActorPlayerCollision() and HandleActorShotCollision())
  if (state->var1)
  {
    state->var1++;

    state->frame = (byte)(ctx->gfxCurrentDisplayPage + 1);

    if (RandomNumber(ctx) & 8)
    {
      state->drawStyle = DS_WHITEFLASH;
      PlaySound(ctx, SND_FORCE_FIELD_FIZZLE);
    }

    if (state->var1 == 20)
    {
      state->var1 = 0;
      state->frame = 0;
    }
  }

  // Draw the emitter on top
  DrawActor(
    ctx, ACT_SUPER_FORCE_FIELD_L, state->var4, state->x, state->y, DS_NORMAL);

  // If not destroyed yet, we're done here. var3 is set in
  // UpdateActorPlayerCollision().
  if (!state->var3)
  {
    return;
  }

  //
  // Destruction animation
  //
  state->var3++;

  if (state->var3 % 2)
  {
    PlaySound(ctx, SND_GLASS_BREAKING);
    SpawnParticles(
      ctx, state->x + 1, state->y - state->var3 + 15, 0, CLR_LIGHT_BLUE);
    SpawnEffect(
      ctx,
      ACT_SCORE_NUMBER_FX_500,
      state->x,
      state->y - state->var3 + 19,
      EM_SCORE_NUMBER,
      0);
    GiveScore(ctx, 500);
  }

  if (state->var3 == 11)
  {
    state->deleted = true;

    SpawnEffect(
      ctx, ACT_EXPLOSION_FX_2, state->x - 1, state->y + 5, EM_FLY_DOWN, 0);
    SpawnEffect(
      ctx,
      ACT_EXPLOSION_FX_2,
      state->x - 1,
      state->y + 5,
      EM_FLY_UPPER_LEFT,
      0);
    SpawnEffect(
      ctx,
      ACT_EXPLOSION_FX_2,
      state->x - 1,
      state->y + 5,
      EM_FLY_UPPER_RIGHT,
      0);
    PlaySound(ctx, SND_BIG_EXPLOSION);
    ShowInGameMessage(ctx, MID_FORCE_FIELD_DESTROYED);
  }
}


static void Act_IntactMissile(Context* ctx, word handle)
{
  register ActorState* state = ctx->gmActorStates + handle;
  register int16_t i;

  if (state->var1) // launching/flying?
  {
    DrawActor(
      ctx,
      ACT_MISSILE_EXHAUST_FLAME,
      ctx->gfxCurrentDisplayPage,
      state->x,
      state->y,
      DS_NORMAL);

    if (state->var1 == 1)
    {
      SpawnEffect(
        ctx, ACT_WHITE_CIRCLE_FLASH_FX, state->x - 2, state->y + 1, EM_NONE, 0);
      SpawnEffect(
        ctx, ACT_WHITE_CIRCLE_FLASH_FX, state->x + 1, state->y + 1, EM_NONE, 0);
    }

    if (state->var1 > 5)
    {
      state->y--;

      if (ApplyWorldCollision(ctx, handle, MD_UP))
      {
        state->var2 = 1;
      }

      if (state->var1 > 8)
      {
        state->y--;

        if (ApplyWorldCollision(ctx, handle, MD_UP))
        {
          state->var2 = 1;
        }
      }

      PlaySound(ctx, SND_FLAMETHROWER_SHOT);
    }

    if (state->var1 <= 8)
    {
      state->var1++;
    }
  }

  if (state->var2) // ceiling hit?
  {
    state->deleted = true;

    FLASH_SCREEN(SFC_WHITE);
    PLAY_EXPLOSION_SOUND();

    Map_DestroySection(
      ctx, state->x, state->y - 14, state->x + 2, state->y - 12);

    for (i = 0; i < 4; i++)
    {
      SpawnEffect(
        ctx,
        ACT_MISSILE_DEBRIS,
        state->x + i * 2,
        state->y - 8,
        (word)i % 2 ? EM_FLY_LEFT : EM_FLY_DOWN,
        i);
      SpawnEffect(
        ctx,
        ACT_MISSILE_DEBRIS,
        state->x + i * 2,
        (state->y - 8) + i * 2,
        (word)i % 2 ? EM_FLY_UP : EM_FLY_RIGHT,
        i);
    }

    state->drawStyle = DS_INVISIBLE;
  }
}


static void Act_GrabberClaw(Context* ctx, word handle)
{
  register ActorState* state = ctx->gmActorStates + handle;
  register word i;
  word savedY;

  state->drawStyle = DS_INVISIBLE;

  // Waiting - neither shootable nor player damaging in this state
  if (state->var2 == 3)
  {
    state->var1++;

    DrawActor(ctx, ACT_METAL_GRABBER_CLAW, 0, state->x, state->y, DS_NORMAL);
    DrawActor(
      ctx, ACT_METAL_GRABBER_CLAW, 1, state->x, state->y + 1, DS_NORMAL);

    if (state->var1 == 10)
    {
      state->var1 = 1;
      state->var2 = 1;
    }
  }
  else // active
  {
    // Draw mounting pole at needed length
    for (i = 0; i < state->var1; i++)
    {
      DrawActor(
        ctx, ACT_METAL_GRABBER_CLAW, 0, state->x, state->y + i, DS_NORMAL);
    }

    // Extending
    if (state->var2 == 1)
    {
      DrawActor(
        ctx, ACT_METAL_GRABBER_CLAW, 1, state->x, state->y + i, DS_NORMAL);

      state->var1++;

      if (state->var1 == 7)
      {
        // Start grabbing
        state->var2 = 2;
      }
    }

    // Retracting
    if (state->var2 == 0)
    {
      DrawActor(
        ctx, ACT_METAL_GRABBER_CLAW, 1, state->x, state->y + i, DS_NORMAL);

      state->var1--;

      if (state->var1 == 1)
      {
        state->var2 = 3;
      }
    }

    // Grabbing - player damaging in this state
    if (state->var2 == 2)
    {
      const byte ANIM_SEQ[] = {
        0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0};

      state->frame = 1 + ANIM_SEQ[state->var3];

      DrawActor(
        ctx,
        ACT_METAL_GRABBER_CLAW,
        state->frame,
        state->x,
        state->y + i,
        DS_NORMAL);

      // Manually test for collision against the player,
      // since we use drawStyle DS_INVISIBLE.
      if (AreSpritesTouching(
            ctx,
            state->id,
            2,
            state->x,
            state->y + i,
            ctx->plActorId,
            ctx->plAnimationFrame,
            ctx->plPosX,
            ctx->plPosY))
      {
        DamagePlayer(ctx);
      }

      state->var3++;

      if (state->var3 == 19)
      {
        state->var3 = 0;
        state->var2 = 0;
      }
    }

    // Manually test for collision against player shots,
    // since we use drawStyle DS_INVISIBLE. Because the actor position
    // itself doesn't change while extending/retracting, temporarily
    // adjust it for the collision check.
    savedY = state->y;
    state->y += i;

    if (TestShotCollision(ctx, handle))
    {
      state->deleted = true;

      GiveScore(ctx, 250);
      SpawnEffect(
        ctx,
        ACT_METAL_GRABBER_CLAW_DEBRIS_1,
        state->x,
        state->y,
        EM_FLY_UPPER_LEFT,
        0);
      SpawnEffect(
        ctx,
        ACT_METAL_GRABBER_CLAW_DEBRIS_2,
        state->x + 2,
        state->y,
        EM_FLY_UPPER_RIGHT,
        0);
      PLAY_EXPLOSION_SOUND();
      SpawnBurnEffect(ctx, ACT_FLAME_FX, state->id, state->x, state->y);
    }

    state->y = savedY;
  }
}


static void Act_FloatingLaserBot(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;
  int16_t xDiff;
  int16_t yDiff;

  if (state->var1 < 10) // Waiting
  {
    state->var1++;

    if (!IsActorOnScreen(ctx, handle))
    {
      state->alwaysUpdate = false;
    }
  }
  else if (state->var2 < 40) // Moving towards player
  {
    if (!((word)RandomNumber(ctx) % 4))
    {
      xDiff = Sign(ctx->plPosX - state->x + 1);
      yDiff = Sign(ctx->plPosY - state->y - 2);

      state->x += xDiff;
      state->y += yDiff;

      if (xDiff > 0)
      {
        if (CheckWorldCollision(
              ctx, MD_RIGHT, ACT_HOVERING_LASER_TURRET, 0, state->x, state->y))
        {
          state->x--;
        }
      }

      if (xDiff < 0)
      {
        if (CheckWorldCollision(
              ctx, MD_LEFT, ACT_HOVERING_LASER_TURRET, 0, state->x, state->y))
        {
          state->x++;
        }
      }

      if (yDiff > 0)
      {
        ApplyWorldCollision(ctx, handle, MD_DOWN);
      }

      if (yDiff < 0)
      {
        ApplyWorldCollision(ctx, handle, MD_UP);
      }
    }

    state->var2++;
  }
  else if (state->var2 < 50) // Opening
  {
    state->var2++;

    if (state->frame < 5)
    {
      state->frame++;
    }
  }
  else if (state->var2 < 80) // Shooting
  {
    if (ctx->gfxCurrentDisplayPage)
    {
      return;
    }

    switch (state->var2 % 4)
    {
      case 0:
        SpawnActor(ctx, ACT_ENEMY_LASER_SHOT_L, state->x - 2, state->y - 1);
        break;

      case 1:
        SpawnActor(ctx, ACT_ENEMY_LASER_SHOT_L, state->x - 2, state->y);
        break;

      case 2:
        SpawnActor(ctx, ACT_ENEMY_LASER_SHOT_R, state->x + 2, state->y);
        break;

      case 3:
        SpawnActor(ctx, ACT_ENEMY_LASER_SHOT_R, state->x + 2, state->y - 1);
        break;
    }

    state->var2++;
  }
  else if (state->var2 < 100) // Closing
  {
    if (state->frame != 0)
    {
      state->frame--;
    }
    else
    {
      // Back to waiting
      state->var1 = 0;
      state->var2 = 0;
    }
  }
}


static void Act_Spider(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  // Delete ourselves if attached to the player and the player gets eaten by
  // a snake or ceiling sucker
  if (ctx->plState == PS_GETTING_EATEN)
  {
    if (ctx->plAttachedSpider1 == handle)
    {
      ctx->plAttachedSpider1 = 0;
      state->deleted = true;
      return;
    }

    if (ctx->plAttachedSpider2 == handle)
    {
      ctx->plAttachedSpider2 = 0;
      state->deleted = true;
      return;
    }

    if (ctx->plAttachedSpider3 == handle)
    {
      ctx->plAttachedSpider3 = 0;
      state->deleted = true;
      return;
    }
  }

  if (state->var4) // attached to player
  {
    //
    // Move spider along with player, and animate
    //
    if (!state->scoreGiven)
    {
      state->y = ctx->plPosY - 3;
      state->x = ctx->plActorId == ACT_DUKE_L ? ctx->plPosX + 1 : ctx->plPosX;

      state->frame = (byte)(8 + ((word)RandomNumber(ctx) % 2));
    }
    else
    {
      if (ctx->plAttachedSpider2 == handle)
      {
        state->y = ctx->plPosY - 1;

        if (ctx->plActorId == ACT_DUKE_L)
        {
          state->x = ctx->plPosX - 1;
          state->var2 = 12;
        }
        else
        {
          state->x = ctx->plPosX + 2;
          state->var2 = 14;
        }
      }
      else if (ctx->plAttachedSpider3 == handle)
      {
        state->y = ctx->plPosY - 2;

        if (ctx->plActorId == ACT_DUKE_R)
        {
          state->x = ctx->plPosX - 2;
          state->var2 = 12;
        }
        else
        {
          state->x = ctx->plPosX + 3;
          state->var2 = 14;
        }
      }

      state->frame = (byte)(state->var2 + ((word)RandomNumber(ctx) % 2));
    }

    //
    // Fall off if player quickly changes direction
    //
    if (ctx->plActorId != state->var4)
    {
      // Whenever the player orientation is different than last frame, increment
      // this counter. When the counter reaches 2, the spider falls off.
      // If the player orientation is unchanged, the counter decays back down
      // to zero, but only every other frame.
      state->var5++;

      state->var4 = ctx->plActorId;

      if (state->var5 == 2)
      {
        SpawnEffect(
          ctx,
          ACT_SPIDER_SHAKEN_OFF,
          state->x,
          state->y,
          RandomNumber(ctx) & 2 ? EM_FLY_UPPER_LEFT : EM_FLY_UPPER_RIGHT,
          0);

        if (ctx->plAttachedSpider2 == handle)
        {
          ctx->plAttachedSpider2 = 0;
        }
        else if (ctx->plAttachedSpider3 == handle)
        {
          ctx->plAttachedSpider3 = 0;
        }
        else
        {
          ctx->plAttachedSpider1 = 0;
        }

        state->deleted = true;
      }
    }
    else // player orientation unchanged
    {
      if (ctx->gfxCurrentDisplayPage && state->var5)
      {
        state->var5--;
      }
    }

    // Also fall off if player is dying
    if (ctx->plState == PS_DYING)
    {
      SpawnEffect(
        ctx,
        ACT_SPIDER_SHAKEN_OFF,
        state->x,
        state->y,
        RandomNumber(ctx) & 2 ? EM_FLY_UPPER_RIGHT : EM_FLY_UPPER_LEFT,
        0);

      state->deleted = true;
    }

    return;
  }

  if (state->var3 == 0)
  {
    state->var3 = 1;

    if (CheckWorldCollision(
          ctx, MD_DOWN, ACT_SPIDER, 0, state->x, state->y + 1))
    {
      state->scoreGiven = true;
      state->frame = 9;
    }
  }

  //
  // Movement
  //
  if (state->var1 >= 2 || ctx->gfxCurrentDisplayPage == 0)
  {
    if (state->var1 == ORIENTATION_RIGHT)
    {
      state->x++;

      if (state->scoreGiven) // on ground
      {
        if (ApplyWorldCollision(ctx, handle, MD_RIGHT))
        {
          state->frame = 9;
          state->var1 = ORIENTATION_LEFT;
        }
        else
        {
          UPDATE_ANIMATION_LOOP(state, 6, 8);
        }

        // Skip logic for falling onto the player
        return;
      }

      // on ceiling
      if (
        CheckWorldCollision(ctx, MD_RIGHT, ACT_SPIDER, 0, state->x, state->y) ||
        !CheckWorldCollision(
          ctx, MD_UP, ACT_SPIDER, 0, state->x + 2, state->y - 1))
      {
        state->x--;
        state->var1 = ORIENTATION_LEFT;
        state->frame = 3;
      }
      else
      {
        UPDATE_ANIMATION_LOOP(state, 0, 2);
      }
    }

    if (state->var1 == ORIENTATION_LEFT)
    {
      state->x--;

      if (state->scoreGiven) // on ground
      {
        if (ApplyWorldCollision(ctx, handle, MD_LEFT))
        {
          state->frame = 6;
          state->var1 = ORIENTATION_RIGHT;
        }
        else
        {
          UPDATE_ANIMATION_LOOP(state, 9, 11);
        }

        // Skip logic for falling onto the player
        return;
      }

      // on ceiling
      if (
        CheckWorldCollision(ctx, MD_LEFT, ACT_SPIDER, 0, state->x, state->y) ||
        !CheckWorldCollision(
          ctx, MD_UP, ACT_SPIDER, 0, state->x - 2, state->y - 1))
      {
        state->x++;
        state->var1 = ORIENTATION_RIGHT;
        state->frame = 0;
      }
      else
      {
        UPDATE_ANIMATION_LOOP(state, 3, 5);
      }
    }
  }

  // Check if we want to fall onto the player from above
  if (
    state->x == ctx->plPosX && state->var1 != 2 && state->frame < 6 &&
    state->y < ctx->plPosY - 3)
  {
    state->var1 = 2;
    state->frame = 6;
    state->gravityAffected = true;
    return;
  }

  if (state->var1 == 2 && !state->gravityState)
  {
    // We've reached the ground
    state->scoreGiven = true;
    state->var1 = ORIENTATION_RIGHT;
  }
}


static bool BlueGuard_UpdateShooting(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  if (ctx->plCloakTimeLeft)
  {
    return false;
  }

  // Don't attack if facing away from the player. Frames 0..5 are facing right,
  // 6..11 are facing left.
  if (
    (state->frame < 6 && state->x > ctx->plPosX) ||
    (state->frame > 5 && state->x < ctx->plPosX))
  {
    return false;
  }

  if (
    state->y + 3 > ctx->plPosY && state->y - 3 < ctx->plPosY &&
    ctx->plState == PS_NORMAL)
  {
    if (state->var3) // Stance change cooldown set
    {
      state->var3--;
    }
    else
    {
      if (ctx->inputMoveDown || state->y < ctx->plPosY)
      {
        // Crouch down
        state->frame = state->var1 ? 11 : 5;

        // Set stance change cooldown
        state->var3 = (word)RandomNumber(ctx) % 16;
      }
      else
      {
        // Stand up
        state->frame = state->var1 ? 10 : 4;
      }
    }
  }
  else
  {
    return false;
  }


  if (!((word)RandomNumber(ctx) % 8))
  {
    switch (state->frame)
    {
      case 10:
        SpawnActor(ctx, ACT_ENEMY_LASER_SHOT_L, state->x - 2, state->y - 2);
        state->frame = 15; // Recoil animation
        break;

      case 11:
        SpawnActor(ctx, ACT_ENEMY_LASER_SHOT_L, state->x - 2, state->y - 1);
        break;

      case 4:
        SpawnActor(ctx, ACT_ENEMY_LASER_SHOT_R, state->x + 3, state->y - 2);
        state->frame = 14; // Recoil animation
        break;

      case 5:
        SpawnActor(ctx, ACT_ENEMY_LASER_SHOT_R, state->x + 3, state->y - 1);
        break;
    }
  }

  return true;
}


static void Act_BlueGuard(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;
  bool roundTwo = false;

  // Reset "recoil" animation back to regular version
  if (state->frame == 15)
  {
    state->frame = 10;
  }
  else if (state->frame == 14)
  {
    state->frame = 4;
  }

begin:
  // "Typing on computer" state
  if (state->var5 > 1)
  {
    if (state->y == ctx->plPosY && PlayerInRange(ctx, handle, 6))
    {
      // Stop typing
      state->var5 = 1;

      // Face player
      if (state->x > ctx->plPosX)
      {
        state->var1 = 1;
      }
      else
      {
        state->var1 = 0;
      }
    }
    else // continue typing
    {
      state->frame = (byte)(
        12 + ((word)ctx->gfxCurrentDisplayPage >> (RandomNumber(ctx) & 4)));
      return;
    }
  }

  // Attack if player in sight
  if (BlueGuard_UpdateShooting(ctx, handle))
  {
    // Don't walk when attacking
    return;
  }

  state->var3 = 0;

  //
  // Walking
  //
  if (state->var1 == 0 && ctx->gfxCurrentDisplayPage)
  {
    // Count how many steps we've walked
    state->var2++;

    state->x++;

    if (ApplyWorldCollision(ctx, handle, MD_RIGHT) || state->var2 == 20)
    {
      // Turn around
      state->var1 = 1;
      state->var2 = 0;

      // [BUG?] Unlike below when turning from left to right, the guard doesn't
      // immediately start attacking after turning around.
      state->frame = 6;
    }
    else
    {
      // Animate the walk cycle
      state->frame++;

      if (state->frame > 3)
      {
        state->frame = 0;
      }
    }
  }

  if (state->var1 == 1 && ctx->gfxCurrentDisplayPage)
  {
    // Count how many steps we've walked
    state->var2++;

    state->x--;

    if (ApplyWorldCollision(ctx, handle, MD_LEFT) || state->var2 == 20)
    {
      // Turn around
      state->var1 = 0;
      state->var2 = 0;

      if (!roundTwo)
      {
        roundTwo = true;
        goto begin;
      }
    }
    else
    {
      // Animate the walk cycle
      state->frame++;

      if (state->frame > 9 || state->frame < 6)
      {
        state->frame = 6;
      }
    }
  }
}


static void Act_SpikedGreenCreature(Context* ctx, word handle)
{
  register ActorState* state = ctx->gmActorStates + handle;

  if (state->var1 < 15) // waiting
  {
    state->var1++;

    // Blinking eye
    if (state->var1 == 5)
    {
      SpawnEffect(
        ctx,
        state->id == ACT_GREEN_CREATURE_L ? ACT_GREEN_CREATURE_EYE_FX_L
                                          : ACT_GREEN_CREATURE_EYE_FX_R,
        state->x,
        state->y,
        EM_NONE,
        0);

      SpawnEffect(
        ctx,
        state->id == ACT_GREEN_CREATURE_L ? ACT_GREEN_CREATURE_EYE_FX_L
                                          : ACT_GREEN_CREATURE_EYE_FX_R,
        state->x,
        state->y,
        EM_NONE,
        4);
    }

    // Shell burst animation
    if (state->var1 == 15)
    {
      register int16_t i;
      word effectId;

      if (state->id == ACT_GREEN_CREATURE_L)
      {
        effectId = ACT_GREEN_CREATURE_SHELL_1_L;
      }
      else
      {
        effectId = ACT_GREEN_CREATURE_SHELL_1_R;
      }

      for (i = 0; i < 4; i++)
      {
        SpawnEffect(ctx, effectId + i, state->x, state->y, i, 0);
      }

      PlaySound(ctx, SND_GLASS_BREAKING);

      // Switch to form without shell
      state->frame++;
    }
  }
  else // awake
  {
    if (state->var1 < 30)
    {
      if (state->x > ctx->plPosX)
      {
        state->id = ACT_GREEN_CREATURE_L;
      }
      else
      {
        state->id = ACT_GREEN_CREATURE_R;
      }

      state->var1++;

      if (state->var1 == 26)
      {
        state->frame++;
      }
    }
    else
    {
      if (state->var1 <= 31)
      {
        // A list of entries: (anim frame, x offset, y offset). 0xFF terminates
        // the list.
        // [PERF] Missing `static` causes a copy operation here
        const int16_t JUMP_SEQUENCE[] = {
          3, 0, 0, 3, 0, 0, 4, 2, -2, 4, 2, -1, 4, 2, 0, 5, 2, 0, 0xFF};

        if (state->var1 == 30)
        {
          state->gravityAffected = false;
          state->var2 = 0;
          state->var1 = 31;
        }

        state->frame = (byte)JUMP_SEQUENCE[state->var2];

        if (state->frame == 0xFF)
        {
          state->var1 = 32;
          state->frame = 5;

          state->gravityAffected = true;

          if (state->id == ACT_GREEN_CREATURE_L)
          {
            state->x -= 2;
          }
          else
          {
            state->x += 2;
          }

          state->gravityState = 2;
        }
        else
        {
          if (state->id == ACT_GREEN_CREATURE_L)
          {
            state->x -= JUMP_SEQUENCE[state->var2 + 1];
          }
          else
          {
            state->x += JUMP_SEQUENCE[state->var2 + 1];
          }

          // [BUG] No collision checking at all for Y movement.
          state->y += JUMP_SEQUENCE[state->var2 + 2];

          state->var2 += 3;
        }
      }
      else if (state->var1 == 32)
      {
        if (!state->gravityState)
        {
          state->var1 = 15;
          state->gravityAffected = false;
          state->frame = 2;
        }

        if (state->id == ACT_GREEN_CREATURE_L)
        {
          state->x -= 2;
        }
        else
        {
          state->x += 2;
        }
      }

      // [BUG] The actor moves by more than one unit per frame, but there's only
      // one collision check - the actor can move through walls under the right
      // circumstances.
      if (
        state->id == ACT_GREEN_CREATURE_L &&
        ApplyWorldCollision(ctx, handle, MD_LEFT))
      {
        state->id = ACT_GREEN_CREATURE_R;
      }
      else if (
        state->id == ACT_GREEN_CREATURE_R &&
        ApplyWorldCollision(ctx, handle, MD_RIGHT))
      {
        state->id = ACT_GREEN_CREATURE_L;
      }
    }
  }
}


static void Act_GreenPanther(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  // [PERF] Missing `static` causes a copy operation here
  const byte ANIM_SEQ[] = {0, 1, 2, 1};

  if (state->var1)
  {
    --state->var1;

    state->frame = 0;

    if (!state->var1)
    {
      state->var2 = 0;
    }
  }
  else
  {
    if (state->id == ACT_BIG_GREEN_CAT_L)
    {
      state->x--;

      if (ApplyWorldCollision(ctx, handle, MD_LEFT))
      {
        state->id = ACT_BIG_GREEN_CAT_R;
        state->var1 = 10;
      }

      state->x--;

      if (ApplyWorldCollision(ctx, handle, MD_LEFT))
      {
        state->id = ACT_BIG_GREEN_CAT_R;
        state->var1 = 10;
      }
    }
    else
    {
      state->x++;

      if (ApplyWorldCollision(ctx, handle, MD_RIGHT))
      {
        state->id = ACT_BIG_GREEN_CAT_L;
        state->var1 = 10;
      }

      state->x++;

      if (ApplyWorldCollision(ctx, handle, MD_RIGHT))
      {
        state->id = ACT_BIG_GREEN_CAT_L;
        state->var1 = 10;
      }
    }

    state->var2++;

    if (state->var2 == 4)
    {
      state->var2 = 0;
    }

    state->frame = ANIM_SEQ[state->var2];

    if (state->gravityState)
    {
      state->frame = 2;
    }
  }
}


static void Act_Turkey(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  if (!state->var1)
  {
    state->var1 = true;

    if (state->x < ctx->plPosX)
    {
      state->var2 = ORIENTATION_RIGHT;
    }
  }

  if (state->var2 == ORIENTATION_RIGHT)
  {
    state->x++;

    if (ApplyWorldCollision(ctx, handle, MD_RIGHT))
    {
      state->var2 = ORIENTATION_LEFT;
    }
    else
    {
      state->frame = (byte)(ctx->gfxCurrentDisplayPage + 2);
    }
  }
  else if (state->var2 == ORIENTATION_LEFT)
  {
    state->x--;

    if (ApplyWorldCollision(ctx, handle, MD_LEFT))
    {
      state->var2 = ORIENTATION_RIGHT;
    }
    else
    {
      state->frame = (byte)(ctx->gfxCurrentDisplayPage);
    }
  }
  else // cooked turkey
  {
    state->frame = (byte)(state->var3++ % 4 + 4);
  }
}


static void Act_GreenBird(Context* ctx, word handle)
{
  // [PERF] Missing `static` causes a copy operation here
  const word ANIM_SEQ[] = {0, 1, 2, 1};

  ActorState* state = ctx->gmActorStates + handle;

  // Orient towards player on first update
  if (!state->var3)
  {
    if (state->x > ctx->plPosX)
    {
      state->var1 = 0;
    }
    else
    {
      state->var1 = 3;
    }

    state->var3 = true;
  }

  // Fly and switch orientation when hitting a wall
  if (state->var1)
  {
    state->x++;

    if (ApplyWorldCollision(ctx, handle, MD_RIGHT))
    {
      state->var1 = 0;
    }
  }
  else
  {
    state->x--;

    if (ApplyWorldCollision(ctx, handle, MD_LEFT))
    {
      state->var1 = 3;
    }
  }

  // Animate
  state->var2++;
  state->frame = (byte)(state->var1 + ANIM_SEQ[state->var2 % 4]);
}


static void Act_RedBird(Context* ctx, word handle)
{
  // [PERF] Missing `static` causes a copy operation here
  const word FLY_ANIM_SEQ[] = {0, 1, 2, 1};

  ActorState* state = ctx->gmActorStates + handle;

  // Switch to attacking state when above player
  if (
    state->var1 != 2 && state->y + 2 < ctx->plPosY && state->x > ctx->plPosX &&
    state->x < ctx->plPosX + 2)
  {
    state->var1 = 2;
  }

  if (state->var1 == ORIENTATION_RIGHT) // Fly right
  {
    state->x++;

    if (ApplyWorldCollision(ctx, handle, MD_RIGHT))
    {
      state->var1 = ORIENTATION_LEFT;
    }
    else
    {
      state->var2++;

      state->frame = (byte)(3 + FLY_ANIM_SEQ[state->var2 % 4]);
    }
  }

  if (state->var1 == ORIENTATION_LEFT) // Fly left
  {
    state->x--;

    if (ApplyWorldCollision(ctx, handle, MD_LEFT))
    {
      state->var1 = ORIENTATION_RIGHT;
      return;
    }
    else
    {
      state->var2++;

      state->frame = (byte)FLY_ANIM_SEQ[state->var2 % 4];
    }
  }

  if (state->var1 != 2)
  {
    return;
  }

  if (state->var3 < 7) // Hover above player
  {
    // Store original height so we can rise back up there after plunging down
    // onto the player
    state->var4 = state->y;

    state->frame = (byte)(6 + ctx->gfxCurrentDisplayPage);

    state->var3++;
  }
  else if (state->var3 == 7) // Plunge down
  {
    // On the first frame in this state, we don't want the if-statement below
    // to be true, so we set gravityState to 1 here. It will be set to 0 by
    // the engine once we reach the ground.
    // A side-effect of this is that we start falling one frame sooner than
    // usual.
    if (!state->gravityAffected)
    {
      state->gravityState = 1;
    }

    // Start falling
    state->gravityAffected = true;

    state->frame = 6;

    if (state->gravityState == 0) // Reached the ground?
    {
      state->var3 = 8;

      state->gravityAffected = false;
    }
  }
  else if (state->var3 == 8) // Rise back up to original height
  {
    state->frame = (byte)(6 + ctx->gfxCurrentDisplayPage);

    if (state->var4 < state->y)
    {
      state->y--;
    }
    else
    {
      state->var3 = 9;
    }
  }
  else if (state->var3 == 9) // Return to flying
  {
    // Semi-randomly fly either left or right
    state->var1 = ctx->gfxCurrentDisplayPage;
    state->var3 = 0;
  }
}


static void Act_Elevator(Context* ctx, word handle)
{
  register ActorState* state = ctx->gmActorStates + handle;
  register int16_t i;

  if (
    ctx->plState == PS_DYING || ctx->plState == PS_AIRLOCK_DEATH_L ||
    ctx->plState == PS_AIRLOCK_DEATH_R)
  {
    return;
  }

  if (state->var5)
  {
    state->var5 = false;

    state->tileBuffer = MM_PushChunk(ctx, 2 * sizeof(word), CT_TEMPORARY);
    if (!state->tileBuffer)
    {
      return;
    }

    state->scoreGiven = FindFullySolidTileIndex(ctx);

    for (i = 0; i < 2; i++)
    {
      state->tileBuffer[i] = Map_GetTile(ctx, state->x + i + 1, state->y - 2);
    }
  }

  if (state->var4)
  {
    if (!CheckWorldCollision(
          ctx, MD_DOWN, ACT_ELEVATOR, 0, state->x, state->y + 1))
    {
      state->y++;
    }
    else
    {
      state->var4 = 0;
    }

    if (!CheckWorldCollision(
          ctx, MD_DOWN, ACT_ELEVATOR, 0, state->x, state->y + 1))
    {
      state->y++;
    }
    else
    {
      state->var4 = 0;
    }

    if (!state->var4)
    {
      for (i = 0; i < 2; i++)
      {
        state->tileBuffer[i] = Map_GetTile(ctx, state->x + i + 1, state->y - 2);
      }
    }
  }
  else
  {
    if (
      state->y - 3 == ctx->plPosY &&
      ((ctx->plActorId == ACT_DUKE_R && state->x <= ctx->plPosX &&
        state->x + 2 > ctx->plPosX) ||
       (ctx->plActorId == ACT_DUKE_L && state->x - 1 <= ctx->plPosX &&
        state->x >= ctx->plPosX)))
    {
      ctx->plOnElevator = true;

      ShowTutorial(ctx, TUT_ELEVATOR);

      if (
        ctx->inputMoveUp &&
        !CheckWorldCollision(
          ctx, MD_UP, ACT_ELEVATOR, 0, state->x, state->y - 6))
      {
        state->var1 = 1;

        if (ctx->gfxCurrentDisplayPage)
        {
          PlaySound(ctx, SND_FLAMETHROWER_SHOT);
        }

        for (i = 0; i < 2; i++)
        {
          Map_SetTile(
            ctx, state->tileBuffer[i], state->x + i + 1, state->y - 2);
        }

        if (!CheckWorldCollision(
              ctx, MD_UP, ACT_ELEVATOR, 0, state->x, state->y - 6))
        {
          state->y--;
          ctx->plPosY--;
          ctx->plState = PS_RIDING_ELEVATOR;
        }

        if (!CheckWorldCollision(
              ctx, MD_UP, ACT_ELEVATOR, 0, state->x, state->y - 6))
        {
          state->y--;
          ctx->plPosY--;
          ctx->plState = PS_RIDING_ELEVATOR;
        }

        for (i = 0; i < 2; i++)
        {
          state->tileBuffer[i] =
            Map_GetTile(ctx, state->x + i + 1, state->y - 2);
        }

        goto drawFlame;
      }
      else if (
        ctx->inputMoveDown &&
        !CheckWorldCollision(
          ctx, MD_DOWN, ACT_ELEVATOR, 0, state->x, state->y + 1))
      {
        state->var1 = 0;

        for (i = 0; i < 2; i++)
        {
          Map_SetTile(
            ctx, state->tileBuffer[i], state->x + i + 1, state->y - 2);
        }

        if (!CheckWorldCollision(
              ctx, MD_DOWN, ACT_ELEVATOR, 0, state->x, state->y + 1))
        {
          state->y++;
          ctx->plPosY++;
          ctx->plState = PS_RIDING_ELEVATOR;
        }

        if (!CheckWorldCollision(
              ctx, MD_DOWN, ACT_ELEVATOR, 0, state->x, state->y + 1))
        {
          state->y++;
          ctx->plPosY++;
          ctx->plState = PS_RIDING_ELEVATOR;
        }

        for (i = 0; i < 2; i++)
        {
          state->tileBuffer[i] =
            Map_GetTile(ctx, state->x + i + 1, state->y - 2);
        }

        goto drawFlame;
      }
      else
      {
        if (CheckWorldCollision(
              ctx, MD_DOWN, ACT_ELEVATOR, 0, state->x, state->y + 1))
        {
          ctx->plOnElevator = false;
        }

        if (ctx->inputJump)
        {
          ctx->plOnElevator = false;
          state->var1 = 3;
        }
        else
        {
          ctx->plState = PS_NORMAL;
          state->var1 = 3;
        }
      }
    }
    else if (!state->var4)
    {
      ctx->plOnElevator = false;

      if (!CheckWorldCollision(
            ctx, MD_DOWN, ACT_ELEVATOR, 0, state->x, state->y + 1))
      {
        for (i = 0; i < 2; i++)
        {
          Map_SetTile(
            ctx, state->tileBuffer[i], state->x + i + 1, state->y - 2);
        }

        state->var4 = 1;

        goto drawHandrail;
      }
    }

    for (i = 0; i < 2; i++)
    {
      Map_SetTile(ctx, state->scoreGiven, state->x + i + 1, state->y - 2);
    }

  drawFlame:
    if (
      state->var1 &&
      !CheckWorldCollision(
        ctx, MD_DOWN, ACT_ELEVATOR, 0, state->x, state->y + 1))
    {
      DrawActor(
        ctx,
        ACT_ELEVATOR,
        state->var1 + ctx->gfxCurrentDisplayPage,
        state->x,
        state->y,
        DS_NORMAL);
    }
  }

drawHandrail:
  DrawActor(ctx, ACT_ELEVATOR, 5, state->x, state->y, DS_NORMAL);
}


static void Act_SmashHammer(Context* ctx, word handle)
{
  register ActorState* state = ctx->gmActorStates + handle;

  if (state->var1 < 20) // waiting
  {
    if (!state->var1 && !IsActorOnScreen(ctx, handle))
    {
      return;
    }

    state->var1++;

    if (state->var1 == 20)
    {
      // Start lowering
      state->var3 = 1;
    }
  }

  if (state->var3 == 1) // lowering
  {
    register word i;

    state->var2++;
    state->y++;

    // Draw the shaft
    for (i = 0; i < state->var2; i++)
    {
      DrawActor(ctx, ACT_SMASH_HAMMER, 1, state->x, state->y - i, DS_NORMAL);
    }

    if (ApplyWorldCollision(ctx, handle, MD_DOWN))
    {
      SpawnEffect(ctx, ACT_SMOKE_CLOUD_FX, state->x, state->y + 4, EM_NONE, 0);
      PlaySound(ctx, SND_HAMMER_SMASH);

      // Start raising
      state->var3 = 2;
    }
  }
  else if (state->var3 == 2) // raising
  {
    register word i;

    state->var2--;
    state->y--;

    // Draw the shaft
    for (i = 0; i < state->var2; i++)
    {
      DrawActor(
        ctx, ACT_SMASH_HAMMER, 1, state->x, state->y - i + 1, DS_NORMAL);
    }

    // Switch to waiting state
    if (state->var2 == 1)
    {
      state->var1 = 0;
      state->var3 = 0;
      state->var2 = 0;
    }
  }
}


static void Act_WaterArea(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  state->drawStyle = DS_INVISIBLE;

  if (state->var1) // water surface or 4x4 block
  {
    state->var1++;
    if (state->var1 == 5)
    {
      state->var1 = 1;
    }

    DrawWaterArea(ctx, state->x, state->y, state->var1);

    if (state->var2) // 4x4 block
    {
      DrawWaterArea(ctx, state->x + 2, state->y, state->var1);
      DrawWaterArea(ctx, state->x, state->y + 2, 0);
      DrawWaterArea(ctx, state->x + 2, state->y + 2, 0);
    }
  }
  else // 1x1 block
  {
    DrawWaterArea(ctx, state->x, state->y, 0);
  }
}


static void Act_WaterDrop(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  // Once the water drop reaches solid ground, it deletes itself
  if (!state->gravityState)
  {
    state->deleted = true;
  }
}


static void Act_WaterDropSpawner(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  state->drawStyle = DS_INVISIBLE;

  if (ctx->gfxCurrentDisplayPage && RandomNumber(ctx) > 220)
  {
    SpawnActor(ctx, ACT_WATER_DROP, state->x, state->y);

    // [NOTE] This could be simplified to PlaySoundIfOnScreen().
    if (IsActorOnScreen(ctx, handle))
    {
      PlaySound(ctx, SND_WATER_DROP);
    }
  }
}


static void Act_LavaFountain(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  // This table is a list of lists of pairs of (animationFrame, yOffset).
  // Each sub-list is terminated by a value of 127.
  // The list as a whole is terminated by -127.
  //
  // [PERF] Missing `static` causes a copy operation here
  // clang-format off
  const sbyte SPRITE_PLACEMENT_TABLE[] = {
    3, 0, 127,
    4, -3, 1, 1, 127,
    5, -6, 2, -2, 0, 2, 127,
    3, -8, 0, -4, 1, 0, 127,
    4, -9, 1, -5, 2, -1, 0, 3, 127,
    5, -10, 2, -6, 0, -2, 1, 2, 127,
    3, -9, 0, -5, 1, -1, 2, 3, 127,
    3, -8, 0, -4, 1, 0, 127,
    4, -6, 1, -2, 2, 2, 127,
    5, -3, 2, 1, 127,
    3, 0, 127,
    -127
  };
  // clang-format on

  state->drawStyle = DS_INVISIBLE;

  if (state->var1 < 15)
  {
    state->var1++;
    return;
  }

  while (SPRITE_PLACEMENT_TABLE[state->var2] != 127)
  {
    DrawActor(
      ctx,
      ACT_LAVA_FOUNTAIN,
      SPRITE_PLACEMENT_TABLE[state->var2],
      state->x,
      state->y + SPRITE_PLACEMENT_TABLE[state->var2 + 1],
      DS_NORMAL);

    // Since we use drawStyle DS_INVISIBLE, we have to test for intersection
    // with the player manually.
    if (AreSpritesTouching(
          ctx,
          ACT_LAVA_FOUNTAIN,
          SPRITE_PLACEMENT_TABLE[state->var2],
          state->x,
          state->y + SPRITE_PLACEMENT_TABLE[state->var2 + 1],
          ctx->plActorId,
          ctx->plAnimationFrame,
          ctx->plPosX,
          ctx->plPosY))
    {
      DamagePlayer(ctx);
    }

    if (state->var2 < 5)
    {
      PlaySound(ctx, SND_LAVA_FOUNTAIN);
    }

    state->var2 += 2;
  }

  state->var2++;

  if (SPRITE_PLACEMENT_TABLE[state->var2] == -127)
  {
    state->var2 = 0;
    state->var1 = 0;

    if (!IsActorOnScreen(ctx, handle))
    {
      state->alwaysUpdate = false;
      state->remainActive = true;
    }
  }
}


static void Act_RadarComputer(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;
  byte i;

  // [PERF] Missing `static` causes a copy operation here
  const byte RADARS_PRESENT_ANIM_SEQ[] = {4, 4, 4, 0, 4, 4, 4, 0, 4, 4,
                                          4, 0, 5, 5, 5, 5, 5, 5, 5, 5,
                                          5, 5, 5, 5, 5, 5, 5, 5, 5};

  // [PERF] Missing `static` causes a copy operation here
  const byte RADARS_DESTROYED_ANIM_SEQ[] = {6, 6, 6, 0, 6, 6, 6, 0, 6, 6,
                                            6, 0, 7, 7, 7, 7, 7, 7, 7, 7,
                                            7, 7, 7, 7, 7, 7, 7, 7, 7};


  // Draw additional parts (the actor sprite itself is just the screen)
  for (i = 1; i < 4; i++)
  {
    DrawActor(
      ctx, ACT_RADAR_COMPUTER_TERMINAL, i, state->x, state->y, DS_NORMAL);
  }

  if (ctx->gmRadarDishesLeft)
  {
    state->frame = RADARS_PRESENT_ANIM_SEQ[state->var2];

    if (state->frame == 5)
    {
      // We want to draw the sprite for the number of remaining radars on top
      // of the screen sprite, but the actor itself is drawn after calling the
      // update function. Thus, we set the draw style to invisible here and then
      // draw frame 5 manually. This way, we can then draw the number sprite on
      // top.
      state->drawStyle = DS_INVISIBLE;

      DrawActor(
        ctx, ACT_RADAR_COMPUTER_TERMINAL, 5, state->x, state->y, DS_NORMAL);

      DrawActor(
        ctx,
        ACT_RADAR_COMPUTER_TERMINAL,
        7 + ctx->gmRadarDishesLeft,
        state->x - 1,
        state->y,
        DS_NORMAL);
    }

    // [NOTE] This is identical to the code in the else branch, so it could be
    // moved to below the if/else to avoid some code duplication.
    if (ctx->gfxCurrentDisplayPage)
    {
      state->var2++;
    }

    if (state->var2 == 29)
    {
      state->var2 = 0;
    }
  }
  else // all radars destroyed
  {
    state->frame = RADARS_DESTROYED_ANIM_SEQ[state->var2];

    if (ctx->gfxCurrentDisplayPage)
    {
      state->var2++;
    }

    if (state->var2 == 29)
    {
      state->var2 = 0;
    }
  }
}


static void Act_HintMachine(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  // Draw the hint globe attached to the machine if it has been placed
  if (state->var1)
  {
    DrawActor(
      ctx,
      ACT_SPECIAL_HINT_GLOBE_ICON,
      0,
      state->x + 1,
      state->y - 4,
      DS_NORMAL);
  }
}


static void Act_WindBlownSpiderGenerator(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  state->drawStyle = DS_INVISIBLE;

  if (
    state->y > ctx->plPosY && ((word)RandomNumber(ctx) % 2) &&
    ctx->gfxCurrentDisplayPage)
  {
    SpawnEffect(
      ctx,
      ACT_WINDBLOWN_SPIDER_GENERATOR + (int16_t)RandomNumber(ctx) % 3,
      ctx->gmCameraPosX + (VIEWPORT_WIDTH - 1),
      ctx->gmCameraPosY + (word)RandomNumber(ctx) % 16,

      // either EM_BLOW_IN_WIND or EM_FLY_LEFT
      EM_BLOW_IN_WIND - ((word)RandomNumber(ctx) & 2),
      0);
  }
}


static void Act_UniCycleBot(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  if (state->var1 < 15)
  {
    state->var1++;

    if (state->var1 == 15)
    {
      if (state->x < ctx->plPosX)
      {
        state->var2 = 2;
      }
      else
      {
        state->var2 = 1;
      }

      state->var4 = (word)RandomNumber(ctx) % 32 + 15;
      state->var3 = 0;
    }

    state->frame = (byte)(((word)RandomNumber(ctx) % 2) * 5);
  }
  else
  {
    if (state->var4 != 0)
    {
      state->var4--;
    }

    if (state->var2 == 1)
    {
      state->frame = (byte)(3 + ctx->gfxCurrentDisplayPage);

      if (state->var3 < 10)
      {
        state->var3++;

        if (ctx->gfxCurrentDisplayPage)
        {
          SpawnEffect(
            ctx,
            ACT_SMOKE_PUFF_FX,
            state->x + 1,
            state->y,
            EM_FLY_UPPER_RIGHT,
            0);
        }
      }
      else
      {
        state->x--;
      }

      if (ApplyWorldCollision(ctx, handle, MD_LEFT) || state->var4 == 0)
      {
        state->var1 = 0;
      }
    }

    if (state->var2 == 2)
    {
      state->frame = (byte)(1 + ctx->gfxCurrentDisplayPage);

      if (state->var3 < 10)
      {
        state->var3++;

        if (ctx->gfxCurrentDisplayPage)
        {
          SpawnEffect(
            ctx, ACT_SMOKE_PUFF_FX, state->x, state->y, EM_FLY_UPPER_LEFT, 0);
        }
      }
      else
      {
        state->x++;
      }

      if (ApplyWorldCollision(ctx, handle, MD_RIGHT) || state->var4 == 0)
      {
        state->var1 = 0;
      }
    }
  }
}


static void Act_WallWalker(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  // [PERF] Missing `static` causes a copy operation here
  const byte MOVEMENT_BY_STATE[] = {MD_UP, MD_DOWN, MD_LEFT, MD_RIGHT};

  state->var5 = !state->var5;

  if (state->var5)
  {
    return;
  }

  state->var4 = !state->var4;

  if (state->var3 != 0)
  {
    state->var3--;
  }

  switch (state->var1)
  {
    case 0:
      state->frame = (byte)(state->var4 * 2);

      if (state->frame)
      {
        state->y--;
      }
      break;

    case 1:
      state->frame = (byte)(state->var4 * 2);

      if (!state->frame)
      {
        state->y++;
      }
      break;

    case 2:
      state->frame = (byte)state->var4;

      if (!state->frame)
      {
        state->x--;
      }
      break;

    case 3:
      state->frame = (byte)state->var4;

      if (state->frame)
      {
        state->x++;
      }
      break;
  }

// TODO: Is there a loop that produces the same ASM?
repeat:
  if (
    ApplyWorldCollision(ctx, handle, MOVEMENT_BY_STATE[state->var1]) ||
    state->var3 == 0)
  {
    if (state->var1 < 2)
    {
      state->var1 = (word)RandomNumber(ctx) % 2 + 2;
    }
    else
    {
      state->var1 = (word)RandomNumber(ctx) % 2;
    }

    state->var3 = (word)RandomNumber(ctx) % 32 + 10;

    goto repeat;
  }
}


static void Act_AirlockDeathTrigger(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  state->drawStyle = DS_INVISIBLE;

  if (
    state->id == ACT_AIRLOCK_DEATH_TRIGGER_R &&
    Map_GetTile(ctx, state->x + 3, state->y))
  {
    return;
  }

  if (
    state->id == ACT_AIRLOCK_DEATH_TRIGGER_L &&
    Map_GetTile(ctx, state->x - 3, state->y))
  {
    return;
  }

  state->deleted = true;

  ctx->plAnimationFrame = 8;

  if (state->id == ACT_AIRLOCK_DEATH_TRIGGER_L)
  {
    ctx->plState = PS_AIRLOCK_DEATH_L;
  }
  else
  {
    ctx->plState = PS_AIRLOCK_DEATH_R;
  }

  ctx->plAnimationFrame = 8;
}


static void Act_AggressivePrisoner(Context* ctx, word handle)
{
  // [PERF] Missing `static` causes a copy operation here
  const byte ANIM_SEQ[] = {1, 2, 3, 4, 0};

  ActorState* state = ctx->gmActorStates + handle;

  if (state->var1 != 2) // not dying
  {
    DrawActor(ctx, ACT_AGGRESSIVE_PRISONER, 0, state->x, state->y, DS_NORMAL);

    // This also makes it so that the actor's actual bounding box doesn't
    // collide with the player, and thus doesn't cause any damage on touch
    state->drawStyle = DS_INVISIBLE;
  }
  else
  {
    // Death animation
    state->var3++;

    if (state->var3 & 2)
    {
      state->frame++;
    }

    if (state->frame == 8)
    {
      state->deleted = true;
    }

    return;
  }

  // Do we want to try grabbing the player?
  if (
    state->x - 4 < ctx->plPosX && state->x + 7 > ctx->plPosX && !state->var1 &&
    (RandomNumber(ctx) & 0x10) && ctx->gfxCurrentDisplayPage)
  {
    state->var2 = 0;
    state->var1 = 1;
  }

  if (state->var1 == 1) // grabbing animation
  {
    // Enable collision with the player and player shots
    state->drawStyle = DS_NORMAL;

    state->frame = ANIM_SEQ[state->var2];

    if (state->var2 == 4)
    {
      // grabbing done, go back to regular state
      state->var1 = 0;
    }
    else if (ctx->gfxCurrentDisplayPage)
    {
      state->var2++;
    }
  }
}


static void Act_ExplosionTrigger(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  state->drawStyle = DS_INVISIBLE;

  SpawnEffect(ctx, ACT_EXPLOSION_FX_1, state->x, state->y, EM_NONE, 0);
  SpawnEffect(ctx, ACT_EXPLOSION_FX_1, state->x - 1, state->y - 2, EM_NONE, 1);
  SpawnEffect(ctx, ACT_EXPLOSION_FX_1, state->x + 1, state->y - 3, EM_NONE, 2);

  state->deleted = true;
}


static void UpdateBossDeathSequence(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  if (state->var3 == 2)
  {
    StopMusic(ctx);
    GiveScore(ctx, 50000);
    state->gravityAffected = false;
  }

  if (state->var3 == 50)
  {
    FLASH_SCREEN(SFC_WHITE);
    PlaySound(ctx, SND_BIG_EXPLOSION);
  }

  // Rise up
  if (state->var3 > 50 && state->y > 3)
  {
    state->y -= 2;
  }

  if (state->var3 == 60)
  {
    ctx->gmGameState = GS_EPISODE_FINISHED;
    return;
  }

  switch (state->var3)
  {
    case 1:
    case 3:
    case 7:
    case 14:
    case 16:
    case 21:
    case 25:
    case 27:
    case 30:
    case 32:
    case 36:
    case 40:
    case 43:
    case 48:
    case 50:
      PLAY_EXPLOSION_SOUND();

      SpawnParticles(
        ctx,
        state->x + (word)RandomNumber(ctx) % 4,
        state->y - (word)RandomNumber(ctx) % 8,
        (word)RandomNumber(ctx) % 2 - 1,
        (word)RandomNumber(ctx) % 16);
      SpawnEffect(
        ctx,
        ACT_EXPLOSION_FX_1,
        state->x + (word)RandomNumber(ctx) % 4,
        state->y - (word)RandomNumber(ctx) % 8,
        EM_NONE,
        0);
      SpawnEffect(
        ctx,
        ACT_FLAME_FX,
        state->x + (word)RandomNumber(ctx) % 4,
        state->y - (word)RandomNumber(ctx) % 8,
        EM_FLY_DOWN,
        0);
  }

  if (state->var3 < 50)
  {
    if (ctx->gfxCurrentDisplayPage)
    {
      state->drawStyle = DS_INVISIBLE;
    }

    if ((RandomNumber(ctx) & 4) && ctx->gfxCurrentDisplayPage)
    {
      FLASH_SCREEN(SFC_WHITE);
      PlaySound(ctx, SND_BIG_EXPLOSION);
    }
    else
    {
      PLAY_EXPLOSION_SOUND();
    }
  }

  state->var3++;
}


static void Act_Boss1(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  // [PERF] Missing `static` causes a copy operation here
  sbyte Y_MOVEMENT_SEQ[] = {-1, -1, 0, 0, 1, 1, 1, 0, 0, -1};

  // Animate the ship
  state->frame = (byte)ctx->gfxCurrentDisplayPage;

  if (state->var3 > 1) // Dying
  {
    UpdateBossDeathSequence(ctx, handle);
  }
  else if (state->var1 == 0) // First activation
  {
    state->var1 = 3;
    state->var5 = state->y;
    state->var4 = 0;

    ctx->gmBossActivated = true;
  }
  else if (state->var1 == 1) // Plunge down onto player
  {
    if (state->var4 == 2)
    {
      state->var4 = 0;
      state->var1 = 3;
      state->gravityAffected = false;
    }
    else
    {
      if (!state->gravityAffected)
      {
        state->gravityAffected = true;
        state->gravityState = 0;

        state->var4++;
      }

      if (CheckWorldCollision(
            ctx, MD_DOWN, ACT_BOSS_EPISODE_1, 0, state->x, state->y + 1))
      {
        state->gravityAffected = false;

        state->var1 = 2;

        PlaySound(ctx, SND_HAMMER_SMASH);
      }
    }
  }
  else if (state->var1 == 2)
  {
    if (state->y > state->var5)
    {
      state->y--;
    }
    else
    {
      state->var1 = 3;
    }
  }
  else if (state->var1 == 11)
  {
    if (state->y > state->var5)
    {
      state->y--;
    }
    else
    {
      state->var1 = 5;
      state->y--;
    }
  }
  else if (state->var1 == 3)
  {
    state->x -= 2;

    if (CheckWorldCollision(
          ctx, MD_LEFT, ACT_BOSS_EPISODE_1, 0, state->x - 2, state->y))
    {
      state->var1 = 4;
    }
  }
  else if (state->var1 == 4)
  {
    if (ctx->gfxCurrentDisplayPage)
    {
      SpawnActor(ctx, ACT_MINI_NUKE_SMALL, state->x + 3, state->y + 1);
    }

    state->x += 2;

    if (CheckWorldCollision(
          ctx, MD_RIGHT, ACT_BOSS_EPISODE_1, 0, state->x + 2, state->y))
    {
      state->var1 = 7;
    }
  }
  else if (state->var1 == 7)
  {
    state->var1 = 8;

    state->gravityAffected = true;
    state->gravityState = 0;
  }
  else if (state->var1 == 8)
  {
    if (CheckWorldCollision(
          ctx, MD_DOWN, ACT_BOSS_EPISODE_1, 0, state->x, state->y + 1))
    {
      state->gravityAffected = false;
    }

    if (!state->gravityAffected)
    {
      state->x -= 2;

      if (CheckWorldCollision(
            ctx, MD_LEFT, ACT_BOSS_EPISODE_1, 0, state->x - 2, state->y))
      {
        state->var1 = 11;
      }
    }
  }
  else if (state->var1 == 5)
  {
    if (state->var3)
    {
      if (!CheckWorldCollision(
            ctx, MD_RIGHT, ACT_BOSS_EPISODE_1, 0, state->x + 1, state->y))
      {
        state->x++;
      }
      else
      {
        state->var3 = 0;
      }
    }
    else
    {
      if (!CheckWorldCollision(
            ctx, MD_LEFT, ACT_BOSS_EPISODE_1, 0, state->x - 1, state->y))
      {
        state->x--;
      }
      else
      {
        state->var3 = 1;
      }
    }

    state->y += Y_MOVEMENT_SEQ[state->var4++ % 10];

    if (
      state->var4 > 50 && state->x - 1 <= ctx->plPosX &&
      state->x + 9 >= ctx->plPosX)
    {
      state->var4 = 0;
      state->var1 = 1;
    }

    // This does nothing, but is required to produce assembly matching the
    // original
    while (false)
    {
    }
  }

  if (state->var3 < 3)
  {
    // Normal face
    DrawActor(ctx, ACT_BOSS_EPISODE_1, 2, state->x, state->y, DS_NORMAL);
  }
  else if (!ctx->gfxCurrentDisplayPage)
  {
    // Scared face
    DrawActor(ctx, ACT_BOSS_EPISODE_1, 3, state->x, state->y, DS_NORMAL);
  }
}


static void Act_Boss2(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  // This table is a list of groups of 3: x offset, y offset, animation frame.
  //
  // [PERF] Missing `static` causes a copy operation here
  const sbyte FLY_TO_OTHER_SIDE_SEQ[] = {
    0,  1, 2, 0,  1, 2, 1,  2, 3, 1,  2, 3, 2,  1, 3, 2,  1, 3, 2, 0,
    3,  2, 0, 3,  2, 0, 3,  2, 0, 3,  2, 0, 3,  2, 0, 3,  2, 0, 3, 2,
    0,  3, 2, 0,  3, 2, 0,  3, 2, 0,  3, 2, 0,  3, 2, 0,  3, 2, 0, 3,
    2,  0, 3, 2,  0, 3, 2,  0, 3, 2,  0, 3, 2,  0, 3, 2,  0, 3, 2, 0,
    3,  2, 0, 3,  2, 0, 3,  2, 0, 3,  2, 0, 3,  2, 0, 3,  2, 0, 3, 2,
    -1, 3, 2, -1, 3, 1, -2, 3, 1, -2, 3, 0, -1, 3, 0, -1, 3};

  // This table is a list of groups of 3: x offset, y offset, animation frame.
  //
  // [PERF] Missing `static` causes a copy operation here
  const sbyte JUMP_TO_OTHER_SIDE_SEQ[] = {0, -2, 0, 0, -2, 0, 1, -2, 0,
                                          2, -1, 0, 3, 0,  0, 2, 1,  0,
                                          1, 2,  0, 0, 2,  0, 0, 2,  0};

  if (state->var5) // death sequence (var5 is set in HandleActorShotCollision)
  {
    if (state->var5 == 1)
    {
      state->var5++;
      state->var3 = 2;
    }
    else
    {
      UpdateBossDeathSequence(ctx, handle);
    }
  }
  else if (state->var3) // wait
  {
    state->var3--;
    state->frame = (byte)ctx->gfxCurrentDisplayPage;
  }
  else if (state->var1 == 0) // initial wait upon activation
  {
    state->frame = (byte)ctx->gfxCurrentDisplayPage;

    if (state->var2++ == 30)
    {
      state->var1++;

      ctx->gmBossActivated = true;

      state->var2 = 0;
    }
  }
  else if (state->var1 == 1) // fly from left to right
  {
    state->x += FLY_TO_OTHER_SIDE_SEQ[state->var2];
    state->y += FLY_TO_OTHER_SIDE_SEQ[state->var2 + 1];
    state->frame = FLY_TO_OTHER_SIDE_SEQ[state->var2 + 2];

    if (!state->frame)
    {
      state->frame = (byte)ctx->gfxCurrentDisplayPage;
    }

    state->var2 += 3;

    if (state->var2 == 117)
    {
      // Wait a bit, then fly back to the left
      state->var3 = 25;
      state->var1 = 2;
    }
  }
  else if (state->var1 == 2) // fly from right to left
  {
    state->var2 -= 3;

    state->x += -FLY_TO_OTHER_SIDE_SEQ[state->var2];
    state->y += -FLY_TO_OTHER_SIDE_SEQ[state->var2 + 1];
    state->frame = FLY_TO_OTHER_SIDE_SEQ[state->var2 + 2];

    if (!state->frame)
    {
      state->frame = (byte)ctx->gfxCurrentDisplayPage;
    }

    if (state->var2 == 0)
    {
      state->var1 = 3;
      state->gravityState = 0;

      // wait a bit
      state->var3 = 25;
    }
  }
  else if (state->var1 == 3) // fall down
  {
    state->gravityAffected = true;

    if (CheckWorldCollision(
          ctx, MD_DOWN, ACT_BOSS_EPISODE_2, 0, state->x, state->y + 1))
    {
      state->var1 = 4;
      state->var2 = 0;
      state->var3 = 30; // wait
      state->var4 = 0;
      state->gravityAffected = false;
    }
  }
  else if (state->var1 == 4) // jump from left to right
  {
    state->x += JUMP_TO_OTHER_SIDE_SEQ[state->var2];
    state->y += JUMP_TO_OTHER_SIDE_SEQ[state->var2 + 1];
    state->frame = JUMP_TO_OTHER_SIDE_SEQ[state->var2 + 2];

    if (!state->frame)
    {
      state->frame = (byte)ctx->gfxCurrentDisplayPage;
    }

    state->var2 += 3;

    if (state->var2 == 27)
    {
      state->var4++;
      state->var2 = 0;
    }

    if (state->var4 == 8)
    {
      state->var4 = 0;
      state->var2 = 27;
      state->var1 = 5;
    }
  }
  else if (state->var1 == 5) // jump from right to left
  {
    state->var2 -= 3;

    state->x += -JUMP_TO_OTHER_SIDE_SEQ[state->var2];
    state->y += -JUMP_TO_OTHER_SIDE_SEQ[state->var2 + 1];
    state->frame = JUMP_TO_OTHER_SIDE_SEQ[state->var2 + 2];

    if (!state->frame)
    {
      state->frame = (byte)ctx->gfxCurrentDisplayPage;
    }

    if (state->var2 == 0)
    {
      state->var4++;
      state->var2 = 27;
    }

    if (state->var4 == 8)
    {
      state->var1 = 6;
    }
  }
  else if (state->var1 == 6) // rise up
  {
    state->y--;

    if (state->y == state->scoreGiven)
    {
      // Wait, then restart at the beginning
      state->var3 = 100;
      state->var1 = 1;
      state->var2 = 0;
    }
  }
}


static void Boss3_MoveTowardsPos(
  Context* ctx,
  word* x,
  word* y,
  word targetX,
  word targetY)
{
  if (RandomNumber(ctx) & 1) // % 2
  {
    *x += Sign(targetX - *x - 4);
  }

  if (ctx->gfxCurrentDisplayPage)
  {
    *y += Sign(targetY - *y + 4);
  }
}


static void Act_Boss3(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  if (state->var3 > 1)
  {
    UpdateBossDeathSequence(ctx, handle);
    return;
  }

  if (!state->var3)
  {
    ctx->gmBossActivated = true;

    state->var3 = 1;
  }

  Boss3_MoveTowardsPos(
    ctx, &state->x, &state->y, ctx->plPosX + 3, ctx->plPosY - 1);

  // Draw engine exhaust flames
  DrawActor(
    ctx,
    ACT_BOSS_EPISODE_3,
    1 + ctx->gfxCurrentDisplayPage,
    state->x,
    state->y,
    DS_NORMAL);

  //
  // Shoot rockets at player
  //
  if (
    IsActorOnScreen(ctx, handle) && ctx->gfxCurrentDisplayPage &&
    (word)RandomNumber(ctx) % 2)
  {
    // [NOTE] This code is a little convoluted. The actual goal is to figure out
    // if the player is in specific zones around the boss, in order to determine
    // which direction to shoot at. The way the devs decided to implement this
    // is by using AreSpritesTouching, which is not a bad idea since that
    // function basically implements a rectangle intersection test. However,
    // for some reason they decided to use a wrapper function which only takes
    // the actor handle of the boss as argument. This means that the function
    // uses the boss' actual x/y coordinates for testing, which is not what we
    // want. So what the code here does is to temporarily change the position of
    // the boss, then invoke the intersection test, then change it back.
    // What I don't understand is why the helper function wasn't simply given
    // two extra arguments for x and y offset. AreSpritesTouching takes x & y
    // coordinates, so it would be easy enough to adjust the coordinates when
    // passing them as arguments to that function, instead of modifying the
    // actor state.

    // Player left of boss?
    state->x -= 9;

    if (Boss3_IsTouchingPlayer(ctx, handle))
    {
      state->x += 9;

      SpawnActor(ctx, ACT_ENEMY_ROCKET_LEFT, state->x - 4, state->y - 4);
      return;
    }

    // Player right of boss?
    state->x += 18; // first += 9 to undo the -9 above, then +9

    if (Boss3_IsTouchingPlayer(ctx, handle))
    {
      state->x -= 9;

      SpawnActor(ctx, ACT_ENEMY_ROCKET_RIGHT, state->x + 8, state->y - 4);
      return;
    }

    // Player above boss?
    state->x -= 9;
    state->y -= 9;

    if (Boss3_IsTouchingPlayer(ctx, handle))
    {
      state->y += 9;

      SpawnActor(ctx, ACT_ENEMY_ROCKET_2_UP, state->x + 4, state->y - 8);
      return;
    }

    // Player below boss?
    state->y += 18; // first += 9 to undo the -9 above, then +9

    if (Boss3_IsTouchingPlayer(ctx, handle))
    {
      state->y -= 9;

      SpawnActor(ctx, ACT_ENEMY_ROCKET_2_DOWN, state->x + 4, state->y + 3);
      return;
    }

    state->y -= 9;
  }
}


static void Act_Boss4(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  if (!state->var3)
  {
    state->var3 = 1;

    ctx->gmBossActivated = true;
  }

  if (state->var3 > 1)
  {
    UpdateBossDeathSequence(ctx, handle);
    state->drawStyle = DS_INVISIBLE;
    return;
  }

  state->var1++;

  if (!state->var5 && state->var4 < 14 && ctx->gfxCurrentDisplayPage)
  {
    state->var4 = 0;

    if (state->x + 4 > ctx->plPosX)
    {
      state->x--;
      state->var4++;
    }
    else if (state->x + 4 < ctx->plPosX)
    {
      state->x++;
      state->var4++;
    }


    if (state->y + 4 > ctx->plPosY)
    {
      state->y--;
      state->var4++;
    }
    else if (state->y + 4 < ctx->plPosY)
    {
      state->y++;
      state->var4++;
    }
  }

  DrawActor(
    ctx,
    ACT_BOSS_EPISODE_4,
    state->var1 % 4 + 1,
    state->x,
    state->y,
    DS_NORMAL);

  if (state->var5)
  {
    state->var5--;
  }
  else
  {
    if (state->var4)
    {
      state->var2++;

      if (state->var2 > 12)
      {
        state->var4 = 0;

        SpawnActor(ctx, ACT_BOSS_EPISODE_4_SHOT, state->x + 4, state->y + 2);

        state->var2 = 0;
        state->var5 = 12;
      }
    }
  }
}


static void MoveTowardsPos(word* x, word* y, word targetX, word targetY)
{
  *x += Sign(targetX - *x);
  *y += Sign(targetY - *y);
}


static void Act_Boss4Projectile(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  state->var2 = !state->var2;

  if (state->var1 > 3) // Move towards player once animation finished
  {
    state->frame = 4;

    if (RandomNumber(ctx) & 3) // % 4
    {
      MoveTowardsPos(&state->x, &state->y, ctx->plPosX + 1, ctx->plPosY - 1);
    }
  }
  else if (state->var2) // Play appearing animation, advance every other frame
  {
    state->var1++;
    state->frame++;
  }
}


static void Act_SmallFlyingShip(Context* ctx, word handle)
{
  register ActorState* state = ctx->gmActorStates + handle;
  register int16_t i;

  // [PERF] Missing `static` causes a copy operation here
  const byte ANIM_SEQ[] = {0, 1, 2, 1};

  // Explode when hitting a wall, as if shot by the player (gives score)
  if (HAS_TILE_ATTRIBUTE(
        Map_GetTile(ctx, state->x - 1, state->y), TA_SOLID_RIGHT))
  {
    HandleActorShotCollision(ctx, 1, handle);
    return;
  }

  // Determine initial distance to ground on first update
  if (!state->var1)
  {
    for (i = 0; i < 15; i++)
    {
      if (HAS_TILE_ATTRIBUTE(
            Map_GetTile(ctx, state->x, state->y + i), TA_SOLID_TOP))
      {
        state->var1 = i;
        break;
      }
    }
  }

  // Rise up if distance to ground level reduced
  for (i = 0; i < state->var1; i++)
  {
    if (HAS_TILE_ATTRIBUTE(
          Map_GetTile(ctx, state->x, state->y + i), TA_SOLID_TOP))
    {
      state->y--;
      break;
    }
  }

  // Otherwise, float back down
  if (
    i == state->var1 &&
    !HAS_TILE_ATTRIBUTE(Map_GetTile(ctx, state->x, state->y + i), TA_SOLID_TOP))
  {
    state->y++;
  }

  state->x--;

  // Delete ourselves if the player moves away quickly to the left.
  // This is a little weird, and I'm not sure if it's the intended behavior.
  // Basically, the ships disappear when you run away (or better yet, fly away
  // since they only appear in a level where you get Duke's ship) to the left,
  // but when you let a ship fly past you to the left it will remain active
  // until it hits a wall and gives you points, even if off-screen.
  if (!IsActorOnScreen(ctx, handle) && state->x - 20 == ctx->plPosX)
  {
    state->deleted = true;
  }
  else
  {
    // Animate
    state->var2++;

    if (state->var2 == 4)
    {
      state->var2 = 0;
    }

    state->frame = ANIM_SEQ[state->var2];
  }
}


static void Act_RigelatinSoldier(Context* ctx, word handle)
{
  ActorState* state = ctx->gmActorStates + handle;

  const sbyte JUMP_SEQ[] = {-2, -2, -1, 0};

  if (state->var3)
  {
    state->var3--;

    if (state->var3 < 17)
    {
      if (state->var1)
      {
        state->frame = 4;
      }
      else
      {
        state->frame = 0;
      }
    }

    return;
  }

  if (state->var2 == 1)
  {
    state->y += JUMP_SEQ[state->var5];

    state->var5++;

    if (state->var5 == 4)
    {
      state->var2 = 2;

      state->gravityAffected = true;
      state->gravityState = 0;
    }
  }

  if (CheckWorldCollision(
        ctx, MD_DOWN, ACT_RIGELATIN_SOLDIER, 0, state->x, state->y + 1))
  {
    if (state->x < ctx->plPosX)
    {
      state->var1 = 1;
    }
    else
    {
      state->var1 = 0;
    }

    if ((word)RandomNumber(ctx) % 2)
    {
      if (state->var1)
      {
        state->var4++;
      }
      else
      {
        state->var4--;
      }

      if (state->var4 == 0)
      {
        state->var4++;
      }
      else if (state->var4 == 6)
      {
        state->var4--;
      }
      else
      {
        state->gravityAffected = false;

        state->var2 = 1;
        state->gravityState = 0;
        state->var5 = 0;

        goto animateAndAttack;
      }

      if ((word)RandomNumber(ctx) % 2)
      {
        state->var3 = 20;
      }
    }
    else
    {
      state->var3 = 20;
    }
  }
  else // in the air
  {
    if (state->var1)
    {
      if (!CheckWorldCollision(
            ctx, MD_RIGHT, ACT_RIGELATIN_SOLDIER, 0, state->x + 2, state->y))
      {
        state->x += 2;
      }
    }
    else
    {
      if (!CheckWorldCollision(
            ctx, MD_LEFT, ACT_RIGELATIN_SOLDIER, 0, state->x - 2, state->y))
      {
        state->x -= 2;
      }
    }
  }

animateAndAttack:
  if (state->var1)
  {
    state->frame = 4;
  }
  else
  {
    state->frame = 0;
  }

  if (state->var3 == 20)
  {
    state->frame = (byte)(state->frame + 3);

    if (state->var1)
    {
      SpawnEffect(
        ctx,
        ACT_RIGELATIN_SOLDIER_SHOT,
        state->x + 4,
        state->y - 4,
        EM_FLY_RIGHT,
        0);
    }
    else
    {
      SpawnEffect(
        ctx,
        ACT_RIGELATIN_SOLDIER_SHOT,
        state->x,
        state->y - 4,
        EM_FLY_LEFT,
        0);
    }
  }
  else
  {
    if (state->var2 == 1)
    {
      state->frame++;
    }
    else
    {
      state->frame = (byte)(state->frame + 2);
    }
  }
}


/** Spawn a new actor into the game world using given state slot
 *
 * This function determines many properties of the actors, like their update
 * function, how much health they have, their activation policy (always active
 * vs. only when on screen etc.), the initial state of their actor-specific
 * variables, and more.
 */
bool SpawnActorInSlot(Context* ctx, word slot, word id, word x, word y)
{
  // clang-format off
  switch (id)
  {
    case ACT_HOVERBOT:
      InitActorState(
        ctx, slot, Act_Hoverbot, ACT_HOVERBOT, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        ctx->gmDifficulty,        // health
        0,                   // var1
        9,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        150);                // score
      break;

    case ACT_DUKE_L:
    case ACT_DUKE_R:
      InitActorState(
        ctx, slot, Act_PlayerSprite, id, ctx->plPosX = x, ctx->plPosY = y,
        true,                // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      ctx->plActorId = (byte)id;
      break;

    case ACT_ROCKET_LAUNCHER:
      InitActorState(
        ctx, slot, Act_ItemBox, ACT_GREEN_BOX, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        1,                   // health
        0,                   // var1
        ACT_ROCKET_LAUNCHER, // var2
        WPN_ROCKETLAUNCHER,  // var3
        0,                   // var4
        0,                   // var5
        100);                // score
      ctx->gmWeaponsInLevel++;
      break;

    case ACT_FLAME_THROWER:
      InitActorState(
        ctx, slot, Act_ItemBox, ACT_GREEN_BOX, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        1,                   // health
        0,                   // var1
        ACT_FLAME_THROWER,   // var2
        WPN_FLAMETHROWER,    // var3
        0,                   // var4
        0,                   // var5
        100);                // score
      ctx->gmWeaponsInLevel++;
      break;

    case ACT_NORMAL_WEAPON:
      InitActorState(
        ctx, slot, Act_ItemBox, ACT_GREEN_BOX, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        1,                   // health
        0,                   // var1
        ACT_NORMAL_WEAPON,   // var2
        WPN_REGULAR,         // var3
        0,                   // var4
        0,                   // var5
        100);                // score
      ctx->gmWeaponsInLevel++;
      break;

    case ACT_LASER:
      InitActorState(
        ctx, slot, Act_ItemBox, ACT_GREEN_BOX, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        1,                   // health
        0,                   // var1
        ACT_LASER,           // var2
        WPN_LASER,           // var3
        0,                   // var4
        0,                   // var5
        100);                // score
      ctx->gmWeaponsInLevel++;
      break;

    case ACT_FLAME_THROWER_BOT_R:
    case ACT_FLAME_THROWER_BOT_L:
      InitActorState(
        ctx, slot, Act_FlameThrowerBot, id, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        12,                  // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        5000);               // score
      break;

    case ACT_RED_BOX_BOMB:
      InitActorState(
        ctx, slot, Act_ItemBox, ACT_RED_BOX, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        1,                   // health
        0,                   // var1
        ACT_RED_BOX_BOMB,    // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        100);                // score
      ctx->gmBombBoxesLeft++;
      break;

    case ACT_BLUE_BONUS_GLOBE_1:
      InitActorState(
        ctx, slot, Act_BonusGlobe, ACT_BONUS_GLOBE_SHELL, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        1,                   // health
        ACT_BLUE_BONUS_GLOBE_1, // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        500);                // score
      ctx->gmOrbsLeft++;
      break;

    case ACT_BLUE_BONUS_GLOBE_2:
      InitActorState(
        ctx, slot, Act_BonusGlobe, ACT_BONUS_GLOBE_SHELL, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        1,                   // health
        ACT_BLUE_BONUS_GLOBE_2, // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        2000);               // score
      ctx->gmOrbsLeft++;
      break;

    case ACT_BLUE_BONUS_GLOBE_3:
      InitActorState(
        ctx, slot, Act_BonusGlobe, ACT_BONUS_GLOBE_SHELL, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        1,                   // health
        ACT_BLUE_BONUS_GLOBE_3, // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        5000);               // score
      ctx->gmOrbsLeft++;
      break;

    case ACT_BLUE_BONUS_GLOBE_4:
      InitActorState(
        ctx, slot, Act_BonusGlobe, ACT_BONUS_GLOBE_SHELL, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        1,                   // health
        ACT_BLUE_BONUS_GLOBE_4, // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        10000);              // score
      ctx->gmOrbsLeft++;
      break;

    case ACT_WATCHBOT:
      InitActorState(
        ctx, slot, Act_WatchBot, ACT_WATCHBOT, x, y,
        false,               // always update
        true,                // always update once activated
        true,                // allow stair stepping
        false,               // affected by gravity
        ctx->gmDifficulty + 5,    // health
        0,                   // var1
        1,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        1000);               // score
      break;

    case ACT_TELEPORTER_1:
    case ACT_TELEPORTER_2:
      InitActorState(
        ctx, slot, Act_AnimatedProp, ACT_TELEPORTER_2, x, y,
        true,                // always update
        false,               // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        0,                   // health
        4,                   // var1
        id,                  // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_ROCKET_LAUNCHER_TURRET:
      InitActorState(
        ctx, slot, Act_RocketTurret, ACT_ROCKET_LAUNCHER_TURRET, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        3,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        500);                // score
      break;

    case ACT_ENEMY_ROCKET_LEFT:
    case ACT_ENEMY_ROCKET_UP:
    case ACT_ENEMY_ROCKET_RIGHT:
    case ACT_ENEMY_ROCKET_2_UP:
    case ACT_ENEMY_ROCKET_2_DOWN:
      InitActorState(
        ctx, slot, Act_EnemyRocket, id, x, y,
        true,                // always update
        false,               // always update once activated
        true,                // allow stair stepping
        false,               // affected by gravity
        1,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        10);                 // score
      break;

    case ACT_WATCHBOT_CONTAINER_CARRIER:
      InitActorState(
        ctx, slot, Act_WatchBotContainerCarrier, ACT_WATCHBOT_CONTAINER_CARRIER, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        5,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        500);                // score
      break;

    case ACT_WATCHBOT_CONTAINER:
      InitActorState(
        ctx, slot, Act_WatchBotContainer, ACT_WATCHBOT_CONTAINER, x, y,
        true,                // always update
        false,               // always update once activated
        true,                // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        100);                // score
      break;

    case ACT_BOMBER_PLANE:
      InitActorState(
        ctx, slot, Act_BomberPlane, ACT_BOMBER_PLANE, x, y,
        false,               // always update
        true,                // always update once activated
        true,                // allow stair stepping
        false,               // affected by gravity
        ctx->gmDifficulty + 5,    // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        5000);               // score
      break;

    case ACT_MINI_NUKE_SMALL:
      InitActorState(
        ctx, slot, Act_MiniNuke, ACT_MINI_NUKE_SMALL, x, y,
        true,                // always update
        false,               // always update once activated
        true,                // allow stair stepping
        true,                // affected by gravity
        1,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        200);                // score
      break;

    case ACT_MINI_NUKE:
      InitActorState(
        ctx, slot, Act_MiniNuke, ACT_MINI_NUKE, x, y,
        true,                // always update
        false,               // always update once activated
        true,                // allow stair stepping
        true,                // affected by gravity
        1,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        200);                // score
      break;

    case ACT_BOUNCING_SPIKE_BALL:
      InitActorState(
        ctx, slot, Act_SpikeBall, ACT_BOUNCING_SPIKE_BALL, x, y,
        false,               // always update
        true,                // always update once activated
        true,                // allow stair stepping
        false,               // affected by gravity
        ctx->gmDifficulty + 5,    // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        1000);               // score
      break;

    case ACT_ELECTRIC_REACTOR:
      InitActorState(
        ctx, slot, Act_Reactor, ACT_ELECTRIC_REACTOR, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        10,                  // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        20000);              // score
      break;

    case ACT_SLIME_CONTAINER:
      InitActorState(
        ctx, slot, Act_SlimeContainer, ACT_SLIME_CONTAINER, x, y,
        true,                // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        1,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        100);                // score
      break;

    case ACT_SLIME_BLOB:
      InitActorState(
        ctx, slot, Act_SlimeBlob, ACT_SLIME_BLOB, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        ctx->gmDifficulty + 5,    // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        1500);               // score
      break;

    case ACT_NUCLEAR_WASTE:
      InitActorState(
        ctx, slot, Act_ItemBox, ACT_NUCLEAR_WASTE_CAN_EMPTY, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        1,                   // health
        0,                   // var1
        ACT_NUCLEAR_WASTE,   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        200);                // score
      break;

    case ACT_SNAKE:
      InitActorState(
        ctx, slot, Act_Snake, ACT_SNAKE, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        ctx->gmDifficulty + 7,    // health
        0,                   // var1
        0,                   // var2
        1,                   // var3
        0,                   // var4
        0,                   // var5
        5000);               // score
      break;

    case ACT_CAMERA_ON_CEILING:
    case ACT_CAMERA_ON_FLOOR:
      InitActorState(
        ctx, slot, Act_SecurityCamera, id, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        1,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        100);                // score
      ctx->gmCamerasInLevel++;
      break;

    case ACT_CEILING_SUCKER:
      InitActorState(
        ctx, slot, Act_CeilingSucker, ACT_CEILING_SUCKER, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        ctx->gmDifficulty * 3 + 12, // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        300);                // score
      break;

    case ACT_DUKES_SHIP_R:
      InitActorState(
        ctx, slot, Act_PlayerShip, ACT_DUKES_SHIP_R, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_DUKES_SHIP_AFTER_EXITING_L:
      InitActorState(
        ctx, slot, Act_PlayerShip, ACT_DUKES_SHIP_L, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        0,                   // health
        20,                  // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_DUKES_SHIP_AFTER_EXITING_R:
      InitActorState(
        ctx, slot, Act_PlayerShip, ACT_DUKES_SHIP_R, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        0,                   // health
        20,                  // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_MISSILE_BROKEN:
      InitActorState(
        ctx, slot, Act_BrokenMissile, ACT_MISSILE_BROKEN, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        1,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_EYEBALL_THROWER_L:
      InitActorState(
        ctx, slot, Act_EyeBallThrower, ACT_EYEBALL_THROWER_L, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        8,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        2000);               // score
      break;

    case ACT_DYNAMIC_GEOMETRY_1:
      InitActorState(
        ctx, slot, Act_MovingMapPartTrigger, ACT_DYNAMIC_GEOMETRY_1, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        20,                  // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_DYNAMIC_GEOMETRY_2:
      InitActorState(
        ctx, slot, Act_MovingMapPartTrigger, ACT_DYNAMIC_GEOMETRY_2, x, y,
        true,                // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        0,                   // var1
        1,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_HOVERBOT_GENERATOR:
      InitActorState(
        ctx, slot, Act_HoverBotGenerator, ACT_HOVERBOT_GENERATOR, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        20,                  // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        2500);               // score
      break;

    case ACT_DYNAMIC_GEOMETRY_3:
      InitActorState(
        ctx, slot, Act_MovingMapPartTrigger, ACT_DYNAMIC_GEOMETRY_3, x, y,
        true,                // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        0,                   // var1
        2,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_SLIME_PIPE:
      InitActorState(
        ctx, slot, Act_SlimePipe, ACT_SLIME_PIPE, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_SLIME_DROP:
      InitActorState(
        ctx, slot, Act_SlimeDrop, ACT_SLIME_DROP, x, y,
        true,                // always update
        false,               // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        0,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_FORCE_FIELD:
      InitActorState(
        ctx, slot, Act_ForceField, ACT_FORCE_FIELD, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_CIRCUIT_CARD_KEYHOLE:
      InitActorState(
        ctx, slot, Act_KeyCardSlot, ACT_CIRCUIT_CARD_KEYHOLE, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        1,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_BLUE_KEY_KEYHOLE:
      InitActorState(
        ctx, slot, Act_KeyHole, ACT_BLUE_KEY_KEYHOLE, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        1,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_SLIDING_DOOR_VERTICAL:
      InitActorState(
        ctx, slot, Act_SlidingDoorVertical, ACT_SLIDING_DOOR_VERTICAL, x, y,
        true,                // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_RADAR_DISH:
      InitActorState(
        ctx, slot, Act_AnimatedProp, ACT_RADAR_DISH, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        4,                   // health
        12,                  // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        500);                // score
      ctx->gmRadarDishesLeft++;
      break;

    case ACT_KEYHOLE_MOUNTING_POLE:
    case ACT_LASER_TURRET_MOUNTING_POST:
      InitActorState(
        ctx, slot, Act_AnimatedProp, id, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        1,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_BLOWING_FAN:
      InitActorState(
        ctx, slot, Act_BlowingFan, ACT_BLOWING_FAN, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_LASER_TURRET:
      InitActorState(
        ctx, slot, Act_LaserTurret, ACT_LASER_TURRET, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        1,                   // health
        20,                  // var1
        1,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      ctx->gmTurretsInLevel++;
      break;

    case ACT_SLIDING_DOOR_HORIZONTAL:
      InitActorState(
        ctx, slot, Act_SlidingDoorHorizontal, ACT_SLIDING_DOOR_HORIZONTAL, x, y,
        true,                // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_RESPAWN_CHECKPOINT:
      InitActorState(
        ctx, slot, Act_RespawnBeacon, ACT_RESPAWN_CHECKPOINT, x, y,
        true,                // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        0,                   // var1
        1,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_SKELETON:
      InitActorState(
        ctx, slot, Act_Skeleton, ACT_SKELETON, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        ctx->gmDifficulty + 1,    // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_ENEMY_LASER_SHOT_R:
    case ACT_ENEMY_LASER_SHOT_L:
      InitActorState(
        ctx, slot, Act_EnemyLaserShot, ACT_ENEMY_LASER_SHOT_L, x, y,
        true,                // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        id,                  // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_DYNAMIC_GEOMETRY_4:
      InitActorState(
        ctx, slot, Act_MovingMapPartTrigger, ACT_DYNAMIC_GEOMETRY_4, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        2,                   // var1
        3,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_DYNAMIC_GEOMETRY_5:
      InitActorState(
        ctx, slot, Act_MovingMapPartTrigger, ACT_DYNAMIC_GEOMETRY_5, x, y,
        true,                // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        0,                   // var1
        4,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_EXIT_TRIGGER:
      InitActorState(
        ctx, slot, Act_LevelExitTrigger, ACT_EXIT_TRIGGER, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_DYNAMIC_GEOMETRY_6:
      InitActorState(
        ctx, slot, Act_MovingMapPartTrigger, ACT_DYNAMIC_GEOMETRY_6, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        2,                   // var1
        5,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_DYNAMIC_GEOMETRY_7:
      InitActorState(
        ctx, slot, Act_MovingMapPartTrigger, ACT_DYNAMIC_GEOMETRY_7, x, y,
        true,                // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        0,                   // var1
        6,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_DYNAMIC_GEOMETRY_8:
      InitActorState(
        ctx, slot, Act_MovingMapPartTrigger, ACT_DYNAMIC_GEOMETRY_8, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        20,                  // var1
        8,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_SUPER_FORCE_FIELD_L:
      InitActorState(
        ctx, slot, Act_SuperForceField, ACT_SUPER_FORCE_FIELD_L, x, y,
        true,                // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        1,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        3,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_MISSILE_INTACT:
      InitActorState(
        ctx, slot, Act_IntactMissile, ACT_MISSILE_INTACT, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        1,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_METAL_GRABBER_CLAW:
      InitActorState(
        ctx, slot, Act_GrabberClaw, ACT_METAL_GRABBER_CLAW, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        1,                   // var1
        1,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_HOVERING_LASER_TURRET:
      InitActorState(
        ctx, slot, Act_FloatingLaserBot, ACT_HOVERING_LASER_TURRET, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        ctx->gmDifficulty + 2,    // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        1000);               // score
      break;

    case ACT_SPIDER:
      InitActorState(
        ctx, slot, Act_Spider, ACT_SPIDER, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        ctx->gmDifficulty,        // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_HEALTH_MOLECULE:
    case ACT_N:
    case ACT_U:
    case ACT_K:
    case ACT_E:
    case ACT_M:
    case ACT_GAME_CARTRIDGE:
    case ACT_SUNGLASSES:
    case ACT_PHONE:
    case ACT_BOOM_BOX:
    case ACT_DISK:
    case ACT_TV:
    case ACT_CAMERA:
    case ACT_PC:
    case ACT_CD:
    case ACT_T_SHIRT:
    case ACT_VIDEOCASSETTE:
      InitActorState(
        ctx, slot, Act_ItemBox, ACT_BLUE_BOX, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        1,                   // health
        0,                   // var1
        id,                  // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      ctx->gmMerchInLevel++;
      break;

    case ACT_BLUE_GUARD_R:
      InitActorState(
        ctx, slot, Act_BlueGuard, ACT_BLUE_GUARD_R, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        ctx->gmDifficulty + 1,    // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        3000);               // score
      break;

    case ACT_WHITE_BOX:
    case ACT_GREEN_BOX:
    case ACT_RED_BOX:
    case ACT_BLUE_BOX:
      InitActorState(
        ctx, slot, Act_ItemBox, id, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        1,                   // health
        0,                   // var1
        0xFF,                // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        100);                // score
      break;

    case ACT_BLUE_GUARD_L:
      InitActorState(
        ctx, slot, Act_BlueGuard, ACT_BLUE_GUARD_R, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        ctx->gmDifficulty + 1,    // health
        1,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        3000);               // score
      break;

    case ACT_NUCLEAR_WASTE_CAN_EMPTY:
      InitActorState(
        ctx, slot, Act_ItemBox, ACT_NUCLEAR_WASTE_CAN_EMPTY, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        1,                   // health
        0,                   // var1
        0xFF,                // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        100);                // score
      break;

    case ACT_SODA_CAN:
    case ACT_SODA_6_PACK:
      InitActorState(
        ctx, slot, Act_ItemBox, ACT_RED_BOX, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        1,                   // health
        0,                   // var1
        id,                  // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        100);                // score
      break;

    case ACT_ROTATING_FLOOR_SPIKES:
    case ACT_FIRE_ON_FLOOR_1:
    case ACT_FIRE_ON_FLOOR_2:
      InitActorState(
        ctx, slot, Act_AnimatedProp, id, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        4,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_GREEN_CREATURE_L:
    case ACT_GREEN_CREATURE_R:
      InitActorState(
        ctx, slot, Act_SpikedGreenCreature, id, x, y,
        false,               // always update
        true,                // always update once activated
        true,                // allow stair stepping
        false,               // affected by gravity
        5,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        1000);               // score
      break;

    case ACT_BIG_GREEN_CAT_L:
    case ACT_BIG_GREEN_CAT_R:
      InitActorState(
        ctx, slot, Act_GreenPanther, id, x, y,
        false,               // always update
        true,                // always update once activated
        true,                // allow stair stepping
        true,                // affected by gravity
        5,                   // health
        10,                  // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        1000);               // score
      break;

    case ACT_RED_BOX_TURKEY:
      InitActorState(
        ctx, slot, Act_ItemBox, ACT_RED_BOX, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        1,                   // health
        0,                   // var1
        ACT_TURKEY,          // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        100);                // score
      break;

    case ACT_TURKEY:
      InitActorState(
        ctx, slot, Act_Turkey, ACT_TURKEY, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        1,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        100);                // score
      break;

    case ACT_RED_BIRD:
      InitActorState(
        ctx, slot, Act_RedBird, ACT_RED_BIRD, x, y,
        false,               // always update
        true,                // always update once activated
        true,                // allow stair stepping
        false,               // affected by gravity
        ctx->gmDifficulty,        // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_UGLY_GREEN_BIRD:
      InitActorState(
        ctx, slot, Act_GreenBird, ACT_UGLY_GREEN_BIRD, x, y,
        false,               // always update
        true,                // always update once activated
        true,                // allow stair stepping
        false,               // affected by gravity
        2,                   // health
        200,                 // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        x,                   // var5
        y);                  // score
      break;

    case ACT_CIRCUIT_CARD:
    case ACT_RAPID_FIRE:
    case ACT_CLOAKING_DEVICE:
    case ACT_BLUE_KEY:
      InitActorState(
        ctx, slot, Act_ItemBox, ACT_WHITE_BOX, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        1,                   // health
        0,                   // var1
        id,                  // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        100);                // score
      break;

    case ACT_FLOATING_EXIT_SIGN_R:
    case ACT_FLOATING_EXIT_SIGN_L:
    case ACT_FLOATING_ARROW:
      InitActorState(
        ctx, slot, Act_AnimatedProp, id, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        5,                   // health
        2,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_ELEVATOR:
      InitActorState(
        ctx, slot, Act_Elevator, ACT_ELEVATOR, x, y,
        true,                // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        3,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        1,                   // var5
        0);                  // score
      break;

    case ACT_COMPUTER_TERMINAL:
    case ACT_WATER_FALL_SPLASH_L:
    case ACT_WATER_FALL_SPLASH_CENTER:
    case ACT_WATER_FALL_SPLASH_R:
      InitActorState(
        ctx, slot, Act_AnimatedProp, id, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        3,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_LAVA_PIT:
    case ACT_LAVA_FALL_1:
    case ACT_LAVA_FALL_2:
    case ACT_WATER_FALL_1:
    case ACT_WATER_FALL_2:
    case ACT_GREEN_ACID_PIT:
    case ACT_FLAME_JET_1:
    case ACT_FLAME_JET_2:
    case ACT_FLAME_JET_3:
    case ACT_FLAME_JET_4:
    case ACT_WATER_ON_FLOOR_1:
    case ACT_WATER_ON_FLOOR_2:
    case ACT_PASSIVE_PRISONER:
      InitActorState(
        ctx, slot, Act_AnimatedProp, id, x, y,
        true,                // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        4,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_MESSENGER_DRONE_1:
    case ACT_MESSENGER_DRONE_2:
    case ACT_MESSENGER_DRONE_3:
    case ACT_MESSENGER_DRONE_4:
    case ACT_MESSENGER_DRONE_5:
      InitActorState(
        ctx, slot, Act_MessengerDrone, ACT_MESSENGER_DRONE_BODY, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        1,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        id,                  // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_BLUE_GUARD_USING_TERMINAL:
      InitActorState(
        ctx, slot, Act_BlueGuard, ACT_BLUE_GUARD_R, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        ctx->gmDifficulty + 1,    // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        2,                   // var5
        3000);               // score
      break;

    case ACT_SUPER_FORCE_FIELD_R:
      InitActorState(
        ctx, slot, Act_SuperForceField, ACT_SUPER_FORCE_FIELD_L, x, y,
        true,                // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        1,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        4,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_SMASH_HAMMER:
      InitActorState(
        ctx, slot, Act_SmashHammer, ACT_SMASH_HAMMER, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        10,                  // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_WATER_BODY:
      InitActorState(
        ctx, slot, Act_WaterArea, ACT_WATER_BODY, x, y,
        true,                // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_WATER_DROP:
      InitActorState(
        ctx, slot, Act_WaterDrop, ACT_WATER_DROP, x, y,
        true,                // always update
        false,               // always update once activated
        true,                // allow stair stepping
        true,                // affected by gravity
        0,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_WATER_DROP_SPAWNER:
      InitActorState(
        ctx, slot, Act_WaterDropSpawner, ACT_WATER_DROP_SPAWNER, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_LAVA_FOUNTAIN:
      InitActorState(
        ctx, slot, Act_LavaFountain, ACT_LAVA_FOUNTAIN, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_WATER_SURFACE:
      InitActorState(
        ctx, slot, Act_WaterArea, ACT_WATER_BODY, x, y,
        true,                // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        1,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_WATER_AREA_4x4:
      InitActorState(
        ctx, slot, Act_WaterArea, ACT_WATER_BODY, x, y,
        true,                // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        1,                   // var1
        1,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_RADAR_COMPUTER_TERMINAL:
      InitActorState(
        ctx, slot, Act_RadarComputer, ACT_RADAR_COMPUTER_TERMINAL, x, y,
        true,                // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_SPECIAL_HINT_GLOBE:
      InitActorState(
        ctx, slot, Act_AnimatedProp, ACT_SPECIAL_HINT_GLOBE, x, y,
        false,               // always update
        true,                // always update once activated
        true,                // allow stair stepping
        false,               // affected by gravity
        3,                   // health
        6,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        100);                // score
      break;

    case ACT_SPECIAL_HINT_MACHINE:
      InitActorState(
        ctx, slot, Act_HintMachine, ACT_SPECIAL_HINT_MACHINE, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_WINDBLOWN_SPIDER_GENERATOR:
      InitActorState(
        ctx, slot, Act_WindBlownSpiderGenerator, ACT_WINDBLOWN_SPIDER_GENERATOR, x, y,
        true,                // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_UNICYCLE_BOT:
      InitActorState(
        ctx, slot, Act_UniCycleBot, ACT_UNICYCLE_BOT, x, y,
        false,               // always update
        true,                // always update once activated
        true,                // allow stair stepping
        true,                // affected by gravity
        2,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        300);                // score
      break;

    case ACT_WALL_WALKER:
      InitActorState(
        ctx, slot, Act_WallWalker, ACT_WALL_WALKER, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        2,                   // health
        (word)RandomNumber(ctx) % 4,  // var1
        0,                   // var2
        20,                  // var3
        0,                   // var4
        0,                   // var5
        100);                // score
      break;

    case ACT_AIRLOCK_DEATH_TRIGGER_L:
    case ACT_AIRLOCK_DEATH_TRIGGER_R:
      InitActorState(
        ctx, slot, Act_AirlockDeathTrigger, id, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_AGGRESSIVE_PRISONER:
      InitActorState(
        ctx, slot, Act_AggressivePrisoner, ACT_AGGRESSIVE_PRISONER, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        1,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        100);                // score
      break;

    case ACT_EXPLOSION_FX_TRIGGER:
      InitActorState(
        ctx, slot, Act_ExplosionTrigger, ACT_EXPLOSION_FX_TRIGGER, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        0,                   // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_BOSS_EPISODE_1:
      InitActorState(
        ctx, slot, Act_Boss1, ACT_BOSS_EPISODE_1, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        ctx->gmBossHealth = ctx->gmDifficulty * 20 + 90, // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_BOSS_EPISODE_2:
      InitActorState(
        ctx, slot, Act_Boss2, ACT_BOSS_EPISODE_2, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        ctx->gmBossHealth = ctx->gmDifficulty * 20 + 90, // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        y);                  // score
      break;

    case ACT_BOSS_EPISODE_3:
      InitActorState(
        ctx, slot, Act_Boss3, ACT_BOSS_EPISODE_3, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        ctx->gmBossHealth = ctx->gmDifficulty * 75 + 600, // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_BOSS_EPISODE_4:
      InitActorState(
        ctx, slot, Act_Boss4, ACT_BOSS_EPISODE_4, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        ctx->gmBossHealth = ctx->gmDifficulty * 40 + 100, // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_SMALL_FLYING_SHIP_1:
    case ACT_SMALL_FLYING_SHIP_2:
    case ACT_SMALL_FLYING_SHIP_3:
      InitActorState(
        ctx, slot, Act_SmallFlyingShip, id, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        ctx->gmDifficulty + 1,    // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        1000);               // score
      break;

    case ACT_BOSS_EPISODE_4_SHOT:
      InitActorState(
        ctx, slot, Act_Boss4Projectile, ACT_BOSS_EPISODE_4_SHOT, x, y,
        true,                // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        1,                   // health
        0,                   // var1
        1,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        100);                // score
      break;

    case ACT_RIGELATIN_SOLDIER:
      InitActorState(
        ctx, slot, Act_RigelatinSoldier, ACT_RIGELATIN_SOLDIER, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        ctx->gmDifficulty * 2 + 25, // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        3,                   // var4
        0,                   // var5
        2000);               // score
      break;


    default:
      return false;
  }
  // clang-format on

  return true;
}

RIGEL_RESTORE_WARNINGS
