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

Player control logic

*******************************************************************************/


void UpdatePlayer_Shooting(Context* ctx)
{
  // Which sprite/actor id to use for each shot direction, for each
  // weapon type
  //
  // [PERF] Missing `static` causes a copy operation here
  const word SHOT_SPRITE_MAP[] = {
    ACT_REGULAR_SHOT_VERTICAL,
    ACT_REGULAR_SHOT_VERTICAL,
    ACT_REGULAR_SHOT_HORIZONTAL,
    ACT_REGULAR_SHOT_HORIZONTAL,
    ACT_DUKE_LASER_SHOT_VERTICAL,
    ACT_DUKE_LASER_SHOT_VERTICAL,
    ACT_DUKE_LASER_SHOT_HORIZONTAL,
    ACT_DUKE_LASER_SHOT_HORIZONTAL,
    ACT_DUKE_ROCKET_UP,
    ACT_DUKE_ROCKET_DOWN,
    ACT_DUKE_ROCKET_LEFT,
    ACT_DUKE_ROCKET_RIGHT,
    ACT_DUKE_FLAME_SHOT_UP,
    ACT_DUKE_FLAME_SHOT_DOWN,
    ACT_DUKE_FLAME_SHOT_LEFT,
    ACT_DUKE_FLAME_SHOT_RIGHT};

  const word* shotSpriteMap = SHOT_SPRITE_MAP + ctx->plWeapon * 4;

  // Update rapid fire pacing
  ctx->plRapidFireIsActiveFrame = !ctx->plRapidFireIsActiveFrame;

  if (
    ctx->plAnimationFrame == 28 || ctx->plAnimationFrame == 5 ||
    ctx->plState == PS_RIDING_ELEVATOR)
  {
    // The player can't shoot while pulling up his legs hanging from a pipe
    // (frame 28), recovering from landing (frame 5), or when riding an
    // elevator. There a few more cases, which are handled in UpdatePlayer
    // by completely skipping the call to UpdatePlayer_Shooting.
    return;
  }

  // After having fired a shot, the player needs to let go of the fire button
  // before being able to shoot again. This is implemented by the following
  // code.
  if (!ctx->inputFire)
  {
    if (ctx->plBlockShooting)
    {
      ctx->plBlockShooting = false;
    }
  }

  // Implement rapid fire
  //
  // [BUG] Because plRapidFireIsActiveFrame isn't reset when picking up the
  // powerup, there's no consistency for what happens on the frame after
  // it's picked up if the fire button is already held down at that point.
  // Depending on the state of plRapidFireIsActiveFrame, the player starts
  // shooting immediately, or one frame later.
  // One way to fix this would be to reset plRapidFireIsActiveFrame to zero
  // when not shooting, and to alternate it only when the fire button is
  // pressed.
  if (
    ctx->plWeapon == WPN_FLAMETHROWER || ctx->plRapidFireTimeLeft ||
    ctx->plState == PS_USING_SHIP)
  {
    ctx->plBlockShooting = ctx->plRapidFireIsActiveFrame;
  }

  // Fire a shot if requested and allowed
  if (ctx->inputFire && !ctx->plBlockShooting)
  {
    ctx->plBlockShooting = true;

    // Recoil animation for regular standing pose, this is overwritten in
    // some of the cases below
    if (ctx->plAnimationFrame == 0)
    {
      ctx->plAnimationFrame = 18;
    }

    // Now we need to determine the right offset and direction for spawning
    // a shot, based on Duke's orientation and pose (as indicated by the
    // animation frame).
    if (ctx->plActorId == ACT_DUKES_SHIP_L)
    {
      SpawnPlayerShot(
        ctx, ACT_DUKES_SHIP_LASER_SHOT, ctx->plPosX - 3, ctx->plPosY, SD_LEFT);
    }
    else if (ctx->plActorId == ACT_DUKES_SHIP_R)
    {
      SpawnPlayerShot(
        ctx, ACT_DUKES_SHIP_LASER_SHOT, ctx->plPosX + 8, ctx->plPosY, SD_RIGHT);
    }
    else if (ctx->plActorId == ACT_DUKE_R)
    {
      if (ctx->plAnimationFrame == 37) // Flame thrower jetpack
      {
        SpawnPlayerShot(
          ctx, shotSpriteMap[1], ctx->plPosX + 1, ctx->plPosY + 1, SD_DOWN);
        ctx->plAnimationFrame = 38;
      }
      else if (ctx->plAnimationFrame == 16) // Shooting upwards
      {
        SpawnPlayerShot(
          ctx, shotSpriteMap[0], ctx->plPosX + 2, ctx->plPosY - 5, SD_UP);
        ctx->plAnimationFrame = 19;
      }
      else if (ctx->plAnimationFrame == 17) // Crouched
      {
        SpawnPlayerShot(
          ctx, shotSpriteMap[3], ctx->plPosX + 3, ctx->plPosY - 1, SD_RIGHT);
        ctx->plAnimationFrame = 34;
      }
      else if (ctx->plAnimationFrame == 20) // Hanging from pipe
      {
        SpawnPlayerShot(
          ctx, shotSpriteMap[3], ctx->plPosX + 3, ctx->plPosY - 2, SD_RIGHT);
        ctx->plAnimationFrame = 27;
      }
      else if (ctx->plAnimationFrame == 25) // Shooting down while hanging
      {
        SpawnPlayerShot(
          ctx, shotSpriteMap[1], ctx->plPosX + 0, ctx->plPosY + 1, SD_DOWN);
        ctx->plAnimationFrame = 26;
      }
      else // Regular standing pose, or walking
      {
        SpawnPlayerShot(
          ctx, shotSpriteMap[3], ctx->plPosX + 3, ctx->plPosY - 2, SD_RIGHT);
      }
    }
    else if (ctx->plActorId == ACT_DUKE_L)
    {
      if (ctx->plAnimationFrame == 16)
      {
        SpawnPlayerShot(
          ctx, shotSpriteMap[0], ctx->plPosX + 1, ctx->plPosY - 5, SD_UP);
        ctx->plAnimationFrame = 19;
      }
      else if (ctx->plAnimationFrame == 37)
      {
        SpawnPlayerShot(
          ctx, shotSpriteMap[1], ctx->plPosX + 2, ctx->plPosY + 1, SD_DOWN);
        ctx->plAnimationFrame = 38;
      }
      else if (ctx->plAnimationFrame == 17)
      {
        SpawnPlayerShot(
          ctx, shotSpriteMap[2], ctx->plPosX - 2, ctx->plPosY - 1, SD_LEFT);
        ctx->plAnimationFrame = 34;
      }
      else if (ctx->plAnimationFrame == 20)
      {
        SpawnPlayerShot(
          ctx, shotSpriteMap[2], ctx->plPosX - 2, ctx->plPosY - 2, SD_LEFT);
        ctx->plAnimationFrame = 27;
      }
      else if (ctx->plAnimationFrame == 25)
      {
        SpawnPlayerShot(
          ctx, shotSpriteMap[1], ctx->plPosX + 3, ctx->plPosY + 1, SD_DOWN);
        ctx->plAnimationFrame = 26;
      }
      else
      {
        SpawnPlayerShot(
          ctx, shotSpriteMap[2], ctx->plPosX - 2, ctx->plPosY - 2, SD_LEFT);
      }
    }

    // Ammo consumption and switching back to regular weapon when
    // ammo is depleted
    if (ctx->plWeapon != WPN_REGULAR && ctx->plState != PS_USING_SHIP)
    {
      ctx->plAmmo--;

      if (!ctx->plAmmo)
      {
        ctx->plWeapon = WPN_REGULAR;
        ctx->plAmmo = MAX_AMMO;
      }
    }
  }
}


/** Respawn the ship pickup actor and adjust player back to normal */
void UpdatePlayer_LeaveShip(Context* ctx)
{
  if (ctx->plActorId == ACT_DUKES_SHIP_L)
  {
    SpawnActor(ctx, ACT_DUKES_SHIP_AFTER_EXITING_L, ctx->plPosX, ctx->plPosY);
    ctx->plActorId = ACT_DUKE_L;
    ctx->plPosX += 3;
  }
  else
  {
    SpawnActor(ctx, ACT_DUKES_SHIP_AFTER_EXITING_R, ctx->plPosX, ctx->plPosY);
    ctx->plActorId = ACT_DUKE_R;
    ctx->plPosX += 1;
  }
}


/** Main player update function */
void UpdatePlayer(Context* ctx)
{
  static bool doFlip = false;
  static byte vertScrollCooldown = 0;
  static byte plLadderAnimationStep = 0;

  bool hadCollision;


  // A spider clinging to Duke's front side prevents shooting
  if (ctx->plAttachedSpider2)
  {
    ctx->inputFire = false;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Airlock death sequence
  //////////////////////////////////////////////////////////////////////////////
  if (ctx->plState == PS_AIRLOCK_DEATH_L || ctx->plState == PS_AIRLOCK_DEATH_R)
  {
    // [PERF] Missing `static` causes a copy operation here
    const int16_t AIRLOCK_DEATH_ARC[] = {-2, -2, -1, -1, 0};

    ctx->plPosY += AIRLOCK_DEATH_ARC[ctx->plAirlockDeathStep];

    ctx->plAnimationFrame++;
    if (ctx->plAnimationFrame == 16)
    {
      ctx->plAnimationFrame = 8;
    }

    if (ctx->plState == PS_AIRLOCK_DEATH_L)
    {
      ctx->plActorId = ACT_DUKE_L;
      ctx->plPosX -= 2;

      if (ctx->gmCameraPosX > 2)
      {
        ctx->gmCameraPosX -= 2;
      }
    }
    else
    {
      ctx->plActorId = ACT_DUKE_R;
      ctx->plPosX += 2;

      if (ctx->gmCameraPosX < ctx->mapWidth - (VIEWPORT_WIDTH + 2))
      {
        ctx->gmCameraPosX += 2;
      }
    }

    if (ctx->plPosX > ctx->mapWidth)
    {
      ctx->gmGameState = GS_PLAYER_DIED;
      PlaySound(ctx, SND_DUKE_DEATH);
    }

    if (!ctx->plAirlockDeathStep)
    {
      PlaySound(ctx, SND_DUKE_PAIN);
    }

    if (ctx->plAirlockDeathStep < 4)
    {
      ctx->plAirlockDeathStep++;
    }

    return;
  }

  if (ctx->plState != PS_GETTING_EATEN)
  {
    if (ctx->plState == PS_RIDING_ELEVATOR)
    {
      goto updateShooting;
    }

    ctx->plWalkAnimTicksDue = !ctx->plWalkAnimTicksDue;

    ////////////////////////////////////////////////////////////////////////////
    // Conveyor belt movement
    ////////////////////////////////////////////////////////////////////////////
    CheckWorldCollision(
      ctx, MD_DOWN, ctx->plActorId, 0, ctx->plPosX, ctx->plPosY + 1);

    if (ctx->retConveyorBeltCheckResult)
    {
      if (ctx->retConveyorBeltCheckResult == CB_LEFT)
      {
        if (!CheckWorldCollision(
              ctx, MD_LEFT, ctx->plActorId, 0, ctx->plPosX - 1, ctx->plPosY))
        {
          ctx->plPosX--;
        }
      }
      else if (ctx->retConveyorBeltCheckResult == CB_RIGHT)
      {
        if (!CheckWorldCollision(
              ctx, MD_RIGHT, ctx->plActorId, 0, ctx->plPosX + 1, ctx->plPosY))
        {
          ctx->plPosX++;
        }
      }
    }

    ////////////////////////////////////////////////////////////////////////////
    // Death animation
    ////////////////////////////////////////////////////////////////////////////
    if (ctx->plState == PS_DYING)
    {
      if (ctx->plKilledInShip)
      {
        UpdatePlayer_LeaveShip(ctx);
        ctx->plKilledInShip = false;
      }

      if (ctx->plAnimationFrame == 32 || ctx->plAnimationFrame == 0xFF)
      {
        ctx->plBodyExplosionStep++;

        if (ctx->plBodyExplosionStep >= 10)
        {
          ctx->plAnimationFrame = 0xFF;

          if (ctx->plBodyExplosionStep == 10)
          {
            SpawnEffect(
              ctx,
              ACT_DUKE_DEATH_PARTICLES,
              ctx->plPosX,
              ctx->plPosY,
              EM_NONE,
              0);
          }

          if (
            ctx->plBodyExplosionStep & 1 && ctx->plBodyExplosionStep > 8 &&
            ctx->plBodyExplosionStep < 16)
          {
            if (RandomNumber(ctx) & 1)
            {
              PlaySound(ctx, SND_EXPLOSION);
            }
            else
            {
              PlaySound(ctx, SND_ALTERNATE_EXPLOSION);
            }
          }

          switch (ctx->plBodyExplosionStep)
          {
            case 10:
              SpawnParticles(ctx, ctx->plPosX + 2, ctx->plPosY, 0, 6);
              break;

            case 12:
              SpawnParticles(ctx, ctx->plPosX + 1, ctx->plPosY, 1, 3);
              break;

            case 14:
              SpawnParticles(ctx, ctx->plPosX + 2, ctx->plPosY, -1, 10);
              break;
          }

          if (ctx->plBodyExplosionStep == 35)
          {
            ctx->gmGameState = GS_PLAYER_DIED;
            ctx->plBodyExplosionStep = 0;
            return;
          }
        }
      }
      else
      {
        if (ctx->plDeathAnimationStep == 12)
        {
          ++ctx->plPosY;

          if (
            CheckWorldCollision(
              ctx,
              MD_DOWN,
              ctx->plActorId,
              ctx->plAnimationFrame,
              ctx->plPosX,
              ctx->plPosY + 1) == CR_COLLISION)
          {
            ctx->plAnimationFrame = 32;
          }
          else
          {
            ++ctx->plPosY;

            if (
              CheckWorldCollision(
                ctx,
                MD_DOWN,
                ctx->plActorId,
                ctx->plAnimationFrame,
                ctx->plPosX,
                ctx->plPosY + 1) == CR_COLLISION)
            {
              ctx->plAnimationFrame = 32;
            }
          }
        }
        else
        {
          ctx->plAnimationFrame = PL_DEATH_ANIMATION[ctx->plDeathAnimationStep];
          ctx->plPosY += PL_DEATH_ANIMATION[ctx->plDeathAnimationStep + 1];
          ctx->plDeathAnimationStep = ctx->plDeathAnimationStep + 2;
        }
      }
    }
    else
    {
      //////////////////////////////////////////////////////////////////////////
      // Movement
      //////////////////////////////////////////////////////////////////////////

      // Block input while unlocking a door/force field
      if (ctx->plState == PS_NORMAL && ctx->plInteractAnimTicks)
      {
        return;
      }

      // Attach to ladders
      if (
        ctx->plState != PS_CLIMBING_LADDER && ctx->inputMoveUp &&
        CheckWorldCollision(
          ctx, MD_UP, ctx->plActorId, 36, ctx->plPosX, ctx->plPosY) ==
          CR_LADDER &&
        (ctx->plState != PS_JUMPING || ctx->plJumpStep >= 4))
      {
        ctx->plState = PS_CLIMBING_LADDER;
        ctx->plAnimationFrame = 36;

        goto updateClimbingLadder;
      }

      // Filter inputs to avoid conflicting directional inputs
      if (ctx->inputMoveLeft & ctx->inputMoveRight)
      {
        ctx->inputMoveLeft = ctx->inputMoveRight = 0;
      }

      if (ctx->inputMoveUp & ctx->inputMoveDown)
      {
        ctx->inputMoveUp = ctx->inputMoveDown = 0;
      }


      //////////////////////////////////////////////////////////////////////////
      // Movement in ship
      //////////////////////////////////////////////////////////////////////////
      if (ctx->plState == PS_USING_SHIP)
      {
        static byte shipSpeed = 0;

        ctx->plAnimationFrame = 1;

        // Horizontal movement
        if (ctx->inputMoveLeft)
        {
          if (ctx->plActorId == ACT_DUKES_SHIP_R)
          {
            shipSpeed = 0;
          }

          ctx->plActorId = ACT_DUKES_SHIP_L;

          if (shipSpeed < 4)
          {
            shipSpeed++;
          }

          if (!CheckWorldCollision(
                ctx,
                MD_LEFT,
                ACT_DUKES_SHIP_L,
                0,
                ctx->plPosX - 1,
                ctx->plPosY))
          {
            ctx->plPosX--;
          }
          else if (CheckWorldCollision(
                     ctx,
                     MD_DOWN,
                     ACT_DUKES_SHIP_L,
                     0,
                     ctx->plPosX,
                     ctx->plPosY + 1))
          {
            ctx->plPosY--;
          }
          else if (CheckWorldCollision(
                     ctx,
                     MD_UP,
                     ACT_DUKES_SHIP_L,
                     0,
                     ctx->plPosX,
                     ctx->plPosY - 1))
          {
            ctx->plPosY++;
          }

          if (shipSpeed == 4)
          {
            if (!CheckWorldCollision(
                  ctx,
                  MD_LEFT,
                  ACT_DUKES_SHIP_L,
                  0,
                  ctx->plPosX - 1,
                  ctx->plPosY))
            {
              ctx->plPosX--;
            }
            else if (CheckWorldCollision(
                       ctx,
                       MD_DOWN,
                       ACT_DUKES_SHIP_L,
                       0,
                       ctx->plPosX,
                       ctx->plPosY + 1))
            {
              ctx->plPosY--;
            }
            else if (CheckWorldCollision(
                       ctx,
                       MD_UP,
                       ACT_DUKES_SHIP_L,
                       0,
                       ctx->plPosX,
                       ctx->plPosY - 1))
            {
              ctx->plPosY++;
            }
          }
          else
          {
            // No-op
          }
        }
        else if (ctx->inputMoveRight)
        {
          if (ctx->plActorId == ACT_DUKES_SHIP_L)
          {
            shipSpeed = 0;
          }

          ctx->plActorId = ACT_DUKES_SHIP_R;

          if (shipSpeed < 4)
          {
            shipSpeed++;
          }

          if (!CheckWorldCollision(
                ctx,
                MD_RIGHT,
                ACT_DUKES_SHIP_R,
                0,
                ctx->plPosX + 1,
                ctx->plPosY))
          {
            ctx->plPosX++;
          }
          else if (CheckWorldCollision(
                     ctx,
                     MD_DOWN,
                     ACT_DUKES_SHIP_L,
                     0,
                     ctx->plPosX,
                     ctx->plPosY + 1))
          {
            ctx->plPosY--;
          }
          else if (CheckWorldCollision(
                     ctx,
                     MD_UP,
                     ACT_DUKES_SHIP_L,
                     0,
                     ctx->plPosX,
                     ctx->plPosY - 1))
          {
            ctx->plPosY++;
          }

          if (shipSpeed == 4)
          {
            if (!CheckWorldCollision(
                  ctx,
                  MD_RIGHT,
                  ACT_DUKES_SHIP_R,
                  0,
                  ctx->plPosX + 1,
                  ctx->plPosY))
            {
              ctx->plPosX++;
            }
            else if (CheckWorldCollision(
                       ctx,
                       MD_DOWN,
                       ACT_DUKES_SHIP_L,
                       0,
                       ctx->plPosX,
                       ctx->plPosY + 1))
            {
              ctx->plPosY--;
            }
            else if (CheckWorldCollision(
                       ctx,
                       MD_UP,
                       ACT_DUKES_SHIP_L,
                       0,
                       ctx->plPosX,
                       ctx->plPosY - 1))
            {
              ctx->plPosY++;
            }
          }
          else
          {
            // No-op
          }
        }
        else
        {
          shipSpeed = 0;
        }

        // Vertical movement
        if (ctx->inputMoveUp)
        {
          if (!CheckWorldCollision(
                ctx, MD_UP, ACT_DUKES_SHIP_R, 0, ctx->plPosX, ctx->plPosY - 1))
          {
            ctx->plPosY--;
          }
        }

        if (ctx->inputMoveDown)
        {
          if (!CheckWorldCollision(
                ctx,
                MD_DOWN,
                ACT_DUKES_SHIP_R,
                0,
                ctx->plPosX,
                ctx->plPosY + 1))
          {
            ctx->plPosY++;
          }
        }

        // Exit the ship when jumping
        if (!ctx->inputJump && ctx->plBlockJumping)
        {
          ctx->plBlockJumping = false;
        }

        if (ctx->inputJump && !ctx->plBlockJumping)
        {
          UpdatePlayer_LeaveShip(ctx);

          ctx->plBlockJumping = true;
          PlaySound(ctx, SND_DUKE_JUMPING);
          ctx->plState = PS_JUMPING;
          ctx->plJumpStep = 0;

          goto updateJumping;
        }
      }
      else
      {
        ////////////////////////////////////////////////////////////////////////
        // Regular movement
        ////////////////////////////////////////////////////////////////////////

        // Adjust sprite when changing orientation (left/right)
        if (ctx->inputMoveLeft)
        {
          ctx->plActorId = ACT_DUKE_L;
        }

        if (ctx->inputMoveRight)
        {
          ctx->plActorId = ACT_DUKE_R;
        }

        if (ctx->plState == PS_CLIMBING_LADDER)
        {
          goto updateClimbingLadder;
        }

        // Horizontal movement
        if (
          (ctx->inputMoveLeft || ctx->inputMoveRight) &&
          ctx->plState != PS_RECOVERING && ctx->plAnimationFrame != 28)
        {
          if (ctx->plJumpStep == 1 && ctx->plState == PS_JUMPING)
          {
            goto updateJumping;
          }

          if (
            ctx->plState == PS_NORMAL &&
            (ctx->inputMoveUp || ctx->inputMoveDown))
          {
            goto updateNormal;
          }

          if (ctx->plState == PS_HANGING)
          {
            if (ctx->inputMoveDown || ctx->inputFire)
            {
              goto updateHanging;
            }

            if (
              ctx->plActorId == ACT_DUKE_R &&
              !(hadCollision = CheckWorldCollision(
                  ctx,
                  MD_RIGHT,
                  ACT_DUKE_R,
                  0,
                  ctx->plPosX + 1,
                  ctx->plPosY - 1)))
            {
              ctx->plPosX++;
            }
            else if (
              ctx->plActorId == ACT_DUKE_L &&
              !(hadCollision = CheckWorldCollision(
                  ctx,
                  MD_LEFT,
                  ACT_DUKE_L,
                  ctx->plAnimationFrame,
                  ctx->plPosX,
                  ctx->plPosY)))
            {
              ctx->plPosX--;
            }
          }
          else
          {
            if (
              ctx->plActorId == ACT_DUKE_R &&
              !(hadCollision = CheckWorldCollision(
                  ctx, MD_RIGHT, ACT_DUKE_R, 0, ctx->plPosX + 1, ctx->plPosY)))
            {
              ctx->plPosX++;
            }
            else if (
              ctx->plActorId == ACT_DUKE_L &&
              !(hadCollision = CheckWorldCollision(
                  ctx, MD_LEFT, ACT_DUKE_L, 0, ctx->plPosX - 1, ctx->plPosY)))
            {
              ctx->plPosX--;
            }
          }
        } // end horizontal movement

        // Activate flamethrower jetpack
        if (
          ctx->plWeapon == WPN_FLAMETHROWER &&
          ctx->inputMoveDown & ctx->inputFire)
        {
          ctx->plState = PS_USING_JETPACK;
          ctx->plAnimationFrame = 37;
        }

        // Jump/fall recovery frame
        if (ctx->plState == PS_RECOVERING)
        {
          ctx->plState = PS_NORMAL;
          PlaySound(ctx, SND_DUKE_LANDING);
        }

        // Flamethrower jetpack movement
        if (ctx->plState == PS_USING_JETPACK)
        {
          if (
            ctx->inputMoveDown & ctx->inputFire &&
            ctx->plWeapon == WPN_FLAMETHROWER)
          {
            byte collisionCheck = CheckWorldCollision(
              ctx, MD_UP, ACT_DUKE_L, 37, ctx->plPosX, ctx->plPosY - 1);
            if (collisionCheck != CR_COLLISION)
            {
              ctx->plPosY--;
            }
          }
          else
          {
            ctx->plBlockJumping = true;
            ctx->plState = PS_FALLING;
            ctx->plFallingSpeed = 0;
            ctx->plAnimationFrame = 6;

            goto updateShooting;
          }
        }

      updateNormal:
        if (ctx->plState == PS_NORMAL)
        {
          register word collisionCheck;

          if (!ctx->inputJump && ctx->plBlockJumping)
          {
            ctx->plBlockJumping = false;
          }

          if (
            ctx->inputJump && !ctx->plBlockJumping &&
            CheckWorldCollision(
              ctx, MD_UP, ctx->plActorId, 0, ctx->plPosX, ctx->plPosY - 1) !=
              CR_COLLISION)
          {
            ctx->plBlockJumping = true;
            PlaySound(ctx, SND_DUKE_JUMPING);
            ctx->plState = PS_JUMPING;
            ctx->plJumpStep = 0;

            goto updateJumping;
          }

          collisionCheck = CheckWorldCollision(
            ctx, MD_DOWN, ctx->plActorId, 0, ctx->plPosX, ctx->plPosY + 1);

          if (!collisionCheck || collisionCheck == CR_LADDER)
          {
            ctx->plState = PS_FALLING;
            ctx->plFallingSpeed = 0;

            goto updateFalling;
          }

          if (ctx->inputMoveUp && !ctx->plOnElevator)
          {
            ctx->plAnimationFrame = 16;
          }
          else if (ctx->inputMoveDown && !ctx->plOnElevator)
          {
            ctx->plAnimationFrame = 17;
          }
          else if ((ctx->inputMoveLeft || ctx->inputMoveRight) && !hadCollision)
          {
            if (ctx->plWalkAnimTicksDue)
            {
              ctx->plAnimationFrame++;
            }

            if (ctx->plAnimationFrame >= 5)
            {
              ctx->plAnimationFrame = 1;
            }
          }
          else
          {
            ctx->plAnimationFrame = 0;
          }
        }

      updateHanging:
        if (ctx->plState == PS_HANGING)
        {
          word collCheck;

          if (
            CheckWorldCollision(
              ctx, MD_UP, ctx->plActorId, 0, ctx->plPosX, ctx->plPosY) ==
            CR_CLIMBABLE)
          {
            ctx->plPosY++;
          }

          collCheck = CheckWorldCollision(
            ctx, MD_UP, ctx->plActorId, 0, ctx->plPosX, ctx->plPosY - 1);

          if (!ctx->inputJump && ctx->plBlockJumping)
          {
            ctx->plBlockJumping = false;
          }

          if (
            ctx->inputJump && !ctx->plBlockJumping && !ctx->inputMoveDown &&
            !CheckWorldCollision(
              ctx, MD_UP, ctx->plActorId, 0, ctx->plPosX, ctx->plPosY - 2))
          {
            ctx->plBlockJumping = true;
            PlaySound(ctx, SND_DUKE_JUMPING);
            ctx->plState = PS_JUMPING;
            ctx->plPosY--;
            ctx->plJumpStep = 1;

            goto updateJumping;
          }

          if (
            (ctx->inputMoveDown && ctx->inputJump) || collCheck != CR_CLIMBABLE)
          {
            ctx->plBlockJumping = true;
            ctx->plState = PS_FALLING;
            ctx->plFallingSpeed = 0;
            ctx->plAnimationFrame = 6;

            goto updateShooting;
          }

          if (ctx->inputMoveDown)
          {
            ctx->plAnimationFrame = 25;
          }
          else if (
            !ctx->inputFire && (ctx->inputMoveLeft || ctx->inputMoveRight) &&
            !hadCollision)
          {
            if (ctx->plWalkAnimTicksDue)
            {
              ctx->plAnimationFrame++;
            }

            if (ctx->plAnimationFrame >= 25)
            {
              ctx->plAnimationFrame = 21;
            }
          }
          else
          {
            ctx->plAnimationFrame = 20;
          }

          if (ctx->inputMoveUp)
          {
            ctx->plAnimationFrame = 28;

            goto updateShooting;
          }
        }

        if (ctx->plState == PS_BLOWN_BY_FAN)
        {
          byte collisionCheck = CheckWorldCollision(
            ctx, MD_UP, ctx->plActorId, 0, ctx->plPosX, ctx->plPosY - 1);

          if (collisionCheck != CR_COLLISION)
          {
            ctx->plPosY--;
          }

          collisionCheck = CheckWorldCollision(
            ctx, MD_UP, ctx->plActorId, 0, ctx->plPosX, ctx->plPosY - 1);

          if (collisionCheck != CR_COLLISION)
          {
            ctx->plPosY--;
          }
        }

      updateFalling:
        if (ctx->plState == PS_FALLING)
        {
          if (!ctx->inputJump && ctx->plBlockJumping)
          {
            ctx->plBlockJumping = false;
          }

          if (ctx->plFallingSpeed <= 4)
          {
            if (ctx->plFallingSpeed < 4)
            {
              ctx->plFallingSpeed++;
            }

            if (
              ctx->plFallingSpeed &&
              CheckWorldCollision(
                ctx, MD_UP, ctx->plActorId, 0, ctx->plPosX, ctx->plPosY) ==
                CR_CLIMBABLE)
            {
              ctx->plAnimationFrame = 20;
              ctx->plState = PS_HANGING;
              PlaySound(ctx, SND_ATTACH_CLIMBABLE);
              ctx->plPosY++;

              goto updateShooting;
            }

            if (
              CheckWorldCollision(
                ctx,
                MD_DOWN,
                ctx->plActorId,
                0,
                ctx->plPosX,
                ctx->plPosY + 1) == CR_COLLISION)
            {
              if (ctx->plFallingSpeed == 4)
              {
                ctx->plState = PS_RECOVERING;
                ctx->plAnimationFrame = 5;

                goto updateShooting;
              }
              else
              {
                ctx->plState = PS_NORMAL;

                goto updateNormal;
              }
            }
            else
            {
              ctx->plPosY++;
            }

            ctx->plAnimationFrame = 7;
          }

          if (ctx->plFallingSpeed == 4)
          {
            if (
              CheckWorldCollision(
                ctx, MD_UP, ctx->plActorId, 0, ctx->plPosX, ctx->plPosY) ==
              CR_CLIMBABLE)
            {
              ctx->plAnimationFrame = 20;
              ctx->plState = PS_HANGING;
              PlaySound(ctx, SND_ATTACH_CLIMBABLE);
              ctx->plPosY++;

              goto updateShooting;
            }

            if (
              CheckWorldCollision(
                ctx,
                MD_DOWN,
                ctx->plActorId,
                0,
                ctx->plPosX,
                ctx->plPosY + 1) == CR_COLLISION)
            {
              if (ctx->plFallingSpeed == 4)
              {
                ctx->plState = PS_RECOVERING;
                ctx->plAnimationFrame = 5;

                goto updateShooting;
              }
              else
              {
                ctx->plState = PS_NORMAL;

                goto updateNormal;
              }
            }
            else
            {
              ctx->plPosY++;
            }

            ctx->plAnimationFrame = 8;
          }
        }

      updateJumping:
        if (ctx->plState == PS_JUMPING)
        {
          static byte PL_JUMP_ARC[] = {0, 2, 2, 1, 1, 1, 0, 0, 0};

          register word collisionCheck;

          if (!ctx->inputJump && ctx->plBlockJumping)
          {
            ctx->plBlockJumping = false;
          }

          if (ctx->plJumpStep && ctx->plJumpStep < 3)
          {
            collisionCheck = CheckWorldCollision(
              ctx, MD_UP, ctx->plActorId, 0, ctx->plPosX, ctx->plPosY - 2);

            if (collisionCheck == CR_CLIMBABLE)
            {
              ctx->plAnimationFrame = 20;
              ctx->plPosY--;
              ctx->plState = PS_HANGING;
              PlaySound(ctx, SND_ATTACH_CLIMBABLE);

              goto updateHanging;
            }
            else if (collisionCheck == CR_COLLISION)
            {
              ctx->plJumpStep = 4;
              doFlip = false;
            }

            ctx->plAnimationFrame = 6;
          }

          if (ctx->plJumpStep < 9)
          {
            collisionCheck = CheckWorldCollision(
              ctx, MD_UP, ctx->plActorId, 0, ctx->plPosX, ctx->plPosY - 1);

            if (collisionCheck == CR_CLIMBABLE)
            {
              ctx->plAnimationFrame = 20;
              ctx->plState = PS_HANGING;
              PlaySound(ctx, SND_ATTACH_CLIMBABLE);

              goto updateHanging;
            }

            if (ctx->plJumpStep < 6 && collisionCheck == CR_COLLISION)
            {
              ctx->plFallingSpeed = 0;
              doFlip = false;
              ctx->plState = PS_FALLING;

              goto updateFalling;
            }
            else
            {
              ctx->plPosY -= PL_JUMP_ARC[ctx->plJumpStep];

              if (
                ctx->plJumpStep == 3 &&
                (!ctx->inputJump || ctx->plAttachedSpider1))
              {
                ctx->plJumpStep = 6;
              }

              if (!ctx->plJumpStep)
              {
                ctx->plAnimationFrame = 5;
              }

              if (
                ctx->plJumpStep == 2 && !doFlip &&
                ctx->plAttachedSpider1 == 0 && ctx->plAttachedSpider2 == 0 &&
                ctx->plAttachedSpider3 == 0)
              {
                doFlip = !(RandomNumber(ctx) % 6);

                if (doFlip)
                {
                  ctx->plAnimationFrame = 8;
                }
                else
                {
                  ctx->plAnimationFrame = 6;
                }
              }

              if (doFlip)
              {
                ctx->plAnimationFrame++;

                if (
                  ctx->plAnimationFrame == 16 ||
                  (ctx->inputMoveLeft == false && ctx->inputMoveRight == false))
                {
                  ctx->plAnimationFrame = 6;
                  doFlip = false;
                }
              }

              ctx->plJumpStep++;

              goto updateClimbingLadder;
            }
          }

          ctx->plFallingSpeed = 0;
          doFlip = false;
          ctx->plState = PS_FALLING;

          goto updateFalling;
        }

      updateClimbingLadder:
        if (ctx->plState == PS_CLIMBING_LADDER)
        {
          // [PERF] Missing `static` causes a copy operation here
          const byte LADDER_CLIMB_ANIM[] = {35, 36};

          ctx->plFallingSpeed = 0;

          if (!ctx->inputJump && ctx->plBlockJumping)
          {
            ctx->plBlockJumping = false;
          }

          if (
            ctx->inputJump && !ctx->plBlockJumping &&
            CheckWorldCollision(
              ctx, MD_UP, ctx->plActorId, 36, ctx->plPosX, ctx->plPosY - 1) !=
              CR_COLLISION)
          {
            if (ctx->inputMoveLeft)
            {
              ctx->plPosX--;
            }

            if (ctx->inputMoveRight)
            {
              ctx->plPosX++;
            }

            ctx->plBlockJumping = true;
            ctx->plState = PS_JUMPING;
            PlaySound(ctx, SND_DUKE_JUMPING);
            ctx->plJumpStep = 1;

            goto updateJumping;
          }

          if (
            ctx->inputMoveUp &&
            CheckWorldCollision(
              ctx, MD_UP, ctx->plActorId, 36, ctx->plPosX, ctx->plPosY - 1) ==
              CR_LADDER)
          {
            ctx->plPosY--;

            if (ctx->plPosY % 2)
            {
              ctx->plAnimationFrame = LADDER_CLIMB_ANIM
                [plLadderAnimationStep = !plLadderAnimationStep];
            }
          }

          if (ctx->inputMoveDown)
          {
            if (
              CheckWorldCollision(
                ctx,
                MD_DOWN,
                ctx->plActorId,
                36,
                ctx->plPosX,
                ctx->plPosY + 1) == CR_LADDER)
            {
              ctx->plPosY++;

              if (ctx->plPosY % 2)
              {
                ctx->plAnimationFrame = LADDER_CLIMB_ANIM
                  [plLadderAnimationStep = !plLadderAnimationStep];
              }
            }
            else
            {
              ctx->plFallingSpeed = 0;
              doFlip = false;
              ctx->plState = PS_FALLING;

              goto updateFalling;
            }
          }
        }
      }
    }

    ////////////////////////////////////////////////////////////////////////////
    // Shooting
    ////////////////////////////////////////////////////////////////////////////
  updateShooting:
    if (ctx->inputFire)
    {
      if (!ctx->retConveyorBeltCheckResult)
      {
        vertScrollCooldown = 5;
      }
      else
      {
        vertScrollCooldown = 0;
      }
    }
    else
    {
      ctx->plRapidFireIsActiveFrame = false;
    }


    if (ctx->plState != PS_DYING && ctx->plState != PS_CLIMBING_LADDER)
    {
      UpdatePlayer_Shooting(ctx);
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Camera movement (scrolling)
  //////////////////////////////////////////////////////////////////////////////

  // [NOTE] This is by far the most complex & hard to understand part of this
  // function. "Spaghetti code" is quite fitting here, as the actual logic is
  // hard to see amidst the mess of nested if-statements and repetitive code.

  // Horizontal
  if (ctx->plState == PS_USING_SHIP)
  {
    if (ctx->gmCameraPosX > 0 && ctx->plPosX - ctx->gmCameraPosX < 11)
    {
      ctx->gmCameraPosX--;
    }
    else if (
      ctx->gmCameraPosX < ctx->mapWidth - VIEWPORT_WIDTH &&
      ctx->plPosX - ctx->gmCameraPosX > 13)
    {
      ctx->gmCameraPosX++;
    }

    if (ctx->gmCameraPosX > 0 && ctx->plPosX - ctx->gmCameraPosX < 11)
    {
      ctx->gmCameraPosX--;
    }
    else if (
      ctx->gmCameraPosX < ctx->mapWidth - VIEWPORT_WIDTH &&
      ctx->plPosX - ctx->gmCameraPosX > 13)
    {
      ctx->gmCameraPosX++;
    }
  }
  else
  {
    if (ctx->gmCameraPosX > 0 && ctx->plPosX - ctx->gmCameraPosX < 10)
    {
      ctx->gmCameraPosX--;

      if (ctx->gmCameraPosX > 0 && ctx->plPosX - ctx->gmCameraPosX < 10)
      {
        ctx->gmCameraPosX--;
      }
    }
    else if (
      ctx->gmCameraPosX < ctx->mapWidth - VIEWPORT_WIDTH &&
      ctx->plPosX - ctx->gmCameraPosX > 18)
    {
      ctx->gmCameraPosX++;

      if (
        ctx->gmCameraPosX < ctx->mapWidth - VIEWPORT_WIDTH &&
        ctx->plPosX - ctx->gmCameraPosX > 18)
      {
        ctx->gmCameraPosX++;
      }
    }
  }

  // Vertical movement up, manual (normal state)
  if (
    ctx->plState == PS_NORMAL && ctx->inputMoveUp && ctx->gmCameraPosY != 0 &&
    ctx->plPosY - ctx->gmCameraPosY < 19 && !ctx->plBlockLookingUp &&
    !ctx->plOnElevator)
  {
    if (ctx->gmCameraPosY < 2)
    {
      ctx->gmCameraPosY--;
    }
    else if (ctx->plPosY - ctx->gmCameraPosY < 18 && ctx->gmCameraPosY > 1)
    {
      ctx->gmCameraPosY -= 2;
    }

    if (ctx->plPosY - ctx->gmCameraPosY == 18)
    {
      ctx->gmCameraPosY--;
    }
  }

  // Vertical movement, automated
  if (
    ctx->plState == PS_USING_SHIP || ctx->plState == PS_CLIMBING_LADDER ||
    ctx->plState == PS_USING_JETPACK || ctx->plState == PS_BLOWN_BY_FAN ||
    ctx->plState == PS_RIDING_ELEVATOR ||
    (ctx->retConveyorBeltCheckResult && !ctx->inputMoveUp &&
     !ctx->inputMoveDown))
  {
    if (ctx->gmCameraPosY > 0 && ctx->plPosY - ctx->gmCameraPosY < 11)
    {
      ctx->gmCameraPosY--;
    }
    else
    {
      if (
        ctx->gmCameraPosY < ctx->mapBottom - 19 &&
        ctx->plPosY - ctx->gmCameraPosY > 12)
      {
        ctx->gmCameraPosY++;
      }

      if (
        ctx->plState == PS_RIDING_ELEVATOR &&
        ctx->gmCameraPosY < ctx->mapBottom - 19 &&
        ctx->plPosY - ctx->gmCameraPosY > 12)
      {
        ctx->gmCameraPosY++;
      }
    }

    if (ctx->gmCameraPosY > 0 && ctx->plPosY - ctx->gmCameraPosY < 11)
    {
      ctx->gmCameraPosY--;
    }
    else if (
      ctx->gmCameraPosY < ctx->mapBottom - VIEWPORT_HEIGHT &&
      ctx->plPosY - ctx->gmCameraPosY > 12)
    {
      ctx->gmCameraPosY++;
    }
  }
  else
  {
    // Vertical movement down, manual
    if (
      ctx->inputMoveDown &&
      (ctx->plState == PS_NORMAL || ctx->plState == PS_HANGING) &&
      !ctx->plOnElevator)
    {
      if (ctx->plState == PS_NORMAL && vertScrollCooldown)
      {
        vertScrollCooldown--;

        goto done;
      }
      else
      {
        if (
          ctx->plPosY - ctx->gmCameraPosY > 4 &&
          ctx->gmCameraPosY + 19 < ctx->mapBottom)
        {
          ctx->gmCameraPosY++;
        }

        if (
          ctx->plPosY - ctx->gmCameraPosY > 4 &&
          ctx->gmCameraPosY + 19 < ctx->mapBottom)
        {
          ctx->gmCameraPosY++;
        }
      }
    }
    // Vertical movement up, manual (hanging from a pipe)
    else if (
      ctx->inputMoveUp && ctx->plState == PS_HANGING && ctx->gmCameraPosY != 0)
    {
      if (vertScrollCooldown)
      {
        vertScrollCooldown--;

        goto done;
      }
      else
      {
        if (ctx->gmCameraPosY < 2)
        {
          ctx->gmCameraPosY--;
        }
        else
        {
          if (ctx->plPosY - ctx->gmCameraPosY < 18 && ctx->gmCameraPosY > 1)
          {
            ctx->gmCameraPosY -= 2;
          }

          if (ctx->plPosY - ctx->gmCameraPosY == 19)
          {
            ctx->gmCameraPosY++;
          }
        }
      }
    }

    // Some extra adjustments & special cases
    if (ctx->plPosY > 4096)
    {
      ctx->gmCameraPosY = 0;
    }
    else if (
      ctx->plState == PS_JUMPING && ctx->gmCameraPosY > 2 &&
      ctx->plPosY - 2 < ctx->gmCameraPosY)
    {
      ctx->gmCameraPosY -= 2;
    }
    else
    {
      if (ctx->gmCameraPosY > 0 && ctx->plPosY - ctx->gmCameraPosY < 6)
      {
        ctx->gmCameraPosY--;
      }
      else if (ctx->gmCameraPosY < ctx->mapBottom - 18)
      {
        if (
          ctx->plPosY - ctx->gmCameraPosY > 18 &&
          ctx->gmCameraPosY < ctx->mapBottom - 19)
        {
          ctx->gmCameraPosY++;
        }

        if (
          ctx->plPosY - ctx->gmCameraPosY > 18 &&
          ctx->gmCameraPosY < ctx->mapBottom - 19)
        {
          ctx->gmCameraPosY++;
        }
      }
      else if (ctx->plPosY - ctx->gmCameraPosY >= 19)
      {
        ctx->gmCameraPosY++;
      }
    }
  }

done:
  ctx->plBlockLookingUp = false;
}
