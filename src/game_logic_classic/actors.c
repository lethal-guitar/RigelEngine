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

Actor control logic and actor property definitions

This file contains the behavior control functions for all actors, and a huge
switch statement which assigns these functions to actors based on their IDs
as well as defining various properties like amount of health, score given
when destroyed, etc.

This represents the largest part of the game logic by far.

*******************************************************************************/


/** Semi-generic utility actor
 *
 * This function is used for a variety of actors which feature animated sprites
 * but don't need any other behavior otherwise.
 *
 * In general, the animation repeats from frame 0 to the value of var1,
 * advancing by one animation frame each game frame. But there are also a few
 * special cases for specific types of actors.
 */
void pascal Act_AnimatedProp(word handle)
{
  ActorState* state = gmActorStates + handle;

  if (state->id == ACT_PASSIVE_PRISONER)
  {
    state->frame = !((word)RandomNumber() & 4);
  }
  else if (state->id == ACT_SPECIAL_HINT_GLOBE)
  {
    // [PERF] Missing `static` causes a copy operation here
    const byte HINT_GLOBE_ANIMATION[] = {
      0, 1, 2, 3, 4, 5, 4, 5, 4, 5, 4, 5, 4, 3, 2, 1,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

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
      state->id == ACT_WATER_ON_FLOOR_1 ||
      state->id == ACT_WATER_ON_FLOOR_2 ||
      state->id == ACT_ROTATING_FLOOR_SPIKES)
    {
      // Advance one frame every other game frame (half speed)
      state->frame = state->frame + gfxCurrentDisplayPage;
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


void pascal Act_Hoverbot(word handle)
{
  ActorState* state = gmActorStates + handle;

  if (state->var5) // wait before starting to move (after teleport animation)
  {
    state->var5--;

    // Draw the eye
    DrawActor(ACT_HOVERBOT, state->var4 + 6, state->x, state->y, DS_NORMAL);
  }
  else if (state->var2 <= 9 && state->var2 > 1) // teleport animation
  {
    if (state->var2 == 8)
    {
      // The effect is player-damaging
      SpawnEffect(ACT_HOVERBOT_TELEPORT_FX, state->x, state->y, EM_NONE, 0);
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

  if (state->var5) { return; } // if in initial wait state, we're done here

  if (state->var3 == 0) // moving
  {
    if (state->var1) // moving right
    {
      state->x++;
      ApplyWorldCollision(handle, MD_RIGHT);

      if (state->x > plPosX)
      {
        // switch to "turning left"
        state->var3 = 1;
        state->var4 = 5;
      }
    }
    else // moving left
    {
      --state->x;
      ApplyWorldCollision(handle, MD_LEFT);

      if (state->x < plPosX)
      {
        // switch to "turning right"
        state->var3 = 2;
        state->var4 = 0;
      }
    }
  }

  if (state->var3 == 1) // turning left
  {
    if (gfxCurrentDisplayPage)
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
    if (gfxCurrentDisplayPage)
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
  DrawActor(ACT_HOVERBOT, state->var4 + 6, state->x, state->y, DS_NORMAL);
}


void pascal Act_PlayerSprite(word handle)
{
  ActorState* state = gmActorStates + handle;

  // Synchronize player sprite to player state
  state->x = plPosX;
  state->y = plPosY;
  state->id = plActorId;
  state->frame = plAnimationFrame;

  if (plState == PS_AIRLOCK_DEATH_L || plState == PS_AIRLOCK_DEATH_R)
  {
    return;
  }

  // Kill the player if fallen out of the map (bottom-less pit)
  if (plPosY - 4 > mapBottom && plPosY - 4 < mapBottom + 10)
  {
    gmGameState = GS_PLAYER_DIED;
    PlaySound(SND_DUKE_DEATH);
    return;
  }

  //
  // Cloaking device effect and timer
  //
  if (plCloakTimeLeft)
  {
    state->drawStyle = DS_TRANSLUCENT;

    --plCloakTimeLeft;

    if (plCloakTimeLeft == 30)
    {
      ShowInGameMessage("CLOAK IS DISABLING...");
    }

    // Make player flash when cloak is disabling
    if (plCloakTimeLeft < 30 && gfxCurrentDisplayPage)
    {
      state->drawStyle = DS_WHITEFLASH;
    }

    if (plCloakTimeLeft == 0)
    {
      RemoveFromInventory(ACT_CLOAKING_DEVICE_ICON);
      SpawnActor(ACT_CLOAKING_DEVICE, gmCloakPickupPosX, gmCloakPickupPosY);
    }
  }

  //
  // Rapid fire powerup timer
  //
  if (plRapidFireTimeLeft)
  {
    --plRapidFireTimeLeft;

    if (plRapidFireTimeLeft == 30)
    {
      ShowInGameMessage("RAPID FIRE IS DISABLING...");
    }

    if (plRapidFireTimeLeft == 0)
    {
      RemoveFromInventory(ACT_RAPID_FIRE_ICON);
    }
  }

  //
  // Mercy frames (period of invincibility after getting hit)
  //
  if (plMercyFramesLeft)
  {
    if (plMercyFramesLeft & 1) // % 2
    {
      if (plMercyFramesLeft > 10)
      {
        state->drawStyle = DS_INVISIBLE;
      }
      else
      {
        state->drawStyle = DS_WHITEFLASH;
      }
    }

    plMercyFramesLeft--;
  }

  if (plState == PS_GETTING_EATEN || plAnimationFrame == 0xFF)
  {
    state->drawStyle = DS_INVISIBLE;
    plAttachedSpider1 = plAttachedSpider2 = plAttachedSpider3 = 0;
  }

  //
  // Additional animation logic
  //

  // Draw exhaust flames when the ship is moving
  if (plState == PS_USING_SHIP && state->drawStyle != 0)
  {
    if (inputMoveLeft && inputMoveRight)
    {
      inputMoveLeft = inputMoveRight = 0;
    }

    if (inputMoveLeft && plActorId == ACT_DUKES_SHIP_L)
    {
      DrawActor(
        ACT_DUKES_SHIP_EXHAUST_FLAMES,
        gfxCurrentDisplayPage + 4,
        plPosX,
        plPosY,
        DS_NORMAL);
    }

    if (inputMoveRight && plActorId == ACT_DUKES_SHIP_R)
    {
      DrawActor(
        ACT_DUKES_SHIP_EXHAUST_FLAMES,
        gfxCurrentDisplayPage + 2,
        plPosX,
        plPosY,
        DS_NORMAL);
    }

    if (inputMoveUp && !inputMoveDown)
    {
      if (plActorId == ACT_DUKES_SHIP_L)
      {
        DrawActor(
          ACT_DUKES_SHIP_EXHAUST_FLAMES,
          gfxCurrentDisplayPage,
          plPosX + 1,
          plPosY,
          DS_NORMAL);
      }

      if (plActorId == ACT_DUKES_SHIP_R)
      {
        DrawActor(
          ACT_DUKES_SHIP_EXHAUST_FLAMES,
          gfxCurrentDisplayPage,
          plPosX,
          plPosY,
          DS_NORMAL);
      }
    }
  }
  else if (plInteractAnimTicks)
  {
    if (plState == PS_NORMAL)
    {
      state->frame = 33;
    }

    plInteractAnimTicks++;
    if (plInteractAnimTicks == 9)
    {
      plInteractAnimTicks = 0;
    }
  }
  else if (plState == PS_RIDING_ELEVATOR)
  {
    state->frame = 33;
  }
  else if (plState == PS_BLOWN_BY_FAN)
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
void pascal Act_ItemBox(word handle)
{
  register ActorState* state = gmActorStates + handle;

  // [PERF] Missing `static` causes a copy operation here
  const sbyte FLY_UP_ARC[] = { -3, -2, -1, 0, 1, 2, 3, -1, 1 };

  if (!state->var1) { return; } // container hasn't been shot yet, stop here

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
      const byte FX_LIST[] = {
        ACT_YELLOW_FIREBALL_FX, EM_FLY_UP,
        ACT_GREEN_FIREBALL_FX, EM_FLY_UPPER_LEFT,
        ACT_BLUE_FIREBALL_FX, EM_FLY_UPPER_RIGHT,
        ACT_GREEN_FIREBALL_FX, EM_FLY_DOWN
      };

      register int i;

      for (i = 0; i < 8; i += 2)
      {
        SpawnEffect(FX_LIST[i], state->x, state->y, FX_LIST[i + 1], 0);
      }

      if ((int)state->var2 == -1) // box is empty
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
      const byte FX_LIST[] = {
        ACT_NUCLEAR_WASTE_CAN_DEBRIS_4, EM_FLY_UP,
        ACT_NUCLEAR_WASTE_CAN_DEBRIS_3, EM_FLY_DOWN,
        ACT_NUCLEAR_WASTE_CAN_DEBRIS_1, EM_FLY_UPPER_LEFT,
        ACT_NUCLEAR_WASTE_CAN_DEBRIS_2, EM_FLY_UPPER_RIGHT,
        ACT_SMOKE_CLOUD_FX, EM_NONE
      };

      register int i;

      for (i = 0; i < 10; i += 2)
      {
        SpawnEffect(FX_LIST[i], state->x, state->y, FX_LIST[i + 1], 0);
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
      SpawnEffect(ACT_NUCLEAR_WASTE, state->x, state->y, EM_NONE, 1);
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

      case 0xFFFF:
        // Empty nuclear waste barrel
        state->deleted = true;
        return;

      case ACT_TURKEY:
        state->deleted = true;
        SpawnActor(ACT_TURKEY, state->x, state->y);
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
      (state->var1 == 9 && !CheckWorldCollision(
        MD_DOWN, state->id, state->frame, state->x, state->y + 1)))
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
      state->frame = gfxCurrentDisplayPage;
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
      if (state->var3 > 24 && gfxCurrentDisplayPage)
      {
        state->drawStyle = DS_WHITEFLASH;
      }

      if (state->var3 == 32)
      {
        // [NOTE] This code is basically the same as in
        // HandleActorShotCollision(), a dedicated function would've been good
        // to reduce code duplication.
        register int i;
        bool spawnFailedLeft = false;
        bool spawnFailedRight = false;

        gmBombBoxesLeft--;

        PLAY_EXPLOSION_SOUND();

        SpawnParticles(state->x + 1, state->y, 0, CLR_WHITE);

        for (i = 0; i < 12; i += 2)
        {
          if (!spawnFailedLeft)
          {
            spawnFailedLeft += SpawnEffect(
              ACT_FIRE_BOMB_FIRE,
              state->x - 2 - i,
              state->y,
              EM_NONE,
              i);
          }

          if (!spawnFailedRight)
          {
            spawnFailedRight += SpawnEffect(
              ACT_FIRE_BOMB_FIRE,
              state->x + i + 2,
              state->y,
              EM_NONE,
              i);
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

        if (CheckWorldCollision(MD_UP, ACT_SODA_CAN, 0, state->x, state->y))
        {
          SpawnEffect(
            ACT_COKE_CAN_DEBRIS_1, state->x, state->y, EM_FLY_LEFT, 0);
          SpawnEffect(
            ACT_COKE_CAN_DEBRIS_2, state->x, state->y, EM_FLY_RIGHT, 0);
          PLAY_EXPLOSION_SOUND();
          state->deleted = true;
          return;
        }

        // Draw the rocket exhaust flame
        DrawActor(
          ACT_SODA_CAN,
          gfxCurrentDisplayPage + 6,
          state->x,
          state->y,
          DS_NORMAL);
      }
      return;

    case ACT_SODA_6_PACK:
      if (state->var3) // has the 6-pack been shot?
      {
        register int i;

        PLAY_EXPLOSION_SOUND();
        state->deleted = true;

        for (i = 0; i < 6; i++)
        {
          SpawnEffect(
            ACT_COKE_CAN_DEBRIS_1,
            state->x + (i & 2),
            state->y + (i & 1),
            i,
            0);
          SpawnEffect(
            ACT_COKE_CAN_DEBRIS_2,
            state->x + (i & 2),
            state->y + (i & 1),
            i,
            0);
        }

        GiveScore(10000);
        SpawnEffect(
          ACT_SCORE_NUMBER_FX_10000, state->x, state->y, EM_SCORE_NUMBER, 0);
      }
  }
}


void pascal Act_FlameThrowerBot(word handle)
{
  ActorState* state = gmActorStates + handle;

  // Randomly decide to stop and shoot fire
  if (!(((int)RandomNumber()) & 127))
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
          ACT_FLAME_THROWER_FIRE_R, state->x + 7, state->y - 3, EM_NONE, 0);
      }
      else
      {
        SpawnEffect(
          ACT_FLAME_THROWER_FIRE_L, state->x - 7, state->y - 3, EM_NONE, 0);
      }
    }
  }
  else
  {
    if (state->var1) // moving up
    {
      if (gfxCurrentDisplayPage)
      {
        state->y--;
        if (ApplyWorldCollision(handle, MD_UP))
        {
          // Start moving down
          state->var1 = 0;
        }
      }
    }
    else // moving down
    {
      state->y++;
      if (ApplyWorldCollision(handle, MD_DOWN))
      {
        // Start moving up
        state->var1 = 1;
      }
    }
  }
}


void pascal Act_BonusGlobe(word handle)
{
  ActorState* state = gmActorStates + handle;

  // Animate and draw the content inside the shell
  state->var2++;
  if (state->var2 == 4)
  {
    state->var2 = 0;
  }

  DrawActor(state->var1, state->var2, state->x, state->y, DS_NORMAL);
}


void pascal Act_WatchBot(word handle)
{
  register ActorState* state = gmActorStates + handle;

  // [PERF] Missing `static` causes a copy operation here
  const word HIDE_HEAD_ANIM[] = { 1, 2, 1, 0 };

  if (state->var4)
  {
    // [PERF] Missing `static` causes a copy operation here
    const byte LOOK_AROUND_ANIMS[2][32] = {
      { 1, 1, 1, 3, 3, 1, 6, 6, 7, 8, 7, 6, 6, 6, 7, 8,
      7, 6, 6, 6, 1, 1, 3, 3, 3, 1, 1, 1, 6, 6, 1, 1 },

      { 1, 1, 6, 6, 7, 8, 7, 6, 6, 1, 1, 3, 3, 1, 6, 6,
      1, 1, 1, 3, 4, 5, 4, 3, 3, 3, 4, 5, 4, 3, 1, 1 }
    };

    state->frame = LOOK_AROUND_ANIMS[state->var5][state->var4 - 1];

    if (gfxCurrentDisplayPage)
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
        ApplyWorldCollision(handle, MD_RIGHT);
      }
      else
      {
        state->x--;
        ApplyWorldCollision(handle, MD_LEFT);
      }
    }

doMovementOrWait:
    if (!state->var2)
    {
      state->frame = HIDE_HEAD_ANIM[state->var3];
      state->var3++;

      if (
        (RandomNumber() & 33) &&
        state->var3 == 2 &&
        !ApplyWorldCollision(handle, MD_DOWN))
      {
        state->var4 = 1;
        state->var5 = (word)RandomNumber() % 2;
      }
      else if (state->var3 == 4)
      {
        state->var2++;

        if (state->x > plPosX)
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

        if (ApplyWorldCollision(handle, MD_UP))
        {
          state->var2 = 5;
        }

        if (state->var2 < 3)
        {
          state->y--;

          if (ApplyWorldCollision(handle, MD_UP))
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

        PlaySoundIfOnScreen(handle, SND_DUKE_JUMPING);

        goto doMovementOrWait;
      }
    }
  }
}


void pascal Act_RocketTurret(word handle)
{
  ActorState* state = gmActorStates + handle;

  if (!state->var1)
  {
    if (state->x - 3 > plPosX)
    {
      state->frame = 0;
    }
    else if (state->x + 3 < plPosX)
    {
      state->frame = 2;
    }
    else if (state->y > plPosY)
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
        SpawnActor(ACT_ENEMY_ROCKET_LEFT, state->x - 2, state->y - 1);
        break;

      case 1:
        SpawnActor(ACT_ENEMY_ROCKET_UP, state->x + 1, state->y - 2);
        break;

      case 2:
        SpawnActor(ACT_ENEMY_ROCKET_RIGHT, state->x + 2, state->y - 1);
        break;
    }

    state->var1 = 0;
  }
}


void pascal Act_EnemyRocket(word handle)
{
  ActorState* state = gmActorStates + handle;

  if (!IsActorOnScreen(handle))
  {
    state->deleted = true;
    return;
  }

  state->var1++;

  if (state->var1 == 1)
  {
    PlaySound(SND_FLAMETHROWER_SHOT);
  }

  if (state->id == ACT_ENEMY_ROCKET_LEFT)
  {
    state->x--;

    if (state->var1 > 4)
    {
      state->x--;
    }

    DrawActor(
      ACT_ENEMY_ROCKET_LEFT,
      gfxCurrentDisplayPage + 1,
      state->x,
      state->y,
      DS_NORMAL);

    if (ApplyWorldCollision(handle, MD_LEFT))
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
      ACT_ENEMY_ROCKET_RIGHT,
      gfxCurrentDisplayPage + 1,
      state->x,
      state->y,
      DS_NORMAL);

    if (ApplyWorldCollision(handle, MD_RIGHT))
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
      ACT_ENEMY_ROCKET_UP,
      gfxCurrentDisplayPage + 1,
      state->x,
      state->y,
      DS_NORMAL);

    if (ApplyWorldCollision(handle, MD_UP))
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
      ACT_ENEMY_ROCKET_2_UP,
      gfxCurrentDisplayPage + 1,
      state->x,
      state->y,
      DS_NORMAL);

    if (ApplyWorldCollision(handle, MD_UP))
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
      ACT_ENEMY_ROCKET_2_DOWN,
      gfxCurrentDisplayPage + 1,
      state->x,
      state->y,
      DS_NORMAL);

    // [BUG] Should be MD_DOWN
    if (ApplyWorldCollision(handle, MD_UP))
    {
      state->deleted = true;
    }
  }

  if (state->deleted)
  {
    SpawnEffect(ACT_EXPLOSION_FX_1, state->x, state->y, EM_NONE, 0);
  }

  if (!IsActorOnScreen(handle))
  {
    state->deleted = true;
  }
}


void pascal Act_WatchBotContainerCarrier(word handle)
{
  ActorState* state = gmActorStates + handle;

  if (!state->var1)
  {
    state->frame = 0;

    if (!PlayerInRange(handle, 5))
    {
      if (state->x > plPosX)
      {
        state->x--;

        if (ApplyWorldCollision(handle, MD_LEFT))
        {
          state->var1 = 1;
        }
      }
      else if (state->x + 3 < plPosX)
      {
        state->x++;

        if (ApplyWorldCollision(handle, MD_RIGHT))
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

      SpawnActor(ACT_WATCHBOT_CONTAINER, state->x, state->y - 2);
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
        ACT_FLAME_FX, ACT_WATCHBOT_CONTAINER_CARRIER, state->x, state->y);
    }
  }

  if (!state->var2)
  {
    DrawActor(ACT_WATCHBOT_CONTAINER, 0, state->x, state->y - 2, DS_NORMAL);
  }
}


void pascal Act_WatchBotContainer(word handle)
{
  ActorState* state = gmActorStates + handle;

  UPDATE_ANIMATION_LOOP(state, 1, 5);

  if (state->var1 < 10 && !CheckWorldCollision(
    MD_UP, ACT_WATCHBOT_CONTAINER, 0, state->x, state->y - 1))
  {
    state->y--;
  }

  state->var1++;

  if (state->var1 == 25)
  {
    state->drawStyle = DS_WHITEFLASH;
    state->deleted = true;

    SpawnEffect(
      ACT_WATCHBOT_CONTAINER_DEBRIS_1, state->x, state->y, EM_FLY_LEFT, 0);
    SpawnEffect(
      ACT_WATCHBOT_CONTAINER_DEBRIS_2, state->x, state->y, EM_FLY_RIGHT, 0);
    PlaySound(SND_ATTACH_CLIMBABLE);

    SpawnActor(ACT_WATCHBOT, state->x + 1, state->y + 3);
  }
  else
  {
    DrawActor(ACT_WATCHBOT_CONTAINER, 0, state->x, state->y, state->drawStyle);
  }
}


void pascal Act_BomberPlane(word handle)
{
  ActorState* state = gmActorStates + handle;

  if (state->var1 == 0) // Fly towards player
  {
    state->x--;
    if (
      ApplyWorldCollision(handle, MD_LEFT) ||
      (state->x <= plPosX && state->x + 6 >= plPosX))
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
      SpawnActor(ACT_MINI_NUKE, state->x + 2, state->y + 1);
    }

    if (state->var1 == 30)
    {
      // Fly away
      state->y--;
      state->x -= 2;

      if (!IsActorOnScreen(handle))
      {
        state->deleted = true;
        return;
      }
    }
  }

  // Draw bomb if not dropped yet
  if (state->var1 < 10)
  {
    DrawActor(ACT_MINI_NUKE, 0, state->x + 2, state->y, DS_NORMAL);
  }

  // Draw exhaust flame
  DrawActor(
    ACT_BOMBER_PLANE,
    gfxCurrentDisplayPage + 1,
    state->x,
    state->y,
    DS_NORMAL);
}


void pascal Act_MiniNuke(word handle)
{
  register ActorState* state = gmActorStates + handle;

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

    PlaySound(SND_BIG_EXPLOSION);
    SpawnEffect(ACT_NUCLEAR_EXPLOSION, state->x, state->y, EM_NONE, 0);
    FLASH_SCREEN(SFC_WHITE);

    if (state->id != ACT_MINI_NUKE_SMALL)
    {
      register int i;

      for (i = 4; i < 20; i += 4)
      {
        SpawnEffect(
          ACT_NUCLEAR_EXPLOSION,
          state->x - i,
          state->y + 2,
          EM_NONE,
          i >> 1); // i / 2
        SpawnEffect(
          ACT_NUCLEAR_EXPLOSION,
          state->x + i,
          state->y + 2,
          EM_NONE,
          i >> 1); // i / 2
      }
    }
  }
}


void pascal Act_SpikeBall(word handle)
{
  ActorState* state = gmActorStates + handle;

  if (state->var1 == 2)
  {
    state->x++;

    if (ApplyWorldCollision(handle, MD_RIGHT))
    {
      state->var1 = 1;
    }
  }
  else if (state->var1 == 1)
  {
    state->x--;

    if (ApplyWorldCollision(handle, MD_LEFT))
    {
      state->var1 = 2;
    }
  }

  if (state->var2 < 5)
  {
    state->y--;

    if (ApplyWorldCollision(handle, MD_UP))
    {
      state->var2 = 5;
      PlaySoundIfOnScreen(handle, SND_DUKE_JUMPING);
    }

    if (state->var2 < 2)
    {
      state->y--;

      if (ApplyWorldCollision(handle, MD_UP))
      {
        state->var2 = 5;
        PlaySoundIfOnScreen(handle, SND_DUKE_JUMPING);
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
    PlaySoundIfOnScreen(handle, SND_DUKE_JUMPING);
  }
}


void pascal Act_Reactor(word handle)
{
  ActorState* state = gmActorStates + handle;

  UPDATE_ANIMATION_LOOP(state, 0, 3);
}


void pascal Act_SlimeContainer(word handle)
{
  ActorState* state = gmActorStates + handle;

  // Draw roof
  DrawActor(ACT_SLIME_CONTAINER, 8, state->x, state->y, DS_NORMAL);

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
        SpawnActor(ACT_SLIME_BLOB, state->x + 2, state->y);
      }
    }
    else // still intact
    {
      // Draw bottom part
      DrawActor(ACT_SLIME_CONTAINER, 2, state->x, state->y, DS_NORMAL);

      // Animate slime blob moving around inside
      state->frame = RandomNumber() & 1; // % 2
    }
  }
}


void pascal Act_SlimeBlob(word handle)
{
  ActorState* state = gmActorStates + handle;

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
          MD_DOWN, state->id, state->frame, state->x, state->y + 3))
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
          MD_UP, state->id, state->frame, state->x, state->y))
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
        if (state->x == plPosX)
        {
          state->var1 = 100;
          return;
        }

        state->var2 = !state->var2;
        state->frame = state->x > plPosX ? state->var2 + 7 : state->var2 + 9;

        if (state->var2 % 2)
        {
          if (state->x > plPosX)
          {
            state->x--;

            if (
              CheckWorldCollision(
                MD_LEFT, state->id, state->frame, state->x, state->y) ||
              !CheckWorldCollision(
                MD_UP, state->id, state->frame, state->x - 4, state->y - 1))
            {
              state->var1 = 100;
            }
          }
          else
          {
            state->x++;

            if (
              CheckWorldCollision(
                MD_RIGHT, state->id, state->frame, state->x, state->y) ||
              !CheckWorldCollision(
                MD_UP, state->id, state->frame, state->x + 4, state->y - 1))
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

      if (!((word)RandomNumber() % 32))
      {
        // Start flying up
        state->id = ACT_SLIME_BLOB_2;
        state->frame = 0;
        state->var2 = 0;
      }
      else
      {
        state->frame = state->var2 + (RandomNumber() & 3);

        if (state->var1 == 10)
        {
          if (state->x > plPosX)
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

      state->frame = state->var2 + state->var3 % 2 + 3;

      if (
        (state->x > plPosX && state->var2) ||
        (state->x < plPosX && !state->var2))
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

            if (ApplyWorldCollision(handle, MD_RIGHT))
            {
              state->var1 = 0;
            }
          }
          else
          {
            state->x--;

            if (ApplyWorldCollision(handle, MD_LEFT))
            {
              state->var1 = 0;
            }
          }
        }
      }
    }
  }
}


void pascal Act_Snake(word handle)
{
  ActorState* state = gmActorStates + handle;

  if (state->id == gmPlayerEatingActor && plState == PS_DYING)
  {
    // [PERF] Missing `static` causes a copy operation here
    int DEBRIS_SPEC[] = { 3,
       0,  0, EM_NONE, 0,
      -1, -2, EM_NONE, 2,
       1, -3, EM_NONE, 4
    };

    SpawnDestructionEffects(handle, DEBRIS_SPEC, ACT_EXPLOSION_FX_1);
    state->deleted = true;
    gmPlayerEatingActor = 0;
    return;
  }

  if (state->var2)
  {
    gmPlayerEatingActor = state->id;
    plState = PS_GETTING_EATEN;

    if (state->var3 == 2)
    {
      plAnimationFrame = 0xFF;
    }

    if (!state->var4)
    {
      state->var3++;

      if (state->var3 == 7)
      {
        plPosX += 2;
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
      DamagePlayer();

      if (inputFire && plState != PS_DYING)
      {
        state->health = 1;
        HandleActorShotCollision(state->health, handle);
        plState = PS_NORMAL;
        return;
      }
    }

    state->frame = state->var1 + state->var3 + gfxCurrentDisplayPage;

    if (!state->var4) { return; }
  }

  state->var5++;

  if (state->var1 == 9) // facing right
  {
    if (!state->var2)
    {
      if (!gfxCurrentDisplayPage)
      {
        state->x++;
        state->frame = state->var1 + (state->x & 1);
      }
    }
    else
    {
      if (gfxCurrentDisplayPage)
      {
        plPosX++;
        state->x++;
      }
    }

    if (ApplyWorldCollision(handle, MD_RIGHT))
    {
      state->var1 = 0;
      state->x += 2;
    }
  }
  else // facing left
  {
    if (!state->var2)
    {
      if (!gfxCurrentDisplayPage)
      {
        state->x--;
        state->frame = state->x & 1;
      }
    }
    else
    {
      if (gfxCurrentDisplayPage)
      {
        plPosX--;
        state->x--;
      }
    }

    if (ApplyWorldCollision(handle, MD_LEFT))
    {
      state->var1 = 9;
      state->x -= 2;
    }
  }
}


void pascal Act_SecurityCamera(word handle)
{
  register ActorState* state = gmActorStates + handle;
  word savedY;

  if (plCloakTimeLeft)
  {
    state->frame = 0;
    return;
  }

  if (state->id == ACT_CAMERA_ON_CEILING)
  {
    savedY = state->y;
    state->y++;
  }

  if (state->x + 1 < plPosX)
  {
    if (state->y - 1 > plPosY)
    {
      state->var1 = 3;
    }
    else if (state->y < plPosY)
    {
      state->var1 = 1;
    }
    else
    {
      state->var1 = 2;
    }
  }
  else if (state->x > plPosX)
  {
    if (state->y - 1 > plPosY)
    {
      state->var1 = 5;
    }
    else if (state->y < plPosY)
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
    if (state->y >= plPosY)
    {
      state->var1 = 4;
    }
    else
    {
      state->var1 = 0;
    }
  }

  state->frame = state->var1;

  if (state->id == ACT_CAMERA_ON_CEILING)
  {
    state->y = savedY;
  }
}


void pascal Act_CeilingSucker(word handle)
{
  ActorState* state = gmActorStates + handle;

  // [PERF] Missing `static` causes a copy operation here
  const byte GRAB_ANIM_SEQ[] = { 0, 0, 1, 2, 3, 4, 5, 4, 3, 2, 1, 0 };

  // [PERF] Missing `static` causes a copy operation here
  const byte EAT_PLAYER_ANIM_SEQ[] = {
    0, 0, 0, 0, 0, 0, 10, 9, 8, 7, 6, 0, 6, 0, 6, 0, 6, 0, 6, 0, 6, 7, 8, 9,
    10, 5, 4, 3, 2, 1, 0
  };

  if (
    !state->var1 &&
    plPosX + 4 >= state->x &&
    state->x + 4 >= plPosX)
  {
    state->var1 = 1;
  }

  if (state->var1 < 100 && state->var1)
  {
    if (!state->var2)
    {
      state->frame = GRAB_ANIM_SEQ[state->var1];
    }
    else
    {
      state->frame = EAT_PLAYER_ANIM_SEQ[state->var1];
    }

    state->var1++;

    if (state->var2 && state->var1 == 25)
    {
      plState = PS_NORMAL;
      plAnimationFrame = 0;
      plPosX = state->x;
      DamagePlayer();
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


void pascal Act_PlayerShip(word handle)
{
  ActorState* state = gmActorStates + handle;

  // Update the cooldown timer - the ship can't be picked up again right after
  // exiting it. This is to prevent the player from immediately picking it up
  // right after jumping out of it, which would make it impossible to actually
  // exit the ship. See UpdateActorPlayerCollision()
  if (state->var1 != 0)
  {
    state->var1--;
  }
}


void pascal Act_BrokenMissile(word handle)
{
  static const byte ANIM_SEQ[] = { 1, 2, 3, 2, 3, 4, 3 };

  register ActorState* state = gmActorStates + handle;
  register int i;

  if (!state->var1) { return; } // Hasn't been shot yet

  if (state->var2 >= 12) { return; }

  // Fall over animation
  if (state->var2 < 7)
  {
    if (state->var1 == 1)
    {
      state->frame = ANIM_SEQ[state->var2];
    }
    else
    {
      state->frame = ANIM_SEQ[state->var2] + 4;
    }

    if (ANIM_SEQ[state->var2] == 3)
    {
      PlaySound(SND_ATTACH_CLIMBABLE);
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
      ACT_NUCLEAR_EXPLOSION,
      state->x - (state->var2 == 1 ? 4 : 0),
      state->y,
      EM_NONE,
      0);

    for (i = 0; i < 4; i++)
    {
      SpawnEffect(
        ACT_MISSILE_DEBRIS,
        state->x + (i << 1), // * 2
        state->y,
        i & 1 ? EM_FLY_UPPER_LEFT : EM_FLY_UPPER_RIGHT,
        i);
    }
  }
}


void pascal Act_EyeBallThrower(word handle)
{
  static const byte RISE_UP_ANIM[] = { 0, 0, 0, 0, 0, 0, 1, 2, 3, 4 };

  ActorState* state = gmActorStates + handle;

  if (state->var1 == 0) // Turn towards player
  {
    if (state->x > plPosX)
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
    state->frame = RISE_UP_ANIM[state->var1];
    state->var1++;
  }
  else if (state->var1 == 11) // Walk, decide to attack
  {
    // [PERF] Missing `static` causes a copy operation here
    const byte ANIM_SEQ[] = { 5, 6 };

    state->var3++;

    // Do we want to attack?
    if ((
      (state->id == ACT_EYEBALL_THROWER_L && state->x > plPosX) ||
      (state->id == ACT_EYEBALL_THROWER_R && state->x < plPosX))
      &&
      PlayerInRange(handle, 14) && !PlayerInRange(handle, 9))
    {
      // Start attacking
      state->var1 = 12;
      state->var2 = 0;
    }

    if (state->var1 != 12 && state->var3 % 4 == 0)
    {
      // Animte walking
      state->frame = ANIM_SEQ[state->var2 = !state->var2];

      // Move
      if (state->id == ACT_EYEBALL_THROWER_L)
      {
        state->x--;

        if (ApplyWorldCollision(handle, MD_LEFT))
        {
          // Start reorienting
          state->var1 = 0;
          state->frame = 1;
        }
      }

      if (state->id == ACT_EYEBALL_THROWER_R)
      {
        state->x++;

        if (ApplyWorldCollision(handle, MD_RIGHT))
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
    const byte ANIM_SEQ[] = { 7, 7, 8, 8, 9, 9 };

    state->frame = ANIM_SEQ[state->var2++];

    // Throw eyeball
    if (state->var2 == 4)
    {
      if (state->id == ACT_EYEBALL_THROWER_L)
      {
        SpawnEffect(
          ACT_EYEBALL_PROJECTILE,
          state->x,
          state->y - 6,
          EM_FLY_UPPER_LEFT,
          0);
      }
      else
      {
        SpawnEffect(
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


word FindActorDesc(
  word startIndex, word neededId, word neededX, word neededY, word handle)
{
  register word i;
  register ActorState* state = gmActorStates + handle;
  word x;
  word y;

  for (i = startIndex; i < levelActorListSize * 2; i += 6)
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


void pascal Act_MovingMapPartTrigger(word handle)
{
  ActorState* state = gmActorStates + handle;
  word descIndex;
  word right;
  MovingMapPartState* mapPart = gmMovingMapParts + gmNumMovingMapParts;

  state->drawStyle = DS_INVISIBLE;

  if (gmRequestUnlockNextDoor)
  {
    state->y += 5;

    if (!IsActorOnScreen(handle))
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
    if (gmRequestUnlockNextDoor)
    {
      gmRequestUnlockNextDoor = false;
    }
    else
    {
      return;
    }
  }

  if (
    (state->var2 == 3 || state->var2 == 5) &&
    (mapHasEarthquake == false ||
    gmEarthquakeCountdown >= gmEarthquakeThreshold ||
    gmEarthquakeCountdown == 0))
  {
    return;
  }

  if (state->var1)
  {
    state->var1--;

    if (state->var1 == 0)
    {
      PlaySound(SND_FALLING_ROCK);
    }

    return;
  }

  descIndex = FindActorDesc(0, state->id, state->x, state->y, handle);
  mapPart->left = state->x;
  mapPart->top = state->y;

  descIndex = FindActorDesc(
    descIndex, ACT_META_DYNGEO_MARKER_1, 0x8000, state->y, handle);
  right = mapPart->right = READ_LVL_ACTOR_DESC_X(descIndex);

  descIndex = FindActorDesc(
    descIndex, ACT_META_DYNGEO_MARKER_2, right, 0x8000, handle);
  mapPart->bottom = READ_LVL_ACTOR_DESC_Y(descIndex);

  if (state->var2)
  {
    mapPart->type = state->var2 + 98;
  }

  if (!state->deleted)
  {
    gmNumMovingMapParts++;
    state->deleted = true;
  }
}


void pascal Act_HoverBotGenerator(word handle)
{
  ActorState* state = gmActorStates + handle;

  // Spawn up to 30 robots
  if (state->var1 < 30)
  {
    state->var2++;

    if (state->var2 == 36)
    {
      SpawnActor(ACT_HOVERBOT, state->x + 1, state->y);

      state->var1++;
      state->var2 = 0;
    }
  }

  // Animate
  UPDATE_ANIMATION_LOOP(state, 0, 3);

  // Draw the lower part
  DrawActor(ACT_HOVERBOT_GENERATOR, 4, state->x, state->y, DS_NORMAL);
}


void pascal Act_MessengerDrone(word handle)
{
  register ActorState* state = gmActorStates + handle;
  register byte* screenData;

  // Orient towards player on first update
  if (!state->var1)
  {
    if (state->x < plPosX)
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
        ACT_MESSENGER_DRONE_FLAME_R,
        gfxCurrentDisplayPage,
        state->x,
        state->y,
        DS_NORMAL);
    }

    DrawActor(
      ACT_MESSENGER_DRONE_ENGINE_R,
      0,
      state->x,
      state->y,
      DS_NORMAL);
  }

  if (state->var1 == 2) // facing right
  {
    // Move while not showing message
    if (state->var2 == 0)
    {
      state->x += 2;

      DrawActor(
        ACT_MESSENGER_DRONE_FLAME_L,
        gfxCurrentDisplayPage,
        state->x,
        state->y,
        DS_NORMAL);
    }

    DrawActor(
      ACT_MESSENGER_DRONE_ENGINE_L,
      0,
      state->x,
      state->y,
      DS_NORMAL);
  }

  DrawActor(
    ACT_MESSENGER_DRONE_ENGINE_DOWN,
    0,
    state->x,
    state->y,
    DS_NORMAL);
  DrawActor(
    ACT_MESSENGER_DRONE_BODY,
    0,
    state->x,
    state->y,
    DS_NORMAL);

  // Start showing the message when close enough to the player
  if (!state->var2 && !state->var3 && PlayerInRange(handle, 6))
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
      { 0, 0, 10, 1, 10, 2, 10, 3, 14, 0, 10, 1, 10, 2, 10, 3, 14, 0xFF },

      // "Bring back the brain! ... Please stand by"
      { 0, 0, 8, 1, 8, 2, 8, 3, 14, 4, 1, 5, 1, 6, 1, 7, 1, 4, 1, 5, 1, 6, 1, 7,
        1, 4, 1, 5, 1, 6, 1, 7, 1, 4, 1, 5, 1, 6, 1, 7, 1, 8, 4, 9, 1, 8, 4, 9,
        1, 0xFF },

      // "Live from Rigel it's Saturday night!"
      { 0, 0, 4, 1, 4, 2, 3, 3, 6, 4, 3, 5, 5, 6, 15, 0xFF },

      // "Die!"
      { 0, 0, 1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 15, 0xFF },

      // "You cannot escape us! You will get your brain sucked!"
      { 0, 0, 8, 1, 8, 2, 8, 3, 8, 4, 8, 5, 8, 6, 8, 0xFF }
    };

    DrawActor(
      ACT_MESSENGER_DRONE_FLAME_DOWN,
      gfxCurrentDisplayPage,
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

      DrawActor(state->var4, *screenData, state->x, state->y, DS_NORMAL);
    }
  }

  // Delete ourselves once off screen if we're done showing the message
  if (state->var3 && !IsActorOnScreen(handle))
  {
    state->deleted = true;
  }

  // A drawStyle of DS_INVISIBLE means that this actor is excluded from
  // collision detection against player shots. To make the actor shootable
  // despite that, we need to manually invoke the collision check here.
  HandleActorShotCollision(TestShotCollision(handle), handle);
  state->drawStyle = DS_INVISIBLE;
}


void pascal Act_SlimePipe(word handle)
{
  ActorState* state = gmActorStates + handle;

  state->drawStyle = DS_IN_FRONT;

  state->var1++;
  if (state->var1 % 25 == 0)
  {
    SpawnActor(ACT_SLIME_DROP, state->x + 1, state->y + 1);
    PlaySound(SND_WATER_DROP);
  }

  state->frame = !state->frame;
}


void pascal Act_SlimeDrop(word handle)
{
  ActorState* state = gmActorStates + handle;

  if (!state->gravityState) // landed on ground?
  {
    state->frame++;

    if (state->frame == 2)
    {
      state->deleted = true;
    }
  }
}


void pascal Act_ForceField(word handle)
{
  register ActorState* state = gmActorStates + handle;
  register word drawStyle;

  if (
    !IsSpriteOnScreen(ACT_FORCE_FIELD, 1, state->x, state->y) &&
    !IsSpriteOnScreen(ACT_FORCE_FIELD, 0, state->x, state->y))
  {
    return;
  }

  // Draw emitter on top
  DrawActor(ACT_FORCE_FIELD, 1, state->x, state->y, DS_NORMAL);

  if (state->var2) { return; } // If turned off, we're done here

  // Handle unlocking
  if (gmRequestUnlockNextForceField)
  {
    gmRequestUnlockNextForceField = false;
    state->var2 = true;
    return;
  }

  // Handle player collision
  if (AreSpritesTouching(
    ACT_FORCE_FIELD, 2, state->x, state->y,
    plActorId, plAnimationFrame, plPosX, plPosY))
  {
    // Insta-kill player
    plHealth = 1;
    plMercyFramesLeft = 0;
    plCloakTimeLeft = 0;
    DamagePlayer();

    // [BUG] The cloak doesn't reappear if the player dies while cloaked
    // and then respawns at a checkpoint, potentially making the level
    // unwinnable.  This should use the same cloak respawning code here as
    // in Act_PlayerSprite().
  }

  //
  // Animate and draw the force field itself
  //
  state->var1++;

  if (RandomNumber() & 32)
  {
    drawStyle = DS_WHITEFLASH;
    PlaySound(SND_FORCE_FIELD_FIZZLE);
  }
  else
  {
    drawStyle = DS_NORMAL;
  }

  DrawActor(
    ACT_FORCE_FIELD, state->var1 % 3 + 2, state->x, state->y, drawStyle);
}


void pascal Act_KeyCardSlot(word handle)
{
  ActorState* state = gmActorStates + handle;

  if (state->var1 == 0)
  {
    state->frame = 0;
  }
  else
  {
    UPDATE_ANIMATION_LOOP(state, 0, 3);
  }
}


void pascal Act_KeyHole(word handle)
{
  ActorState* state = gmActorStates + handle;

  // [PERF] Missing `static` causes a copy operation here
  const byte KEY_HOLE_ANIMATION[] = { 0, 1, 2, 3, 4, 3, 2, 1 };

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
word FindFullySolidTileIndex(void)
{
  word i;

  for (i = 0; i < 1000; i++)
  {
    if (gfxTilesetAttributes[i] == 0x0F)
    {
      return i * 8;
    }
  }

  return 0;
}


void pascal Act_SlidingDoorVertical(word handle)
{
  register ActorState* state = gmActorStates + handle;
  register word i;

  state->drawStyle = DS_INVISIBLE;

  if (!state->scoreGiven)
  {
    state->tileBuffer = MM_PushChunk(9 * sizeof(word), CT_TEMPORARY);

    for (i = 0; i < 8; i++)
    {
      state->tileBuffer[i] = Map_GetTile(state->x, state->y - i + 1);
    }

    state->scoreGiven = FindFullySolidTileIndex();

    for (i = 1; i < 8; i++)
    {
      Map_SetTile(state->scoreGiven, state->x, state->y - i + 1);
    }
  }

  if (
    PlayerInRange(handle, 7) &&
    state->y >= plPosY &&
    state->y - 7 < plPosY)
  {
    if (state->health == 1)
    {
      PlaySound(SND_SLIDING_DOOR);
    }

    for (i = state->health; i < 9; i++)
    {
      if (i)
      {
        DrawActor(
          ACT_SLIDING_DOOR_VERTICAL,
          i - state->health,
          state->x,
          state->y - i,
          DS_NORMAL);
      }

      if (state->health == 1 && i < 8)
      {
        Map_SetTile(state->tileBuffer[i], state->x, state->y - i + 1);
      }
    }

    if (state->health < 7)
    {
      state->health++;
    }
    else
    {
      Map_SetTile(state->scoreGiven, state->x, state->y - 7);
    }
  }
  else // player not in range
  {
    if (state->health == 7)
    {
      PlaySound(SND_SLIDING_DOOR);
    }

    for (i = state->health; i < 9; i++)
    {
      if (i)
      {
        DrawActor(
          ACT_SLIDING_DOOR_VERTICAL,
          i - state->health,
          state->x,
          state->y - i,
          DS_NORMAL);
      }

      if (state->health == 1 && i)
      {
        Map_SetTile(state->scoreGiven, state->x, state->y - i + 1);
      }
    }

    if (state->health != 0)
    {
      state->health--;
    }
  }
}


void pascal Act_SlidingDoorHorizontal(word handle)
{
  register ActorState* state = gmActorStates + handle;
  register int i;

  if (!state->scoreGiven)
  {
    state->tileBuffer = MM_PushChunk(6 * sizeof(word), CT_TEMPORARY);

    for (i = 0; i < 5; i++)
    {
      state->tileBuffer[i] = Map_GetTile(state->x + i, state->y);
    }

    state->scoreGiven = FindFullySolidTileIndex();

    for (i = 0; i < 5; i++)
    {
      Map_SetTile(state->scoreGiven, state->x + i, state->y);
    }
  }

  if (
    state->x - 2 <= plPosX && state->x + 6 > plPosX &&
    state->y - 3 < plPosY && state->y + 7 > plPosY)
  {
    if (!state->frame)
    {
      for (i = 0; i < 5; i++)
      {
        Map_SetTile(state->tileBuffer[i], state->x + i, state->y);
      }

      PlaySound(SND_SLIDING_DOOR);
    }

    if (state->frame < 2)
    {
      state->frame++;
    }

    if (state->frame == 2)
    {
      Map_SetTile(state->scoreGiven, state->x, state->y);
      Map_SetTile(state->scoreGiven, state->x + 5, state->y);
    }
  }
  else
  {
    if (state->frame)
    {
      if (state->frame == 2)
      {
        PlaySound(SND_SLIDING_DOOR);
      }

      state->frame--;

      if (!state->frame)
      {
        for (i = 0; i < 5; i++)
        {
          Map_SetTile(state->scoreGiven, state->x + i, state->y);
        }
      }
    }
  }
}


void pascal Act_RespawnBeacon(word handle)
{
  ActorState* state = gmActorStates + handle;

  if (state->var2)
  {
    if (IsActorOnScreen(handle))
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
      gmBeaconPosX = state->x;
      gmBeaconPosY = state->y;
      gmBeaconActivated = true;

      WriteSavedGame('Z');

      ShowInGameMessage("SECTOR SECURE!!!");
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


void pascal Act_Skeleton(word handle)
{
  ActorState* state = gmActorStates + handle;

  if (!state->var5)
  {
    state->var5 = true;

    if (state->x > plPosX)
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

  if (!gfxCurrentDisplayPage)
  {
    if (state->var1 != ORIENTATION_LEFT) // walking right
    {
      state->x++;

      if (ApplyWorldCollision(handle, MD_RIGHT))
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

      if (ApplyWorldCollision(handle, MD_LEFT))
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


void pascal Act_BlowingFan(word handle)
{
  // [PERF] Missing `static` causes a copy operation here
  const byte ANIM_SEQ[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 0, 1, 2, 3,
    0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2,
    0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2
  };

  // [PERF] Missing `static` causes a copy operation here
  const byte THREADS_ANIM_SEQ[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 2, 3, 2, 3,
    2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2,
    3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2
  };

  ActorState* state = gmActorStates + handle;

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
    ACT_BLOWING_FAN_THREADS_ON_TOP,
    THREADS_ANIM_SEQ[state->var2],
    state->x,
    state->y,
    DS_NORMAL);

  // Attach player if in range and fan at speed
  if (
    state->var2 > 24 &&
    plPosY + 25 > state->y &&
    state->y > plPosY &&
    state->x <= plPosX &&
    state->x + 5 > plPosX &&
    plState != PS_DYING)
  {
    plState = PS_BLOWN_BY_FAN;
    gmActiveFanIndex = handle;

    if (
      state->frame == 3 ||
      plPosY + 24 == state->y ||
      plPosY + 25 == state->y)
    {
      PlaySound(SND_SWOOSH);
    }
  }

  // Detach player if out of range, or fan too slow
  if (
    plState == PS_BLOWN_BY_FAN &&
    (state->var2 < 25 ||
     state->x > plPosX ||
     state->x + 5 <= plPosX ||
     state->y > plPosY + 25) &&
    handle == gmActiveFanIndex)
  {
    plState = PS_JUMPING;
    plJumpStep = 5;
    plAnimationFrame = 6;
  }

  // [NOTE] Could be simplified using PlaySoundIfOnScreen()
  if (state->frame == 2 && IsActorOnScreen(handle))
  {
    PlaySound(SND_SWOOSH);
  }
}


void pascal Act_LaserTurret(word handle)
{
  ActorState* state = gmActorStates + handle;

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
      PlaySound(SND_SWOOSH);
    }
  }
  else // not spinning
  {
    if (state->var2 < 7 && state->var2 % 2)
    {
      state->drawStyle = DS_WHITEFLASH;
    }

    if (state->x > plPosX) // player on the right
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
          SpawnActor(ACT_ENEMY_LASER_SHOT_L, state->x - 3, state->y);
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
          SpawnActor(ACT_ENEMY_LASER_SHOT_R, state->x + 2, state->y);
          state->var2 = 40;
        }
      }
    }
  }
}


void pascal Act_EnemyLaserShot(word handle)
{
  register ActorState* state = gmActorStates + handle;

  if (!IsActorOnScreen(handle))
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

    DrawActor(
      muzzleSprite,
      0,
      state->x,
      state->y,
      DS_NORMAL);
    PlaySound(SND_ENEMY_LASER_SHOT);
  }

  if (state->var1 == ACT_ENEMY_LASER_SHOT_R)
  {
    state->x += 2;

    if (CheckWorldCollision(
      MD_RIGHT, ACT_ENEMY_LASER_SHOT_L, 0, state->x, state->y))
    {
      state->deleted = true;
    }
  }
  else
  {
    state->x -= 2;

    if (CheckWorldCollision(
      MD_LEFT, ACT_ENEMY_LASER_SHOT_L, 0, state->x, state->y))
    {
      state->deleted = true;
    }
  }
}


void pascal Act_LevelExitTrigger(word handle)
{
  ActorState* state = gmActorStates + handle;

  state->drawStyle = DS_INVISIBLE;

  if (state->y >= plPosY && state->x >= plPosX && state->x - 2 <= plPosX)
  {
    if (!gmRadarDishesLeft)
    {
      gmGameState = GS_LEVEL_FINISHED;
    }
    else
    {
      ShowTutorial(
        TUT_RADARS_LEFT,
        " WAIT!!!!!!!!      *YOU NEED TO DESTROY ALL THE RADAR*DISHES FIRST BEFORE YOU CAN COMPLETE*THE LEVEL...");
    }
  }
}


void pascal Act_SuperForceField(word handle)
{
  ActorState* state = gmActorStates + handle;

  // Animation when shot or touched by the player (see
  // UpdateActorPlayerCollision() and HandleActorShotCollision())
  if (state->var1)
  {
    state->var1++;

    state->frame = gfxCurrentDisplayPage + 1;

    if (RandomNumber() & 8)
    {
      state->drawStyle = DS_WHITEFLASH;
      PlaySound(SND_FORCE_FIELD_FIZZLE);
    }

    if (state->var1 == 20)
    {
      state->var1 = 0;
      state->frame = 0;
    }
  }

  // Draw the emitter on top
  DrawActor(
    ACT_SUPER_FORCE_FIELD_L, state->var4, state->x, state->y, DS_NORMAL);

  // If not destroyed yet, we're done here. var3 is set in
  // UpdateActorPlayerCollision().
  if (!state->var3) { return; }

  //
  // Destruction animation
  //
  state->var3++;

  if (state->var3 % 2)
  {
    PlaySound(SND_GLASS_BREAKING);
    SpawnParticles(
      state->x + 1, state->y - state->var3 + 15, 0, CLR_LIGHT_BLUE);
    SpawnEffect(
      ACT_SCORE_NUMBER_FX_500,
      state->x,
      state->y - state->var3 + 19,
      EM_SCORE_NUMBER,
      0);
    GiveScore(500);
  }

  if (state->var3 == 11)
  {
    state->deleted = true;

    SpawnEffect(
      ACT_EXPLOSION_FX_2, state->x - 1, state->y + 5, EM_FLY_DOWN, 0);
    SpawnEffect(
      ACT_EXPLOSION_FX_2, state->x - 1, state->y + 5, EM_FLY_UPPER_LEFT, 0);
    SpawnEffect(
      ACT_EXPLOSION_FX_2, state->x - 1, state->y + 5, EM_FLY_UPPER_RIGHT, 0);
    PlaySound(SND_BIG_EXPLOSION);
    ShowInGameMessage("FORCE FIELD DESTROYED... *GOOD WORK...");
  }
}


void pascal Act_IntactMissile(word handle)
{
  register ActorState* state = gmActorStates + handle;
  register int i;

  if (state->var1) // launching/flying?
  {
    DrawActor(
      ACT_MISSILE_EXHAUST_FLAME,
      gfxCurrentDisplayPage,
      state->x,
      state->y,
      DS_NORMAL);

    if (state->var1 == 1)
    {
      SpawnEffect(
        ACT_WHITE_CIRCLE_FLASH_FX, state->x - 2, state->y + 1, EM_NONE, 0);
      SpawnEffect(
        ACT_WHITE_CIRCLE_FLASH_FX, state->x + 1, state->y + 1, EM_NONE, 0);
    }

    if (state->var1 > 5)
    {
      state->y--;

      if (ApplyWorldCollision(handle, MD_UP))
      {
        state->var2 = 1;
      }

      if (state->var1 > 8)
      {
        state->y--;

        if (ApplyWorldCollision(handle, MD_UP))
        {
          state->var2 = 1;
        }
      }

      PlaySound(SND_FLAMETHROWER_SHOT);
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

    Map_DestroySection(state->x, state->y - 14, state->x + 2, state->y - 12);

    for (i = 0; i < 4; i++)
    {
      SpawnEffect(
        ACT_MISSILE_DEBRIS,
        state->x + i * 2,
        state->y - 8,
        (word)i % 2 ? EM_FLY_LEFT : EM_FLY_DOWN,
        i);
      SpawnEffect(
        ACT_MISSILE_DEBRIS,
        state->x + i * 2,
        (state->y - 8) + i * 2,
        (word)i % 2 ? EM_FLY_UP : EM_FLY_RIGHT,
        i);
    }

    state->drawStyle = DS_INVISIBLE;
  }
}


void pascal Act_GrabberClaw(word handle)
{
  register ActorState* state = gmActorStates + handle;
  register word i;
  word savedY;

  state->drawStyle = DS_INVISIBLE;

  // Waiting - neither shootable nor player damaging in this state
  if (state->var2 == 3)
  {
    state->var1++;

    DrawActor(ACT_METAL_GRABBER_CLAW, 0, state->x, state->y, DS_NORMAL);
    DrawActor(ACT_METAL_GRABBER_CLAW, 1, state->x, state->y + 1, DS_NORMAL);

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
      DrawActor(ACT_METAL_GRABBER_CLAW, 0, state->x, state->y + i, DS_NORMAL);
    }

    // Extending
    if (state->var2 == 1)
    {
      DrawActor(ACT_METAL_GRABBER_CLAW, 1, state->x, state->y + i, DS_NORMAL);

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
      DrawActor(ACT_METAL_GRABBER_CLAW, 1, state->x, state->y + i, DS_NORMAL);

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
        0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0 };

      state->frame = 1 + ANIM_SEQ[state->var3];

      DrawActor(
        ACT_METAL_GRABBER_CLAW, state->frame, state->x, state->y + i, DS_NORMAL);

      // Manually test for collision against the player,
      // since we use drawStyle DS_INVISIBLE.
      if (AreSpritesTouching(
        state->id, 2, state->x, state->y + i,
        plActorId, plAnimationFrame, plPosX, plPosY))
      {
        DamagePlayer();
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

    if (TestShotCollision(handle))
    {
      state->deleted = true;

      GiveScore(250);
      SpawnEffect(
        ACT_METAL_GRABBER_CLAW_DEBRIS_1,
        state->x,
        state->y,
        EM_FLY_UPPER_LEFT,
        0);
      SpawnEffect(
        ACT_METAL_GRABBER_CLAW_DEBRIS_2,
        state->x + 2,
        state->y,
        EM_FLY_UPPER_RIGHT,
        0);
      PLAY_EXPLOSION_SOUND();
      SpawnBurnEffect(ACT_FLAME_FX, state->id, state->x, state->y);
    }

    state->y = savedY;
  }
}


void pascal Act_FloatingLaserBot(word handle)
{
  ActorState* state = gmActorStates + handle;
  int xDiff;
  int yDiff;

  if (state->var1 < 10) // Waiting
  {
    state->var1++;

    if (!IsActorOnScreen(handle))
    {
      state->alwaysUpdate = false;
    }
  }
  else if (state->var2 < 40) // Moving towards player
  {
    if (!((word)RandomNumber() % 4))
    {
      xDiff = Sign(plPosX - state->x + 1);
      yDiff = Sign(plPosY - state->y - 2);

      state->x += xDiff;
      state->y += yDiff;

      if (xDiff > 0)
      {
        if (CheckWorldCollision(
          MD_RIGHT, ACT_HOVERING_LASER_TURRET, 0, state->x, state->y))
        {
          state->x--;
        }
      }

      if (xDiff < 0)
      {
        if (CheckWorldCollision(
          MD_LEFT, ACT_HOVERING_LASER_TURRET, 0, state->x, state->y))
        {
          state->x++;
        }
      }

      if (yDiff > 0)
      {
        ApplyWorldCollision(handle, MD_DOWN);
      }

      if (yDiff < 0)
      {
        ApplyWorldCollision(handle, MD_UP);
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
    if (gfxCurrentDisplayPage)
    {
      return;
    }

    switch (state->var2 % 4)
    {
      case 0:
        SpawnActor(ACT_ENEMY_LASER_SHOT_L, state->x - 2, state->y - 1);
        break;

      case 1:
        SpawnActor(ACT_ENEMY_LASER_SHOT_L, state->x - 2, state->y);
        break;

      case 2:
        SpawnActor(ACT_ENEMY_LASER_SHOT_R, state->x + 2, state->y);
        break;

      case 3:
        SpawnActor(ACT_ENEMY_LASER_SHOT_R, state->x + 2, state->y - 1);
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


void pascal Act_Spider(word handle)
{
  ActorState* state = gmActorStates + handle;

  // Delete ourselves if attached to the player and the player gets eaten by
  // a snake or ceiling sucker
  if (plState == PS_GETTING_EATEN)
  {
    if (plAttachedSpider1 == handle)
    {
      plAttachedSpider1 = 0;
      state->deleted = true;
      return;
    }

    if (plAttachedSpider2 == handle)
    {
      plAttachedSpider2 = 0;
      state->deleted = true;
      return;
    }

    if (plAttachedSpider3 == handle)
    {
      plAttachedSpider3 = 0;
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
      state->y = plPosY - 3;
      state->x = plActorId == ACT_DUKE_L ? plPosX + 1 : plPosX;

      state->frame = 8 + ((word)RandomNumber() % 2);
    }
    else
    {
      if (plAttachedSpider2 == handle)
      {
        state->y = plPosY - 1;

        if (plActorId == ACT_DUKE_L)
        {
          state->x = plPosX - 1;
          state->var2 = 12;
        }
        else
        {
          state->x = plPosX + 2;
          state->var2 = 14;
        }
      }
      else if (plAttachedSpider3 == handle)
      {
        state->y = plPosY - 2;

        if (plActorId == ACT_DUKE_R)
        {
          state->x = plPosX - 2;
          state->var2 = 12;
        }
        else
        {
          state->x = plPosX + 3;
          state->var2 = 14;
        }
      }

      state->frame = state->var2 + ((word)RandomNumber() % 2);
    }

    //
    // Fall off if player quickly changes direction
    //
    if (plActorId != state->var4)
    {
      // Whenever the player orientation is different than last frame, increment
      // this counter. When the counter reaches 2, the spider falls off.
      // If the player orientation is unchanged, the counter decays back down
      // to zero, but only every other frame.
      state->var5++;

      state->var4 = plActorId;

      if (state->var5 == 2)
      {
        SpawnEffect(
          ACT_SPIDER_SHAKEN_OFF,
          state->x,
          state->y,
          RandomNumber() & 2 ? EM_FLY_UPPER_LEFT : EM_FLY_UPPER_RIGHT,
          0);

        if (plAttachedSpider2 == handle)
        {
          plAttachedSpider2 = 0;
        }
        else if (plAttachedSpider3 == handle)
        {
          plAttachedSpider3 = 0;
        }
        else
        {
          plAttachedSpider1 = 0;
        }

        state->deleted = true;
      }
    }
    else // player orientation unchanged
    {
      if (gfxCurrentDisplayPage && state->var5)
      {
        state->var5--;
      }
    }

    // Also fall off if player is dying
    if (plState == PS_DYING)
    {
      SpawnEffect(
        ACT_SPIDER_SHAKEN_OFF,
        state->x,
        state->y,
        RandomNumber() & 2 ? EM_FLY_UPPER_RIGHT : EM_FLY_UPPER_LEFT,
        0);

      state->deleted = true;
    }

    return;
  }

  if (state->var3 == 0)
  {
    state->var3 = 1;

    if (CheckWorldCollision(MD_DOWN, ACT_SPIDER, 0, state->x, state->y + 1))
    {
      state->scoreGiven = true;
      state->frame = 9;
    }
  }

  //
  // Movement
  //
  if (state->var1 >= 2 || gfxCurrentDisplayPage == 0)
  {
    if (state->var1 == ORIENTATION_RIGHT)
    {
      state->x++;

      if (state->scoreGiven) // on ground
      {
        if (ApplyWorldCollision(handle, MD_RIGHT))
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
        CheckWorldCollision(MD_RIGHT, ACT_SPIDER, 0, state->x, state->y) ||
        !CheckWorldCollision(MD_UP, ACT_SPIDER, 0, state->x + 2, state->y - 1))
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
        if (ApplyWorldCollision(handle, MD_LEFT))
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
        CheckWorldCollision(MD_LEFT, ACT_SPIDER, 0, state->x, state->y) ||
        !CheckWorldCollision(MD_UP, ACT_SPIDER, 0, state->x - 2, state->y - 1))
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
      state->x == plPosX &&
      state->var1 != 2 &&
      state->frame < 6 &&
      state->y < plPosY - 3)
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


bool BlueGuard_UpdateShooting(word handle)
{
  ActorState* state = gmActorStates + handle;

  if (plCloakTimeLeft)
  {
    return false;
  }

  // Don't attack if facing away from the player. Frames 0..5 are facing right,
  // 6..11 are facing left.
  if (
    (state->frame < 6 && state->x > plPosX) ||
    (state->frame > 5 && state->x < plPosX))
  {
    return false;
  }

  if (
    state->y + 3 > plPosY &&
    state->y - 3 < plPosY &&
    plState == PS_NORMAL)
  {
    if (state->var3) // Stance change cooldown set
    {
      state->var3--;
    }
    else
    {
      if (inputMoveDown || state->y < plPosY)
      {
        // Crouch down
        state->frame = state->var1 ? 11 : 5;

        // Set stance change cooldown
        state->var3 = (word)RandomNumber() % 16;
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


  if (!((word)RandomNumber() % 8))
  {
    switch (state->frame)
    {
      case 10:
        SpawnActor(ACT_ENEMY_LASER_SHOT_L, state->x - 2, state->y - 2);
        state->frame = 15; // Recoil animation
        break;

      case 11:
        SpawnActor(ACT_ENEMY_LASER_SHOT_L, state->x - 2, state->y - 1);
        break;

      case 4:
        SpawnActor(ACT_ENEMY_LASER_SHOT_R, state->x + 3, state->y - 2);
        state->frame = 14; // Recoil animation
        break;

      case 5:
        SpawnActor(ACT_ENEMY_LASER_SHOT_R, state->x + 3, state->y - 1);
        break;

    }
  }

  return true;
}


void pascal Act_BlueGuard(word handle)
{
  ActorState* state = gmActorStates + handle;

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
    if (state->y == plPosY && PlayerInRange(handle, 6))
    {
      // Stop typing
      state->var5 = 1;

      // Face player
      if (state->x > plPosX)
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
      state->frame = 12 + ((word)gfxCurrentDisplayPage >> (RandomNumber() & 4));
      return;
    }
  }

  // Attack if player in sight
  if (BlueGuard_UpdateShooting(handle))
  {
    // Don't walk when attacking
    return;
  }

  state->var3 = 0;

  //
  // Walking
  //
  if (state->var1 == 0 && gfxCurrentDisplayPage)
  {
    // Count how many steps we've walked
    state->var2++;

    state->x++;

    if (ApplyWorldCollision(handle, MD_RIGHT) || state->var2 == 20)
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

  if (state->var1 == 1 && gfxCurrentDisplayPage)
  {
    // Count how many steps we've walked
    state->var2++;

    state->x--;

    if (ApplyWorldCollision(handle, MD_LEFT) || state->var2 == 20)
    {
      // Turn around
      state->var1 = 0;
      state->var2 = 0;

      // [BUG] If the guard is placed in the air, this results in an infinite
      // loop - it keeps alternating between the two direction changes, since
      // each ApplyWorldCollision() call will fail.
      goto begin;
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


void pascal Act_SpikedGreenCreature(word handle)
{
  register ActorState* state = gmActorStates + handle;

  if (state->var1 < 15) // waiting
  {
    state->var1++;

    // Blinking eye
    if (state->var1 == 5)
    {
      SpawnEffect(
        state->id == ACT_GREEN_CREATURE_L
          ? ACT_GREEN_CREATURE_EYE_FX_L : ACT_GREEN_CREATURE_EYE_FX_R,
        state->x,
        state->y,
        EM_NONE,
        0);

      SpawnEffect(
        state->id == ACT_GREEN_CREATURE_L
          ? ACT_GREEN_CREATURE_EYE_FX_L : ACT_GREEN_CREATURE_EYE_FX_R,
        state->x,
        state->y,
        EM_NONE,
        4);
    }

    // Shell burst animation
    if (state->var1 == 15)
    {
      register int i;
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
        SpawnEffect(effectId + i, state->x, state->y, i, 0);
      }

      PlaySound(SND_GLASS_BREAKING);

      // Switch to form without shell
      state->frame++;
    }
  }
  else // awake
  {
    if (state->var1 < 30)
    {
      if (state->x > plPosX)
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
        const int JUMP_SEQUENCE[] = {
          3, 0, 0, 3, 0, 0, 4, 2, -2, 4, 2, -1, 4, 2, 0, 5, 2, 0, 0xFF
        };

        if (state->var1 == 30)
        {
          state->gravityAffected = false;
          state->var2 = 0;
          state->var1 = 31;
        }

        state->frame = JUMP_SEQUENCE[state->var2];

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
        ApplyWorldCollision(handle, MD_LEFT))
      {
        state->id = ACT_GREEN_CREATURE_R;
      }
      else if (
        state->id == ACT_GREEN_CREATURE_R &&
        ApplyWorldCollision(handle, MD_RIGHT))
      {
        state->id = ACT_GREEN_CREATURE_L;
      }
    }
  }
}


void pascal Act_GreenPanther(word handle)
{
  ActorState* state = gmActorStates + handle;

  // [PERF] Missing `static` causes a copy operation here
  const byte ANIM_SEQ[] = { 0, 1, 2, 1 };

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

      if (ApplyWorldCollision(handle, MD_LEFT))
      {
        state->id = ACT_BIG_GREEN_CAT_R;
        state->var1 = 10;
      }

      state->x--;

      if (ApplyWorldCollision(handle, MD_LEFT))
      {
        state->id = ACT_BIG_GREEN_CAT_R;
        state->var1 = 10;
      }
    }
    else
    {
      state->x++;

      if (ApplyWorldCollision(handle, MD_RIGHT))
      {
        state->id = ACT_BIG_GREEN_CAT_L;
        state->var1 = 10;
      }

      state->x++;

      if (ApplyWorldCollision(handle, MD_RIGHT))
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


void pascal Act_Turkey(word handle)
{
  ActorState* state = gmActorStates + handle;

  if (!state->var1)
  {
    state->var1 = true;

    if (state->x < plPosX)
    {
      state->var2 = ORIENTATION_RIGHT;
    }
  }

  if (state->var2 == ORIENTATION_RIGHT)
  {
    state->x++;

    if (ApplyWorldCollision(handle, MD_RIGHT))
    {
      state->var2 = ORIENTATION_LEFT;
    }
    else
    {
      state->frame = gfxCurrentDisplayPage + 2;
    }
  }
  else if (state->var2 == ORIENTATION_LEFT)
  {
    state->x--;

    if (ApplyWorldCollision(handle, MD_LEFT))
    {
      state->var2 = ORIENTATION_RIGHT;
    }
    else
    {
      state->frame = gfxCurrentDisplayPage;
    }
  }
  else // cooked turkey
  {
    state->frame = state->var3++ % 4 + 4;
  }
}


void pascal Act_GreenBird(word handle)
{
  // [PERF] Missing `static` causes a copy operation here
  const word ANIM_SEQ[] = { 0, 1, 2, 1 };

  ActorState* state = gmActorStates + handle;

  // Orient towards player on first update
  if (!state->var3)
  {
    if (state->x > plPosX)
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

    if (ApplyWorldCollision(handle, MD_RIGHT))
    {
      state->var1 = 0;
    }
  }
  else
  {
    state->x--;

    if (ApplyWorldCollision(handle, MD_LEFT))
    {
      state->var1 = 3;
    }
  }

  // Animate
  state->var2++;
  state->frame = state->var1 + ANIM_SEQ[state->var2 % 4];
}


void pascal Act_RedBird(word handle)
{
  // [PERF] Missing `static` causes a copy operation here
  const word FLY_ANIM_SEQ[] = { 0, 1, 2, 1 };

  ActorState* state = gmActorStates + handle;

  // Switch to attacking state when above player
  if (
    state->var1 != 2 &&
    state->y + 2 < plPosY &&
    state->x > plPosX &&
    state->x < plPosX + 2)
  {
    state->var1 = 2;
  }

  if (state->var1 == ORIENTATION_RIGHT) // Fly right
  {
    state->x++;

    if (ApplyWorldCollision(handle, MD_RIGHT))
    {
      state->var1 = ORIENTATION_LEFT;
    }
    else
    {
      state->var2++;

      state->frame = 3 + FLY_ANIM_SEQ[state->var2 % 4];
    }
  }

  if (state->var1 == ORIENTATION_LEFT) // Fly left
  {
    state->x--;

    if (ApplyWorldCollision(handle, MD_LEFT))
    {
      state->var1 = ORIENTATION_RIGHT;
      return;
    }
    else
    {
      state->var2++;

      state->frame = FLY_ANIM_SEQ[state->var2 % 4];
    }
  }

  if (state->var1 != 2) { return; }

  if (state->var3 < 7) // Hover above player
  {
    // Store original height so we can rise back up there after plunging down
    // onto the player
    state->var4 = state->y;

    state->frame = 6 + gfxCurrentDisplayPage;

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
    state->frame = 6 + gfxCurrentDisplayPage;

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
    state->var1 = gfxCurrentDisplayPage;
    state->var3 = 0;
  }
}


void pascal Act_Elevator(word handle)
{
  register ActorState* state = gmActorStates + handle;
  register int i;

  if (
    plState == PS_DYING ||
    plState == PS_AIRLOCK_DEATH_L ||
    plState == PS_AIRLOCK_DEATH_R)
  {
    return;
  }

  if (state->var5)
  {
    state->var5 = false;

    state->tileBuffer = MM_PushChunk(2 * sizeof(word), CT_TEMPORARY);

    state->scoreGiven = FindFullySolidTileIndex();

    for (i = 0; i < 2; i++)
    {
      state->tileBuffer[i] = Map_GetTile(state->x + i + 1, state->y - 2);
    }
  }

  if (state->var4)
  {
    if (!CheckWorldCollision(MD_DOWN, ACT_ELEVATOR, 0, state->x, state->y + 1))
    {
      state->y++;
    }
    else
    {
      state->var4 = 0;
    }

    if (!CheckWorldCollision(MD_DOWN, ACT_ELEVATOR, 0, state->x, state->y + 1))
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
        state->tileBuffer[i] = Map_GetTile(state->x + i + 1, state->y - 2);
      }
    }
  }
  else
  {
    if (
      state->y - 3 == plPosY &&
      ((plActorId == ACT_DUKE_R && state->x <= plPosX && state->x + 2 > plPosX) ||
       (plActorId == ACT_DUKE_L && state->x - 1 <= plPosX && state->x >= plPosX)))
    {
      plOnElevator = true;

      ShowTutorial(TUT_ELEVATOR, "PRESS UP OR DOWN TO USE THE*TURBO LIFT.");

      if (
        inputMoveUp && !CheckWorldCollision(
          MD_UP, ACT_ELEVATOR, 0, state->x, state->y - 6))
      {
        state->var1 = 1;

        if (gfxCurrentDisplayPage)
        {
          PlaySound(SND_FLAMETHROWER_SHOT);
        }

        for (i = 0; i < 2; i++)
        {
          Map_SetTile(state->tileBuffer[i], state->x + i + 1, state->y - 2);
        }

        if (!CheckWorldCollision(
          MD_UP, ACT_ELEVATOR, 0, state->x, state->y - 6))
        {
          state->y--;
          plPosY--;
          plState = PS_RIDING_ELEVATOR;
        }

        if (!CheckWorldCollision(
          MD_UP, ACT_ELEVATOR, 0, state->x, state->y - 6))
        {
          state->y--;
          plPosY--;
          plState = PS_RIDING_ELEVATOR;
        }

        for (i = 0; i < 2; i++)
        {
          state->tileBuffer[i] = Map_GetTile(state->x + i + 1, state->y - 2);
        }

        goto drawFlame;
      }
      else if (inputMoveDown && !CheckWorldCollision(
        MD_DOWN, ACT_ELEVATOR, 0, state->x, state->y + 1))
      {
        state->var1 = 0;

        for (i = 0; i < 2; i++)
        {
          Map_SetTile(state->tileBuffer[i], state->x + i + 1, state->y - 2);
        }

        if (!CheckWorldCollision(
          MD_DOWN, ACT_ELEVATOR, 0, state->x, state->y + 1))
        {
          state->y++;
          plPosY++;
          plState = PS_RIDING_ELEVATOR;
        }

        if (!CheckWorldCollision(
          MD_DOWN, ACT_ELEVATOR, 0, state->x, state->y + 1))
        {
          state->y++;
          plPosY++;
          plState = PS_RIDING_ELEVATOR;
        }

        for (i = 0; i < 2; i++)
        {
          state->tileBuffer[i] = Map_GetTile(state->x + i + 1, state->y - 2);
        }

        goto drawFlame;
      }
      else
      {
        if (CheckWorldCollision(
          MD_DOWN, ACT_ELEVATOR, 0, state->x, state->y + 1))
        {
          plOnElevator = false;
        }

        if (inputJump)
        {
          plOnElevator = false;
          state->var1 = 3;
        }
        else
        {
          plState = PS_NORMAL;
          state->var1 = 3;
        }
      }
    }
    else if (!state->var4)
    {
      plOnElevator = false;

      if (!CheckWorldCollision(
        MD_DOWN, ACT_ELEVATOR, 0, state->x, state->y + 1))
      {
        for (i = 0; i < 2; i++)
        {
          Map_SetTile(state->tileBuffer[i], state->x + i + 1, state->y - 2);
        }

        state->var4 = 1;

        goto drawHandrail;
      }
    }

    for (i = 0; i < 2; i++)
    {
      Map_SetTile(state->scoreGiven, state->x + i + 1, state->y - 2);
    }

drawFlame:
    if (state->var1 && !CheckWorldCollision(
      MD_DOWN, ACT_ELEVATOR, 0, state->x, state->y + 1))
    {
      DrawActor(
        ACT_ELEVATOR,
        state->var1 + gfxCurrentDisplayPage,
        state->x,
        state->y,
        DS_NORMAL);
    }
  }

drawHandrail:
  DrawActor(ACT_ELEVATOR, 5, state->x, state->y, DS_NORMAL);
}


void pascal Act_SmashHammer(word handle)
{
  register ActorState* state = gmActorStates + handle;

  if (state->var1 < 20) // waiting
  {
    if (!state->var1 && !IsActorOnScreen(handle)) { return; }

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
      DrawActor(ACT_SMASH_HAMMER, 1, state->x, state->y - i, DS_NORMAL);
    }

    if (ApplyWorldCollision(handle, MD_DOWN))
    {
      SpawnEffect(ACT_SMOKE_CLOUD_FX, state->x, state->y + 4, EM_NONE, 0);
      PlaySound(SND_HAMMER_SMASH);

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
      DrawActor(ACT_SMASH_HAMMER, 1, state->x, state->y - i + 1, DS_NORMAL);
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


void pascal Act_WaterArea(word handle)
{
  ActorState* state = gmActorStates + handle;

  state->drawStyle = DS_INVISIBLE;

  if (state->var1) // water surface or 4x4 block
  {
    state->var1++;
    if (state->var1 == 5)
    {
      state->var1 = 1;
    }

    DrawWaterArea(state->x, state->y, state->var1);

    if (state->var2) // 4x4 block
    {
      DrawWaterArea(state->x + 2, state->y, state->var1);
      DrawWaterArea(state->x, state->y + 2, 0);
      DrawWaterArea(state->x + 2, state->y + 2, 0);
    }
  }
  else // 1x1 block
  {
    DrawWaterArea(state->x, state->y, 0);
  }
}


void pascal Act_WaterDrop(word handle)
{
  ActorState* state = gmActorStates + handle;

  // Once the water drop reaches solid ground, it deletes itself
  if (!state->gravityState)
  {
    state->deleted = true;
  }
}


void pascal Act_WaterDropSpawner(word handle)
{
  ActorState* state = gmActorStates + handle;

  state->drawStyle = DS_INVISIBLE;

  if (gfxCurrentDisplayPage && RandomNumber() > 220)
  {
    SpawnActor(ACT_WATER_DROP, state->x, state->y);

    // [NOTE] This could be simplified to PlaySoundIfOnScreen().
    if (IsActorOnScreen(handle))
    {
      PlaySound(SND_WATER_DROP);
    }
  }
}


void pascal Act_LavaFountain(word handle)
{
  ActorState* state = gmActorStates + handle;

  // This table is a list of lists of pairs of (animationFrame, yOffset).
  // Each sub-list is terminated by a value of 127.
  // The list as a whole is terminated by -127.
  //
  // [PERF] Missing `static` causes a copy operation here
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

  state->drawStyle = DS_INVISIBLE;

  if (state->var1 < 15)
  {
    state->var1++;
    return;
  }

  while (SPRITE_PLACEMENT_TABLE[state->var2] != 127)
  {
    DrawActor(
      ACT_LAVA_FOUNTAIN,
      SPRITE_PLACEMENT_TABLE[state->var2],
      state->x,
      state->y + SPRITE_PLACEMENT_TABLE[state->var2 + 1],
      DS_NORMAL);

    // Since we use drawStyle DS_INVISIBLE, we have to test for intersection
    // with the player manually.
    if (AreSpritesTouching(
      ACT_LAVA_FOUNTAIN,
      SPRITE_PLACEMENT_TABLE[state->var2],
      state->x,
      state->y + SPRITE_PLACEMENT_TABLE[state->var2 + 1],
      plActorId,
      plAnimationFrame,
      plPosX,
      plPosY))
    {
      DamagePlayer();
    }

    if (state->var2 < 5)
    {
      PlaySound(SND_LAVA_FOUNTAIN);
    }

    state->var2 += 2;
  }

  state->var2++;

  if (SPRITE_PLACEMENT_TABLE[state->var2] == -127)
  {
    state->var2 = 0;
    state->var1 = 0;

    if (!IsActorOnScreen(handle))
    {
      state->alwaysUpdate = false;
      state->remainActive = true;
    }
  }
}


void pascal Act_RadarComputer(word handle)
{
  ActorState* state = gmActorStates + handle;
  byte i;

  // [PERF] Missing `static` causes a copy operation here
  const byte RADARS_PRESENT_ANIM_SEQ[] = {
    4, 4, 4, 0, 4, 4, 4, 0, 4, 4, 4, 0, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5
  };

  // [PERF] Missing `static` causes a copy operation here
  const byte RADARS_DESTROYED_ANIM_SEQ[] = {
    6, 6, 6, 0, 6, 6, 6, 0, 6, 6, 6, 0, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7
  };


  // Draw additional parts (the actor sprite itself is just the screen)
  for (i = 1; i < 4; i++)
  {
    DrawActor(
      ACT_RADAR_COMPUTER_TERMINAL, i, state->x, state->y, DS_NORMAL);
  }

  if (gmRadarDishesLeft)
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
        ACT_RADAR_COMPUTER_TERMINAL,
        5,
        state->x,
        state->y,
        DS_NORMAL);

      DrawActor(
        ACT_RADAR_COMPUTER_TERMINAL,
        7 + gmRadarDishesLeft,
        state->x - 1,
        state->y,
        DS_NORMAL);
    }

    // [NOTE] This is identical to the code in the else branch, so it could be
    // moved to below the if/else to avoid some code duplication.
    if (gfxCurrentDisplayPage)
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

    if (gfxCurrentDisplayPage)
    {
      state->var2++;
    }

    if (state->var2 == 29)
    {
      state->var2 = 0;
    }
  }
}


void pascal Act_HintMachine(word handle)
{
  ActorState* state = gmActorStates + handle;

  // Draw the hint globe attached to the machine if it has been placed
  if (state->var1)
  {
    DrawActor(
      ACT_SPECIAL_HINT_GLOBE_ICON, 0, state->x + 1, state->y - 4, DS_NORMAL);
  }
}


void pascal Act_WindBlownSpiderGenerator(word handle)
{
  ActorState* state = gmActorStates + handle;

  state->drawStyle = DS_INVISIBLE;

  if (state->y > plPosY && ((word)RandomNumber() % 2) && gfxCurrentDisplayPage)
  {
    SpawnEffect(
      ACT_WINDBLOWN_SPIDER_GENERATOR + (int)RandomNumber() % 3,
      gmCameraPosX + (VIEWPORT_WIDTH - 1),
      gmCameraPosY + (word)RandomNumber() % 16,

      // either EM_BLOW_IN_WIND or EM_FLY_LEFT
      EM_BLOW_IN_WIND - ((word)RandomNumber() & 2),
      0);
  }
}


void pascal Act_UniCycleBot(word handle)
{
  ActorState* state = gmActorStates + handle;

  if (state->var1 < 15)
  {
    state->var1++;

    if (state->var1 == 15)
    {
      if (state->x < plPosX)
      {
        state->var2 = 2;
      }
      else
      {
        state->var2 = 1;
      }

      state->var4 = (word)RandomNumber() % 32 + 15;
      state->var3 = 0;
    }

    state->frame = ((word)RandomNumber() % 2) * 5;
  }
  else
  {
    if (state->var4 != 0)
    {
      state->var4--;
    }

    if (state->var2 == 1)
    {
      state->frame = 3 + gfxCurrentDisplayPage;

      if (state->var3 < 10)
      {
        state->var3++;

        if (gfxCurrentDisplayPage)
        {
          SpawnEffect(
            ACT_SMOKE_PUFF_FX, state->x + 1, state->y, EM_FLY_UPPER_RIGHT, 0);
        }
      }
      else
      {
        state->x--;
      }

      if (ApplyWorldCollision(handle, MD_LEFT) || state->var4 == 0)
      {
        state->var1 = 0;
      }
    }

    if (state->var2 == 2)
    {
      state->frame = 1 + gfxCurrentDisplayPage;

      if (state->var3 < 10)
      {
        state->var3++;

        if (gfxCurrentDisplayPage)
        {
          SpawnEffect(
            ACT_SMOKE_PUFF_FX, state->x, state->y, EM_FLY_UPPER_LEFT, 0);
        }
      }
      else
      {
        state->x++;
      }

      if (ApplyWorldCollision(handle, MD_RIGHT) || state->var4 == 0)
      {
        state->var1 = 0;
      }
    }
  }
}


void pascal Act_WallWalker(word handle)
{
  ActorState* state = gmActorStates + handle;

  // [PERF] Missing `static` causes a copy operation here
  const byte MOVEMENT_BY_STATE[] = { MD_UP, MD_DOWN, MD_LEFT, MD_RIGHT };

  state->var5 = !state->var5;

  if (state->var5) { return; }

  state->var4 = !state->var4;

  if (state->var3 != 0)
  {
    state->var3--;
  }

  switch (state->var1)
  {
    case 0:
      state->frame = state->var4 * 2;

      if (state->frame)
      {
        state->y--;
      }
      break;

    case 1:
      state->frame = state->var4 * 2;

      if (!state->frame)
      {
        state->y++;
      }
      break;

    case 2:
      state->frame = state->var4;

      if (!state->frame)
      {
        state->x--;
      }
      break;

    case 3:
      state->frame = state->var4;

      if (state->frame)
      {
        state->x++;
      }
      break;
  }

// TODO: Is there a loop that produces the same ASM?
repeat:
  if (
    ApplyWorldCollision(handle, MOVEMENT_BY_STATE[state->var1]) ||
    state->var3 == 0)
  {
    if (state->var1 < 2)
    {
      state->var1 = (word)RandomNumber() % 2 + 2;
    }
    else
    {
      state->var1 = (word)RandomNumber() % 2;
    }

    state->var3 = (word)RandomNumber() % 32 + 10;

    goto repeat;
  }
}


void pascal Act_AirlockDeathTrigger(word handle)
{
  ActorState* state = gmActorStates + handle;

  state->drawStyle = DS_INVISIBLE;

  if (
    state->id == ACT_AIRLOCK_DEATH_TRIGGER_R &&
    Map_GetTile(state->x + 3, state->y))
  {
    return;
  }

  if (
    state->id == ACT_AIRLOCK_DEATH_TRIGGER_L &&
    Map_GetTile(state->x - 3, state->y))
  {
    return;
  }

  state->deleted = true;

  plAnimationFrame = 8;

  if (state->id == ACT_AIRLOCK_DEATH_TRIGGER_L)
  {
    plState = PS_AIRLOCK_DEATH_L;
  }
  else
  {
    plState = PS_AIRLOCK_DEATH_R;
  }

  plAnimationFrame = 8;
}


void pascal Act_AggressivePrisoner(word handle)
{
  // [PERF] Missing `static` causes a copy operation here
  const byte ANIM_SEQ[] = { 1, 2, 3, 4, 0 };

  ActorState* state = gmActorStates + handle;

  if (state->var1 != 2) // not dying
  {
    DrawActor(ACT_AGGRESSIVE_PRISONER, 0, state->x, state->y, DS_NORMAL);

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
    state->x - 4 < plPosX &&
    state->x + 7 > plPosX &&
    !state->var1 &&
    (RandomNumber() & 0x10) &&
    gfxCurrentDisplayPage)
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
    else if (gfxCurrentDisplayPage)
    {
      state->var2++;
    }
  }
}


void pascal Act_ExplosionTrigger(word handle)
{
  ActorState* state = gmActorStates + handle;

  state->drawStyle = DS_INVISIBLE;

  SpawnEffect(ACT_EXPLOSION_FX_1, state->x, state->y, EM_NONE, 0);
  SpawnEffect(ACT_EXPLOSION_FX_1, state->x - 1, state->y - 2, EM_NONE, 1);
  SpawnEffect(ACT_EXPLOSION_FX_1, state->x + 1, state->y - 3, EM_NONE, 2);

  state->deleted = true;
}


void pascal UpdateBossDeathSequence(word handle)
{
  ActorState* state = gmActorStates + handle;

  if (state->var3 == 2)
  {
    StopMusic();
    GiveScore(50000);
    state->gravityAffected = false;
  }

  if (state->var3 == 50)
  {
    FLASH_SCREEN(SFC_WHITE);
    PlaySound(SND_BIG_EXPLOSION);
  }

  // Rise up
  if (state->var3 > 50 && state->y > 3)
  {
    state->y -= 2;
  }

  if (state->var3 == 60)
  {
    gmGameState = GS_EPISODE_FINISHED;
    return;
  }

  switch (state->var3)
  {
    case 1: case 3: case 7: case 14: case 16: case 21: case 25: case 27:
    case 30: case 32: case 36: case 40: case 43: case 48: case 50:
      PLAY_EXPLOSION_SOUND();

      SpawnParticles(
        state->x + (word)RandomNumber() % 4,
        state->y - (word)RandomNumber() % 8,
        (word)RandomNumber() % 2 - 1,
        (word)RandomNumber() % 16);
      SpawnEffect(
        ACT_EXPLOSION_FX_1,
        state->x + (word)RandomNumber() % 4,
        state->y - (word)RandomNumber() % 8,
        EM_NONE,
        0);
      SpawnEffect(
        ACT_FLAME_FX,
        state->x + (word)RandomNumber() % 4,
        state->y - (word)RandomNumber() % 8,
        EM_FLY_DOWN,
        0);
  }

  if (state->var3 < 50)
  {
    if (gfxCurrentDisplayPage)
    {
      state->drawStyle = DS_INVISIBLE;
    }

    if ((RandomNumber() & 4) && gfxCurrentDisplayPage)
    {
      FLASH_SCREEN(SFC_WHITE);
      PlaySound(SND_BIG_EXPLOSION);
    }
    else
    {
      PLAY_EXPLOSION_SOUND();
    }
  }

  state->var3++;
}


void pascal Act_Boss1(word handle)
{
  ActorState* state = gmActorStates + handle;

  // [PERF] Missing `static` causes a copy operation here
  sbyte Y_MOVEMENT_SEQ[] = { -1, -1, 0, 0, 1, 1, 1, 0, 0, -1 };

  // Animate the ship
  state->frame = gfxCurrentDisplayPage;

  if (state->var3 > 1) // Dying
  {
    UpdateBossDeathSequence(handle);
  }
  else if (state->var1 == 0) // First activation
  {
    state->var1 = 3;
    state->var5 = state->y;
    state->var4 = 0;

    StopPreBossMusic();
    StartMusicPlayback(sndInGameMusicBuffer);

    HUD_DrawBossHealthBar(gmBossHealth);
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
        MD_DOWN, ACT_BOSS_EPISODE_1, 0, state->x, state->y + 1))
      {
        state->gravityAffected = false;

        state->var1 = 2;

        PlaySound(SND_HAMMER_SMASH);
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
      MD_LEFT, ACT_BOSS_EPISODE_1, 0, state->x - 2, state->y))
    {
      state->var1 = 4;
    }
  }
  else if (state->var1 == 4)
  {
    if (gfxCurrentDisplayPage)
    {
      SpawnActor(ACT_MINI_NUKE_SMALL, state->x + 3, state->y + 1);
    }

    state->x += 2;

    if (CheckWorldCollision(
      MD_RIGHT, ACT_BOSS_EPISODE_1, 0, state->x + 2, state->y))
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
      MD_DOWN, ACT_BOSS_EPISODE_1, 0, state->x, state->y + 1))
    {
      state->gravityAffected = false;
    }

    if (!state->gravityAffected)
    {
      state->x -= 2;

      if (CheckWorldCollision(
        MD_LEFT, ACT_BOSS_EPISODE_1, 0, state->x - 2, state->y))
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
        MD_RIGHT, ACT_BOSS_EPISODE_1, 0, state->x + 1, state->y))
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
        MD_LEFT, ACT_BOSS_EPISODE_1, 0, state->x - 1, state->y))
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
      state->var4 > 50 &&
      state->x - 1 <= plPosX &&
      state->x + 9 >= plPosX)
    {
      state->var4 = 0;
      state->var1 = 1;
    }

    // This does nothing, but is required to produce assembly matching the
    // original
    while (false) {}
  }

  if (state->var3 < 3)
  {
    // Normal face
    DrawActor(ACT_BOSS_EPISODE_1, 2, state->x, state->y, DS_NORMAL);
  }
  else if (!gfxCurrentDisplayPage)
  {
    // Scared face
    DrawActor(ACT_BOSS_EPISODE_1, 3, state->x, state->y, DS_NORMAL);
  }
}


void pascal Act_Boss2(word handle)
{
  ActorState* state = gmActorStates + handle;

  // This table is a list of groups of 3: x offset, y offset, animation frame.
  //
  // [PERF] Missing `static` causes a copy operation here
  const sbyte FLY_TO_OTHER_SIDE_SEQ[] = {
    0,  1,  2,   0,  1,  2,   1,  2,  3,   1,  2,  3,   2,  1,  3,   2,  1,  3,
    2,  0,  3,   2,  0,  3,   2,  0,  3,   2,  0,  3,   2,  0,  3,   2,  0,  3,
    2,  0,  3,   2,  0,  3,   2,  0,  3,   2,  0,  3,   2,  0,  3,   2,  0,  3,
    2,  0,  3,   2,  0,  3,   2,  0,  3,   2,  0,  3,   2,  0,  3,   2,  0,  3,
    2,  0,  3,   2,  0,  3,   2,  0,  3,   2,  0,  3,   2,  0,  3,   2,  0,  3,
    2,  0,  3,   2,  0,  3,   2,  0,  3,   2, -1,  3,   2, -1,  3,   1, -2,  3,
    1, -2,  3,   0, -1,  3,   0, -1,  3
  };

  // This table is a list of groups of 3: x offset, y offset, animation frame.
  //
  // [PERF] Missing `static` causes a copy operation here
  const sbyte JUMP_TO_OTHER_SIDE_SEQ[] = {
    0, -2,  0,   0, -2,  0,   1, -2,  0,   2, -1,  0,   3,  0,  0,   2,  1,  0,
    1,  2,  0,   0,  2,  0,   0,  2,  0
  };

  if (state->var5) // death sequence (var5 is set in HandleActorShotCollision)
  {
    if (state->var5 == 1)
    {
      state->var5++;
      state->var3 = 2;
    }
    else
    {
      UpdateBossDeathSequence(handle);
    }
  }
  else if (state->var3) // wait
  {
    state->var3--;
    state->frame = gfxCurrentDisplayPage;
  }
  else if (state->var1 == 0) // initial wait upon activation
  {
    state->frame = gfxCurrentDisplayPage;

    if (state->var2++ == 30)
    {
      state->var1++;

      StopPreBossMusic();
      StartMusicPlayback(sndInGameMusicBuffer);

      HUD_DrawBossHealthBar(gmBossHealth);

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
      state->frame = gfxCurrentDisplayPage;
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
      state->frame = gfxCurrentDisplayPage;
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
      MD_DOWN, ACT_BOSS_EPISODE_2, 0, state->x, state->y + 1))
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
      state->frame = gfxCurrentDisplayPage;
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
      state->frame = gfxCurrentDisplayPage;
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


void Boss3_MoveTowardsPos(word* x, word* y, word targetX, word targetY)
{
  if (RandomNumber() & 1) // % 2
  {
    *x += Sign(targetX - *x - 4);
  }

  if (gfxCurrentDisplayPage)
  {
    *y += Sign(targetY - *y + 4);
  }
}


void pascal Act_Boss3(word handle)
{
  ActorState* state = gmActorStates + handle;

  if (state->var3 > 1)
  {
    UpdateBossDeathSequence(handle);
    return;
  }

  if (!state->var3)
  {
    StopPreBossMusic();
    StartMusicPlayback(sndInGameMusicBuffer);

    HUD_DrawBossHealthBar(gmBossHealth);

    state->var3 = 1;
  }

  Boss3_MoveTowardsPos(&state->x, &state->y, plPosX + 3, plPosY - 1);

  // Draw engine exhaust flames
  DrawActor(
    ACT_BOSS_EPISODE_3,
    1 + gfxCurrentDisplayPage,
    state->x,
    state->y,
    DS_NORMAL);

  //
  // Shoot rockets at player
  //
  if (
    IsActorOnScreen(handle) &&
    gfxCurrentDisplayPage &&
    (word)RandomNumber() % 2)
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

    if (Boss3_IsTouchingPlayer(handle))
    {
      state->x += 9;

      SpawnActor(ACT_ENEMY_ROCKET_LEFT, state->x - 4, state->y - 4);
      return;
    }

    // Player right of boss?
    state->x += 18; // first += 9 to undo the -9 above, then +9

    if (Boss3_IsTouchingPlayer(handle))
    {
      state->x -= 9;

      SpawnActor(ACT_ENEMY_ROCKET_RIGHT, state->x + 8, state->y - 4);
      return;
    }

    // Player above boss?
    state->x -= 9;
    state->y -= 9;

    if (Boss3_IsTouchingPlayer(handle))
    {
      state->y += 9;

      SpawnActor(ACT_ENEMY_ROCKET_2_UP, state->x + 4, state->y - 8);
      return;
    }

    // Player below boss?
    state->y += 18; // first += 9 to undo the -9 above, then +9

    if (Boss3_IsTouchingPlayer(handle))
    {
      state->y -= 9;

      SpawnActor(ACT_ENEMY_ROCKET_2_DOWN, state->x + 4, state->y + 3);
      return;
    }

    state->y -= 9;
  }
}


void pascal Act_Boss4(word handle)
{
  ActorState* state = gmActorStates + handle;

  if (!state->var3)
  {
    state->var3 = 1;

    StopPreBossMusic();
    HUD_DrawBossHealthBar(gmBossHealth);
    StartMusicPlayback(sndInGameMusicBuffer);
  }

  if (state->var3 > 1)
  {
    UpdateBossDeathSequence(handle);
    state->drawStyle = DS_INVISIBLE;
    return;
  }

  state->var1++;

  if (!state->var5 && state->var4 < 14 && gfxCurrentDisplayPage)
  {
    state->var4 = 0;

    if (state->x + 4 > plPosX)
    {
      state->x--;
      state->var4++;
    }
    else if (state->x + 4 < plPosX)
    {
      state->x++;
      state->var4++;
    }


    if (state->y + 4 > plPosY)
    {
      state->y--;
      state->var4++;
    }
    else if (state->y + 4 < plPosY)
    {
      state->y++;
      state->var4++;
    }
  }

  DrawActor(
    ACT_BOSS_EPISODE_4, state->var1 % 4 + 1, state->x, state->y, DS_NORMAL);

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

        SpawnActor(
          ACT_BOSS_EPISODE_4_SHOT, state->x + 4, state->y + 2);

        state->var2 = 0;
        state->var5 = 12;
      }
    }
  }
}


void MoveTowardsPos(word* x, word* y, word targetX, word targetY)
{
  *x += Sign(targetX - *x);
  *y += Sign(targetY - *y);
}


void pascal Act_Boss4Projectile(word handle)
{
  ActorState* state = gmActorStates + handle;

  state->var2 = !state->var2;

  if (state->var1 > 3) // Move towards player once animation finished
  {
    state->frame = 4;

    if (RandomNumber() & 3) // % 4
    {
      MoveTowardsPos(&state->x, &state->y, plPosX + 1, plPosY - 1);
    }
  }
  else if (state->var2) // Play appearing animation, advance every other frame
  {
    state->var1++;
    state->frame++;
  }
}


void pascal Act_SmallFlyingShip(word handle)
{
  register ActorState* state = gmActorStates + handle;
  register int i;

  // [PERF] Missing `static` causes a copy operation here
  const byte ANIM_SEQ[] = { 0, 1, 2, 1 };

  // Explode when hitting a wall, as if shot by the player (gives score)
  if (HAS_TILE_ATTRIBUTE(Map_GetTile(state->x - 1, state->y), TA_SOLID_RIGHT))
  {
    HandleActorShotCollision(1, handle);
    return;
  }

  // Determine initial distance to ground on first update
  if (!state->var1)
  {
    for (i = 0; i < 15; i++)
    {
      if (HAS_TILE_ATTRIBUTE(
        Map_GetTile(state->x, state->y + i), TA_SOLID_TOP))
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
      Map_GetTile(state->x, state->y + i), TA_SOLID_TOP))
    {
      state->y--;
      break;
    }
  }

  // Otherwise, float back down
  if (
    i == state->var1 &&
    !HAS_TILE_ATTRIBUTE(Map_GetTile(state->x, state->y + i), TA_SOLID_TOP))
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
  if (!IsActorOnScreen(handle) && state->x - 20 == plPosX)
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


void pascal Act_RigelatinSoldier(word handle)
{
  ActorState* state = gmActorStates + handle;

  const sbyte JUMP_SEQ[] = { -2, -2, -1, 0 };

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
    MD_DOWN, ACT_RIGELATIN_SOLDIER, 0, state->x, state->y + 1))
  {
    if (state->x < plPosX)
    {
      state->var1 = 1;
    }
    else
    {
      state->var1 = 0;
    }

    if ((word)RandomNumber() % 2)
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

      if ((word)RandomNumber() % 2)
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
        MD_RIGHT, ACT_RIGELATIN_SOLDIER, 0, state->x + 2, state->y))
      {
        state->x += 2;
      }
    }
    else
    {
      if (!CheckWorldCollision(
        MD_LEFT, ACT_RIGELATIN_SOLDIER, 0, state->x - 2, state->y))
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
    state->frame = state->frame + 3;

    if (state->var1)
    {
      SpawnEffect(
        ACT_RIGELATIN_SOLDIER_SHOT,
        state->x + 4,
        state->y - 4,
        EM_FLY_RIGHT,
        0);
    }
    else
    {
      SpawnEffect(
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
      state->frame = state->frame + 2;
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
bool SpawnActorInSlot(word slot, word id, word x, word y)
{
  switch (id)
  {
    case ACT_HOVERBOT:
      InitActorState(
        slot, Act_Hoverbot, ACT_HOVERBOT, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        gmDifficulty,        // health
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
        slot, Act_PlayerSprite, id, plPosX = x, plPosY = y,
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
      plActorId = id;
      break;

    case ACT_ROCKET_LAUNCHER:
      InitActorState(
        slot, Act_ItemBox, ACT_GREEN_BOX, x, y,
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
      gmWeaponsInLevel++;
      break;

    case ACT_FLAME_THROWER:
      InitActorState(
        slot, Act_ItemBox, ACT_GREEN_BOX, x, y,
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
      gmWeaponsInLevel++;
      break;

    case ACT_NORMAL_WEAPON:
      InitActorState(
        slot, Act_ItemBox, ACT_GREEN_BOX, x, y,
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
      gmWeaponsInLevel++;
      break;

    case ACT_LASER:
      InitActorState(
        slot, Act_ItemBox, ACT_GREEN_BOX, x, y,
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
      gmWeaponsInLevel++;
      break;

    case ACT_FLAME_THROWER_BOT_R:
    case ACT_FLAME_THROWER_BOT_L:
      InitActorState(
        slot, Act_FlameThrowerBot, id, x, y,
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
        slot, Act_ItemBox, ACT_RED_BOX, x, y,
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
      gmBombBoxesLeft++;
      break;

    case ACT_BLUE_BONUS_GLOBE_1:
      InitActorState(
        slot, Act_BonusGlobe, ACT_BONUS_GLOBE_SHELL, x, y,
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
      gmOrbsLeft++;
      break;

    case ACT_BLUE_BONUS_GLOBE_2:
      InitActorState(
        slot, Act_BonusGlobe, ACT_BONUS_GLOBE_SHELL, x, y,
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
      gmOrbsLeft++;
      break;

    case ACT_BLUE_BONUS_GLOBE_3:
      InitActorState(
        slot, Act_BonusGlobe, ACT_BONUS_GLOBE_SHELL, x, y,
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
      gmOrbsLeft++;
      break;

    case ACT_BLUE_BONUS_GLOBE_4:
      InitActorState(
        slot, Act_BonusGlobe, ACT_BONUS_GLOBE_SHELL, x, y,
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
      gmOrbsLeft++;
      break;

    case ACT_WATCHBOT:
      InitActorState(
        slot, Act_WatchBot, ACT_WATCHBOT, x, y,
        false,               // always update
        true,                // always update once activated
        true,                // allow stair stepping
        false,               // affected by gravity
        gmDifficulty + 5,    // health
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
        slot, Act_AnimatedProp, ACT_TELEPORTER_2, x, y,
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
        slot, Act_RocketTurret, ACT_ROCKET_LAUNCHER_TURRET, x, y,
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
        slot, Act_EnemyRocket, id, x, y,
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
        slot, Act_WatchBotContainerCarrier, ACT_WATCHBOT_CONTAINER_CARRIER, x, y,
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
        slot, Act_WatchBotContainer, ACT_WATCHBOT_CONTAINER, x, y,
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
        slot, Act_BomberPlane, ACT_BOMBER_PLANE, x, y,
        false,               // always update
        true,                // always update once activated
        true,                // allow stair stepping
        false,               // affected by gravity
        gmDifficulty + 5,    // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        5000);               // score
      break;

    case ACT_MINI_NUKE_SMALL:
      InitActorState(
        slot, Act_MiniNuke, ACT_MINI_NUKE_SMALL, x, y,
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
        slot, Act_MiniNuke, ACT_MINI_NUKE, x, y,
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
        slot, Act_SpikeBall, ACT_BOUNCING_SPIKE_BALL, x, y,
        false,               // always update
        true,                // always update once activated
        true,                // allow stair stepping
        false,               // affected by gravity
        gmDifficulty + 5,    // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        1000);               // score
      break;

    case ACT_ELECTRIC_REACTOR:
      InitActorState(
        slot, Act_Reactor, ACT_ELECTRIC_REACTOR, x, y,
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
        slot, Act_SlimeContainer, ACT_SLIME_CONTAINER, x, y,
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
        slot, Act_SlimeBlob, ACT_SLIME_BLOB, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        gmDifficulty + 5,    // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        1500);               // score
      break;

    case ACT_NUCLEAR_WASTE:
      InitActorState(
        slot, Act_ItemBox, ACT_NUCLEAR_WASTE_CAN_EMPTY, x, y,
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
        slot, Act_Snake, ACT_SNAKE, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        gmDifficulty + 7,    // health
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
        slot, Act_SecurityCamera, id, x, y,
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
      gmCamerasInLevel++;
      break;

    case ACT_CEILING_SUCKER:
      InitActorState(
        slot, Act_CeilingSucker, ACT_CEILING_SUCKER, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        gmDifficulty * 3 + 12, // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        300);                // score
      break;

    case ACT_DUKES_SHIP_R:
      InitActorState(
        slot, Act_PlayerShip, ACT_DUKES_SHIP_R, x, y,
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
        slot, Act_PlayerShip, ACT_DUKES_SHIP_L, x, y,
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
        slot, Act_PlayerShip, ACT_DUKES_SHIP_R, x, y,
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
        slot, Act_BrokenMissile, ACT_MISSILE_BROKEN, x, y,
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
        slot, Act_EyeBallThrower, ACT_EYEBALL_THROWER_L, x, y,
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
        slot, Act_MovingMapPartTrigger, ACT_DYNAMIC_GEOMETRY_1, x, y,
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
        slot, Act_MovingMapPartTrigger, ACT_DYNAMIC_GEOMETRY_2, x, y,
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
        slot, Act_HoverBotGenerator, ACT_HOVERBOT_GENERATOR, x, y,
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
        slot, Act_MovingMapPartTrigger, ACT_DYNAMIC_GEOMETRY_3, x, y,
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
        slot, Act_SlimePipe, ACT_SLIME_PIPE, x, y,
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
        slot, Act_SlimeDrop, ACT_SLIME_DROP, x, y,
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
        slot, Act_ForceField, ACT_FORCE_FIELD, x, y,
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
        slot, Act_KeyCardSlot, ACT_CIRCUIT_CARD_KEYHOLE, x, y,
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
        slot, Act_KeyHole, ACT_BLUE_KEY_KEYHOLE, x, y,
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
        slot, Act_SlidingDoorVertical, ACT_SLIDING_DOOR_VERTICAL, x, y,
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
        slot, Act_AnimatedProp, ACT_RADAR_DISH, x, y,
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
      gmRadarDishesLeft++;
      break;

    case ACT_KEYHOLE_MOUNTING_POLE:
    case ACT_LASER_TURRET_MOUNTING_POST:
      InitActorState(
        slot, Act_AnimatedProp, id, x, y,
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
        slot, Act_BlowingFan, ACT_BLOWING_FAN, x, y,
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
        slot, Act_LaserTurret, ACT_LASER_TURRET, x, y,
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
      gmTurretsInLevel++;
      break;

    case ACT_SLIDING_DOOR_HORIZONTAL:
      InitActorState(
        slot, Act_SlidingDoorHorizontal, ACT_SLIDING_DOOR_HORIZONTAL, x, y,
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
        slot, Act_RespawnBeacon, ACT_RESPAWN_CHECKPOINT, x, y,
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
        slot, Act_Skeleton, ACT_SKELETON, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        gmDifficulty + 1,    // health
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
        slot, Act_EnemyLaserShot, ACT_ENEMY_LASER_SHOT_L, x, y,
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
        slot, Act_MovingMapPartTrigger, ACT_DYNAMIC_GEOMETRY_4, x, y,
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
        slot, Act_MovingMapPartTrigger, ACT_DYNAMIC_GEOMETRY_5, x, y,
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
        slot, Act_LevelExitTrigger, ACT_EXIT_TRIGGER, x, y,
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
        slot, Act_MovingMapPartTrigger, ACT_DYNAMIC_GEOMETRY_6, x, y,
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
        slot, Act_MovingMapPartTrigger, ACT_DYNAMIC_GEOMETRY_7, x, y,
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
        slot, Act_MovingMapPartTrigger, ACT_DYNAMIC_GEOMETRY_8, x, y,
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
        slot, Act_SuperForceField, ACT_SUPER_FORCE_FIELD_L, x, y,
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
        slot, Act_IntactMissile, ACT_MISSILE_INTACT, x, y,
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
        slot, Act_GrabberClaw, ACT_METAL_GRABBER_CLAW, x, y,
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
        slot, Act_FloatingLaserBot, ACT_HOVERING_LASER_TURRET, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        gmDifficulty + 2,    // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        1000);               // score
      break;

    case ACT_SPIDER:
      InitActorState(
        slot, Act_Spider, ACT_SPIDER, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        gmDifficulty,        // health
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
        slot, Act_ItemBox, ACT_BLUE_BOX, x, y,
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
      gmMerchInLevel++;
      break;

    case ACT_BLUE_GUARD_R:
      InitActorState(
        slot, Act_BlueGuard, ACT_BLUE_GUARD_R, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        gmDifficulty + 1,    // health
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
        slot, Act_ItemBox, id, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        1,                   // health
        0,                   // var1
        -1,                  // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        100);                // score
      break;

    case ACT_BLUE_GUARD_L:
      InitActorState(
        slot, Act_BlueGuard, ACT_BLUE_GUARD_R, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        gmDifficulty + 1,    // health
        1,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        3000);               // score
      break;

    case ACT_NUCLEAR_WASTE_CAN_EMPTY:
      InitActorState(
        slot, Act_ItemBox, ACT_NUCLEAR_WASTE_CAN_EMPTY, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        true,                // affected by gravity
        1,                   // health
        0,                   // var1
        -1,                  // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        100);                // score
      break;

    case ACT_SODA_CAN:
    case ACT_SODA_6_PACK:
      InitActorState(
        slot, Act_ItemBox, ACT_RED_BOX, x, y,
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
        slot, Act_AnimatedProp, id, x, y,
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
        slot, Act_SpikedGreenCreature, id, x, y,
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
        slot, Act_GreenPanther, id, x, y,
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
        slot, Act_ItemBox, ACT_RED_BOX, x, y,
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
        slot, Act_Turkey, ACT_TURKEY, x, y,
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
        slot, Act_RedBird, ACT_RED_BIRD, x, y,
        false,               // always update
        true,                // always update once activated
        true,                // allow stair stepping
        false,               // affected by gravity
        gmDifficulty,        // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_UGLY_GREEN_BIRD:
      InitActorState(
        slot, Act_GreenBird, ACT_UGLY_GREEN_BIRD, x, y,
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
        slot, Act_ItemBox, ACT_WHITE_BOX, x, y,
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
        slot, Act_AnimatedProp, id, x, y,
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
        slot, Act_Elevator, ACT_ELEVATOR, x, y,
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
        slot, Act_AnimatedProp, id, x, y,
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
        slot, Act_AnimatedProp, id, x, y,
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
        slot, Act_MessengerDrone, ACT_MESSENGER_DRONE_BODY, x, y,
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
        slot, Act_BlueGuard, ACT_BLUE_GUARD_R, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        gmDifficulty + 1,    // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        2,                   // var5
        3000);               // score
      break;

    case ACT_SUPER_FORCE_FIELD_R:
      InitActorState(
        slot, Act_SuperForceField, ACT_SUPER_FORCE_FIELD_L, x, y,
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
        slot, Act_SmashHammer, ACT_SMASH_HAMMER, x, y,
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
        slot, Act_WaterArea, ACT_WATER_BODY, x, y,
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
      gmWaterAreasPresent = true;
      break;

    case ACT_WATER_DROP:
      InitActorState(
        slot, Act_WaterDrop, ACT_WATER_DROP, x, y,
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
        slot, Act_WaterDropSpawner, ACT_WATER_DROP_SPAWNER, x, y,
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
        slot, Act_LavaFountain, ACT_LAVA_FOUNTAIN, x, y,
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
        slot, Act_WaterArea, ACT_WATER_BODY, x, y,
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
      gmWaterAreasPresent = true;
      break;

    case ACT_WATER_AREA_4x4:
      InitActorState(
        slot, Act_WaterArea, ACT_WATER_BODY, x, y,
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
      gmWaterAreasPresent = true;
      break;

    case ACT_RADAR_COMPUTER_TERMINAL:
      InitActorState(
        slot, Act_RadarComputer, ACT_RADAR_COMPUTER_TERMINAL, x, y,
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
        slot, Act_AnimatedProp, ACT_SPECIAL_HINT_GLOBE, x, y,
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
        slot, Act_HintMachine, ACT_SPECIAL_HINT_MACHINE, x, y,
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
        slot, Act_WindBlownSpiderGenerator, ACT_WINDBLOWN_SPIDER_GENERATOR, x, y,
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
        slot, Act_UniCycleBot, ACT_UNICYCLE_BOT, x, y,
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
        slot, Act_WallWalker, ACT_WALL_WALKER, x, y,
        false,               // always update
        false,               // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        2,                   // health
        (word)RandomNumber() % 4,  // var1
        0,                   // var2
        20,                  // var3
        0,                   // var4
        0,                   // var5
        100);                // score
      break;

    case ACT_AIRLOCK_DEATH_TRIGGER_L:
    case ACT_AIRLOCK_DEATH_TRIGGER_R:
      InitActorState(
        slot, Act_AirlockDeathTrigger, id, x, y,
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
        slot, Act_AggressivePrisoner, ACT_AGGRESSIVE_PRISONER, x, y,
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
        slot, Act_ExplosionTrigger, ACT_EXPLOSION_FX_TRIGGER, x, y,
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
        slot, Act_Boss1, ACT_BOSS_EPISODE_1, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        gmBossHealth = gmDifficulty * 20 + 90, // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      break;

    case ACT_BOSS_EPISODE_2:
      InitActorState(
        slot, Act_Boss2, ACT_BOSS_EPISODE_2, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        gmBossHealth = gmDifficulty * 20 + 90, // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        y);                  // score
      break;

    case ACT_BOSS_EPISODE_3:
      InitActorState(
        slot, Act_Boss3, ACT_BOSS_EPISODE_3, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        gmBossHealth = gmDifficulty * 75 + 600, // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        0);                  // score
      gmBossHealth /= 4;
      break;

    case ACT_BOSS_EPISODE_4:
      InitActorState(
        slot, Act_Boss4, ACT_BOSS_EPISODE_4, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        gmBossHealth = gmDifficulty * 40 + 100, // health
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
        slot, Act_SmallFlyingShip, id, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        gmDifficulty + 1,    // health
        0,                   // var1
        0,                   // var2
        0,                   // var3
        0,                   // var4
        0,                   // var5
        1000);               // score
      break;

    case ACT_BOSS_EPISODE_4_SHOT:
      InitActorState(
        slot, Act_Boss4Projectile, ACT_BOSS_EPISODE_4_SHOT, x, y,
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
        slot, Act_RigelatinSoldier, ACT_RIGELATIN_SOLDIER, x, y,
        false,               // always update
        true,                // always update once activated
        false,               // allow stair stepping
        false,               // affected by gravity
        gmDifficulty * 2 + 25, // health
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

  return true;
}
