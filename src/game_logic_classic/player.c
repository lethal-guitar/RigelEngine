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


/** Sets inputXXX variables based on keyboard/joystick input, or demo data */
void ReadInput(void)
{
  byte keyboardInput = 0;
  byte scriptResult;

  // [NOTE] This seems like an odd place to call this function..
  // Doing this within UpdateAndDrawGame or RunInGameLoop would seem
  // more appropriate.
  HUD_DrawLowHealthAnimation(plHealth);

  if (!demoIsPlaying)
  {
    // Set input flags based on keyboard state, and check if any keyboard
    // input occured
    keyboardInput += (
      inputMoveUp = kbKeyState[kbBindingUp] | kbKeyState[SCANCODE_PGUP]);
    keyboardInput += (
      inputMoveDown = kbKeyState[kbBindingDown] | kbKeyState[SCANCODE_PGDOWN]);
    keyboardInput += (inputMoveLeft = kbKeyState[kbBindingLeft]);
    keyboardInput += (inputMoveRight = kbKeyState[kbBindingRight]);
    keyboardInput += (inputJump = kbKeyState[kbBindingJump]);
    keyboardInput += (inputFire = kbKeyState[kbBindingFire]);

    // If there was no keyboard input, poll the joystick if it's calibrated.
    // Note that there is no mechanism for the game to detect if a joystick is
    // actually plugged into the gameport.
    if (!keyboardInput && jsCalibrated)
    {
      PollJoystick();
    }

    // Check if the player wants to quit. This is handled here instead of
    // RunInGameLoop() because of demo recording.
    if (kbKeyState[SCANCODE_ESC] || kbKeyState[SCANCODE_Q])
    {
      scriptResult = ShowScriptedUI("2Quit_Select", "TEXT.MNI");

      if (scriptResult == 1)
      {
        gmGameState = GS_QUIT;
        FinishDemoRecording();
        FinishDemoPlayback();
      }
    }

    // Record demo input if applicable - always a no-op in the shipping
    // version of the game.
    if (RecordDemoInput())
    {
      // RecordDemoInput() returns true if a level change was requested
      gmGameState = GS_LEVEL_FINISHED;
      return; // [NOTE] Redundant
    }
  }
  else // demo playback
  {
    if (ReadDemoInput())
    {
      // ReadDemoInput() returns true when encountering a level change
      // command in the recorded demo data
      gmGameState = GS_LEVEL_FINISHED;
    }

    if (ANY_KEY_PRESSED())
    {
      gmGameState = GS_QUIT;
      demoPlaybackAborted = true;
    }
  }
}


void UpdatePlayer_Shooting(void)
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
    ACT_DUKE_FLAME_SHOT_RIGHT
  };

  const word* shotSpriteMap = SHOT_SPRITE_MAP + plWeapon * 4;

  // Update rapid fire pacing
  plRapidFireIsActiveFrame = !plRapidFireIsActiveFrame;

  if (
    plAnimationFrame == 28 ||
    plAnimationFrame == 5 ||
    plState == PS_RIDING_ELEVATOR)
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
  if (!inputFire)
  {
    if (plBlockShooting)
    {
      plBlockShooting = false;
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
    plWeapon == WPN_FLAMETHROWER ||
    plRapidFireTimeLeft ||
    plState == PS_USING_SHIP)
  {
    plBlockShooting = plRapidFireIsActiveFrame;
  }

  // Fire a shot if requested and allowed
  if (inputFire && !plBlockShooting)
  {
    plBlockShooting = true;

    // Recoil animation for regular standing pose, this is overwritten in
    // some of the cases below
    if (plAnimationFrame == 0)
    {
      plAnimationFrame = 18;
    }

    // Now we need to determine the right offset and direction for spawning
    // a shot, based on Duke's orientation and pose (as indicated by the
    // animation frame).
    if (plActorId == ACT_DUKES_SHIP_L)
    {
      SpawnPlayerShot(ACT_DUKES_SHIP_LASER_SHOT, plPosX - 3, plPosY, SD_LEFT);
    }
    else if (plActorId == ACT_DUKES_SHIP_R)
    {
      SpawnPlayerShot(ACT_DUKES_SHIP_LASER_SHOT, plPosX + 8, plPosY, SD_RIGHT);
    }
    else if (plActorId == ACT_DUKE_R)
    {
      if (plAnimationFrame == 37) // Flame thrower jetpack
      {
        SpawnPlayerShot(shotSpriteMap[1], plPosX + 1, plPosY + 1, SD_DOWN);
        plAnimationFrame = 38;
      }
      else if (plAnimationFrame == 16) // Shooting upwards
      {
        SpawnPlayerShot(shotSpriteMap[0], plPosX + 2, plPosY - 5, SD_UP);
        plAnimationFrame = 19;
      }
      else if (plAnimationFrame == 17) // Crouched
      {
        SpawnPlayerShot(shotSpriteMap[3], plPosX + 3, plPosY - 1, SD_RIGHT);
        plAnimationFrame = 34;
      }
      else if (plAnimationFrame == 20) // Hanging from pipe
      {
        SpawnPlayerShot(shotSpriteMap[3], plPosX + 3, plPosY - 2, SD_RIGHT);
        plAnimationFrame = 27;
      }
      else if (plAnimationFrame == 25) // Shooting down while hanging
      {
        SpawnPlayerShot(shotSpriteMap[1], plPosX + 0, plPosY + 1, SD_DOWN);
        plAnimationFrame = 26;
      }
      else // Regular standing pose, or walking
      {
        SpawnPlayerShot(shotSpriteMap[3], plPosX + 3, plPosY - 2, SD_RIGHT);
      }
    }
    else if (plActorId == ACT_DUKE_L)
    {
      if (plAnimationFrame == 16)
      {
        SpawnPlayerShot(shotSpriteMap[0], plPosX + 1, plPosY - 5, SD_UP);
        plAnimationFrame = 19;
      }
      else if (plAnimationFrame == 37)
      {
        SpawnPlayerShot(shotSpriteMap[1], plPosX + 2, plPosY + 1, SD_DOWN);
        plAnimationFrame = 38;
      }
      else if (plAnimationFrame == 17)
      {
        SpawnPlayerShot(shotSpriteMap[2], plPosX - 2, plPosY - 1, SD_LEFT);
        plAnimationFrame = 34;
      }
      else if (plAnimationFrame == 20)
      {
        SpawnPlayerShot(shotSpriteMap[2], plPosX - 2, plPosY - 2, SD_LEFT);
        plAnimationFrame = 27;
      }
      else if (plAnimationFrame == 25)
      {
        SpawnPlayerShot(shotSpriteMap[1], plPosX + 3, plPosY + 1, SD_DOWN);
        plAnimationFrame = 26;
      }
      else
      {
        SpawnPlayerShot(shotSpriteMap[2], plPosX - 2, plPosY - 2, SD_LEFT);
      }
    }

    // Ammo consumption and switching back to regular weapon when
    // ammo is depleted
    if (plWeapon != WPN_REGULAR && plState != PS_USING_SHIP)
    {
      plAmmo--;
      HUD_DrawAmmo(plAmmo);

      if (!plAmmo)
      {
        plWeapon = WPN_REGULAR;
        HUD_DrawWeapon(WPN_REGULAR);
        plAmmo = MAX_AMMO;
        HUD_DrawAmmo(MAX_AMMO);
      }
    }
  }
}


/** Respawn the ship pickup actor and adjust player back to normal */
void UpdatePlayer_LeaveShip(void)
{
  if (plActorId == ACT_DUKES_SHIP_L)
  {
    SpawnActor(ACT_DUKES_SHIP_AFTER_EXITING_L, plPosX, plPosY);
    plActorId = ACT_DUKE_L;
    plPosX += 3;
  }
  else
  {
    SpawnActor(ACT_DUKES_SHIP_AFTER_EXITING_R, plPosX, plPosY);
    plActorId = ACT_DUKE_R;
    plPosX += 1;
  }
}


/** Main player update function */
void UpdatePlayer(void)
{
  static bool doFlip = false;
  static byte vertScrollCooldown = 0;

  bool hadCollision;


  // A spider clinging to Duke's front side prevents shooting
  if (plAttachedSpider2)
  {
    inputFire = false;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Airlock death sequence
  //////////////////////////////////////////////////////////////////////////////
  if (plState == PS_AIRLOCK_DEATH_L || plState == PS_AIRLOCK_DEATH_R)
  {
    // [PERF] Missing `static` causes a copy operation here
    const int AIRLOCK_DEATH_ARC[] = { -2, -2, -1, -1, 0 };

    plPosY += AIRLOCK_DEATH_ARC[plAirlockDeathStep];

    plAnimationFrame++;
    if (plAnimationFrame == 16)
    {
      plAnimationFrame = 8;
    }

    if (plState == PS_AIRLOCK_DEATH_L)
    {
      plActorId = ACT_DUKE_L;
      plPosX -= 2;

      if (gmCameraPosX > 2)
      {
        gmCameraPosX -= 2;
      }
    }
    else
    {
      plActorId = ACT_DUKE_R;
      plPosX += 2;

      if (gmCameraPosX < mapWidth - (VIEWPORT_WIDTH + 2))
      {
        gmCameraPosX += 2;
      }
    }

    if (plPosX > mapWidth)
    {
      gmGameState = GS_PLAYER_DIED;
      PlaySound(SND_DUKE_DEATH);
    }

    if (!plAirlockDeathStep)
    {
      PlaySound(SND_DUKE_PAIN);
    }

    if (plAirlockDeathStep < 4)
    {
      plAirlockDeathStep++;
    }

    return;
  }

  if (plState != PS_GETTING_EATEN)
  {
    if (plState == PS_RIDING_ELEVATOR)
    {
      goto updateShooting;
    }

    plWalkAnimTicksDue = !plWalkAnimTicksDue;

    ////////////////////////////////////////////////////////////////////////////
    // Conveyor belt movement
    ////////////////////////////////////////////////////////////////////////////
    CheckWorldCollision(MD_DOWN, plActorId, 0, plPosX, plPosY + 1);

    if (retConveyorBeltCheckResult)
    {
      if (retConveyorBeltCheckResult == CB_LEFT)
      {
        if (!CheckWorldCollision(MD_LEFT, plActorId, 0, plPosX - 1, plPosY))
        {
          plPosX--;
        }
      }
      else if (retConveyorBeltCheckResult == CB_RIGHT)
      {
        if (!CheckWorldCollision(MD_RIGHT, plActorId, 0, plPosX + 1, plPosY))
        {
          plPosX++;
        }
      }
    }

    ////////////////////////////////////////////////////////////////////////////
    // Death animation
    ////////////////////////////////////////////////////////////////////////////
    if (plState == PS_DYING)
    {
      if (plKilledInShip)
      {
        UpdatePlayer_LeaveShip();
        plKilledInShip = false;
      }

      if (plAnimationFrame == 32 || plAnimationFrame == 0xFF)
      {
        plBodyExplosionStep++;

        if (plBodyExplosionStep >= 10)
        {
          plAnimationFrame = 0xFF;

          if (plBodyExplosionStep == 10)
          {
            SpawnEffect(
              ACT_DUKE_DEATH_PARTICLES, plPosX, plPosY, EM_NONE, 0);
          }

          if (
            plBodyExplosionStep & 1 && plBodyExplosionStep > 8 &&
            plBodyExplosionStep < 16)
          {
            if (RandomNumber() & 1)
            {
              PlaySound(SND_EXPLOSION);
            }
            else
            {
              PlaySound(SND_ALTERNATE_EXPLOSION);
            }
          }

          switch (plBodyExplosionStep)
          {
            case 10:
              SpawnParticles(plPosX + 2, plPosY, 0, 6);
              break;

            case 12:
              SpawnParticles(plPosX + 1, plPosY, 1, 3);
              break;

            case 14:
              SpawnParticles(plPosX + 2, plPosY, -1, 10);
              break;
          }

          if (plBodyExplosionStep == 35)
          {
            gmGameState = GS_PLAYER_DIED;
            plBodyExplosionStep = 0;
            return;
          }
        }
      }
      else
      {
        if (plDeathAnimationStep == 12)
        {
          ++plPosY;

          if (CheckWorldCollision(
            MD_DOWN, plActorId, plAnimationFrame, plPosX, plPosY + 1) ==
            CR_COLLISION)
          {
            plAnimationFrame = 32;
          }
          else
          {
            ++plPosY;

            if (CheckWorldCollision(
              MD_DOWN, plActorId, plAnimationFrame, plPosX, plPosY + 1) ==
              CR_COLLISION)
            {
              plAnimationFrame = 32;
            }
          }
        }
        else
        {
          plAnimationFrame = PL_DEATH_ANIMATION[plDeathAnimationStep];
          plPosY += PL_DEATH_ANIMATION[plDeathAnimationStep + 1];
          plDeathAnimationStep = plDeathAnimationStep + 2;
        }
      }
    }
    else
    {
      //////////////////////////////////////////////////////////////////////////
      // Movement
      //////////////////////////////////////////////////////////////////////////

      // Block input while unlocking a door/force field
      if (plState == PS_NORMAL && plInteractAnimTicks) { return; }

      // Attach to ladders
      if (
        plState != PS_CLIMBING_LADDER &&
        inputMoveUp &&
        CheckWorldCollision(MD_UP, plActorId, 36, plPosX, plPosY) == CR_LADDER &&
        (plState != PS_JUMPING || plJumpStep >= 4))
      {
        plState = PS_CLIMBING_LADDER;
        plAnimationFrame = 36;

        goto updateClimbingLadder;
      }

      // Filter inputs to avoid conflicting directional inputs
      if (inputMoveLeft & inputMoveRight)
      {
        inputMoveLeft = inputMoveRight = 0;
      }

      if (inputMoveUp & inputMoveDown)
      {
        inputMoveUp = inputMoveDown = 0;
      }


      //////////////////////////////////////////////////////////////////////////
      // Movement in ship
      //////////////////////////////////////////////////////////////////////////
      if (plState == PS_USING_SHIP)
      {
        static byte shipSpeed = 0;

        plAnimationFrame = 1;

        // Horizontal movement
        if (inputMoveLeft)
        {
          if (plActorId == ACT_DUKES_SHIP_R)
          {
            shipSpeed = 0;
          }

          plActorId = ACT_DUKES_SHIP_L;

          if (shipSpeed < 4)
          {
            shipSpeed++;
          }

          if (!CheckWorldCollision(
            MD_LEFT, ACT_DUKES_SHIP_L, 0, plPosX - 1, plPosY))
          {
            plPosX--;
          }
          else if (CheckWorldCollision(
            MD_DOWN, ACT_DUKES_SHIP_L, 0, plPosX, plPosY + 1))
          {
            plPosY--;
          }
          else if (CheckWorldCollision(
            MD_UP, ACT_DUKES_SHIP_L, 0, plPosX, plPosY - 1))
          {
            plPosY++;
          }

          if (shipSpeed == 4)
          {
            if (!CheckWorldCollision(
              MD_LEFT, ACT_DUKES_SHIP_L, 0, plPosX - 1, plPosY))
            {
              plPosX--;
            }
            else if (CheckWorldCollision(
              MD_DOWN, ACT_DUKES_SHIP_L, 0, plPosX, plPosY + 1))
            {
              plPosY--;
            }
            else if (CheckWorldCollision(
              MD_UP, ACT_DUKES_SHIP_L, 0, plPosX, plPosY - 1))
            {
              plPosY++;
            }
          }
          else
          {
            // No-op
          }
        }
        else if (inputMoveRight)
        {
          if (plActorId == ACT_DUKES_SHIP_L)
          {
            shipSpeed = 0;
          }

          plActorId = ACT_DUKES_SHIP_R;

          if (shipSpeed < 4)
          {
            shipSpeed++;
          }

          if (!CheckWorldCollision(
            MD_RIGHT, ACT_DUKES_SHIP_R, 0, plPosX + 1, plPosY))
          {
            plPosX++;
          }
          else if (CheckWorldCollision(
            MD_DOWN, ACT_DUKES_SHIP_L, 0, plPosX, plPosY + 1))
          {
            plPosY--;
          }
          else if (CheckWorldCollision(
            MD_UP, ACT_DUKES_SHIP_L, 0, plPosX, plPosY - 1))
          {
            plPosY++;
          }

          if (shipSpeed == 4)
          {
            if (!CheckWorldCollision(
              MD_RIGHT, ACT_DUKES_SHIP_R, 0, plPosX + 1, plPosY))
            {
              plPosX++;
            }
            else if (CheckWorldCollision(
              MD_DOWN, ACT_DUKES_SHIP_L, 0, plPosX, plPosY + 1))
            {
              plPosY--;
            }
            else if (CheckWorldCollision(
              MD_UP, ACT_DUKES_SHIP_L, 0, plPosX, plPosY - 1))
            {
              plPosY++;
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
        if (inputMoveUp)
        {
          if (!CheckWorldCollision(
            MD_UP, ACT_DUKES_SHIP_R, 0, plPosX, plPosY - 1))
          {
            plPosY--;
          }
        }

        if (inputMoveDown)
        {
          if (!CheckWorldCollision(
            MD_DOWN, ACT_DUKES_SHIP_R, 0, plPosX, plPosY + 1))
          {
            plPosY++;
          }
        }

        // Exit the ship when jumping
        if (!inputJump && plBlockJumping)
        {
          plBlockJumping = false;
        }

        if (inputJump && !plBlockJumping)
        {
          UpdatePlayer_LeaveShip();

          plBlockJumping = true;
          PlaySound(SND_DUKE_JUMPING);
          plState = PS_JUMPING;
          plJumpStep = 0;

          goto updateJumping;
        }
      }
      else
      {
        ////////////////////////////////////////////////////////////////////////
        // Regular movement
        ////////////////////////////////////////////////////////////////////////

        // Adjust sprite when changing orientation (left/right)
        if (inputMoveLeft)
        {
          plActorId = ACT_DUKE_L;
        }

        if (inputMoveRight)
        {
          plActorId = ACT_DUKE_R;
        }

        if (plState == PS_CLIMBING_LADDER)
        {
          goto updateClimbingLadder;
        }

        // Horizontal movement
        if (
          (inputMoveLeft || inputMoveRight) &&
          plState != PS_RECOVERING &&
          plAnimationFrame != 28)
        {
          if (plJumpStep == 1 && plState == PS_JUMPING)
          {
            goto updateJumping;
          }

          if (plState == PS_NORMAL && (inputMoveUp || inputMoveDown))
          {
            goto updateNormal;
          }

          if (plState == PS_HANGING)
          {
            if (inputMoveDown || inputFire)
            {
              goto updateHanging;
            }

            if (
              plActorId == ACT_DUKE_R &&
              !(hadCollision = CheckWorldCollision(
                MD_RIGHT, ACT_DUKE_R, 0, plPosX + 1, plPosY - 1)))
            {
              plPosX++;
            }
            else if (
              plActorId == ACT_DUKE_L &&
              !(hadCollision = CheckWorldCollision(
                MD_LEFT, ACT_DUKE_L, plAnimationFrame, plPosX, plPosY)))
            {
              plPosX--;
            }
          }
          else
          {
            if (
              plActorId == ACT_DUKE_R &&
              !(hadCollision = CheckWorldCollision(
                MD_RIGHT, ACT_DUKE_R, 0, plPosX + 1, plPosY)))
            {
              plPosX++;
            }
            else if (
              plActorId == ACT_DUKE_L &&
              !(hadCollision = CheckWorldCollision(
                MD_LEFT, ACT_DUKE_L, 0, plPosX - 1, plPosY)))
            {
              plPosX--;
            }
          }
        } // end horizontal movement

        // Activate flamethrower jetpack
        if (
          plWeapon == WPN_FLAMETHROWER &&
          inputMoveDown & inputFire)
        {
          plState = PS_USING_JETPACK;
          plAnimationFrame = 37;
        }

        // Jump/fall recovery frame
        if (plState == PS_RECOVERING)
        {
          plState = PS_NORMAL;
          PlaySound(SND_DUKE_LANDING);
        }

        // Flamethrower jetpack movement
        if (plState == PS_USING_JETPACK)
        {
          if (inputMoveDown & inputFire && plWeapon == WPN_FLAMETHROWER)
          {
            byte collisionCheck = CheckWorldCollision(
              MD_UP, ACT_DUKE_L, 37, plPosX, plPosY - 1);
            if (collisionCheck != CR_COLLISION)
            {
              plPosY--;
            }
          }
          else
          {
            plBlockJumping = true;
            plState = PS_FALLING;
            plFallingSpeed = 0;
            plAnimationFrame = 6;

            goto updateShooting;
          }
        }

updateNormal:
        if (plState == PS_NORMAL)
        {
          register word collisionCheck;

          if (!inputJump && plBlockJumping)
          {
            plBlockJumping = false;
          }

          if (
            inputJump && !plBlockJumping &&
            CheckWorldCollision(MD_UP, plActorId, 0, plPosX, plPosY - 1) !=
            CR_COLLISION)
          {
            plBlockJumping = true;
            PlaySound(SND_DUKE_JUMPING);
            plState = PS_JUMPING;
            plJumpStep = 0;

            goto updateJumping;
          }

          collisionCheck = CheckWorldCollision(
            MD_DOWN, plActorId, 0, plPosX, plPosY + 1);

          if (!collisionCheck || collisionCheck == CR_LADDER)
          {
            plState = PS_FALLING;
            plFallingSpeed = 0;

            goto updateFalling;
          }

          if (inputMoveUp && !plOnElevator)
          {
            plAnimationFrame = 16;
          }
          else if (inputMoveDown && !plOnElevator)
          {
            plAnimationFrame = 17;
          }
          else if ((inputMoveLeft || inputMoveRight) && !hadCollision)
          {
            if (plWalkAnimTicksDue)
            {
              plAnimationFrame++;
            }

            if (plAnimationFrame >= 5)
            {
              plAnimationFrame = 1;
            }
          }
          else
          {
            plAnimationFrame = 0;
          }
        }

updateHanging:
        if (plState == PS_HANGING)
        {
          word collCheck;

          if (CheckWorldCollision(
            MD_UP, plActorId, 0, plPosX, plPosY) == CR_CLIMBABLE)
          {
            plPosY++;
          }

          collCheck = CheckWorldCollision(
            MD_UP, plActorId, 0, plPosX, plPosY - 1);

          if (!inputJump && plBlockJumping)
          {
            plBlockJumping = false;
          }

          if (
            inputJump && !plBlockJumping && !inputMoveDown &&
            !CheckWorldCollision(MD_UP, plActorId, 0, plPosX, plPosY - 2))
          {
            plBlockJumping = true;
            PlaySound(SND_DUKE_JUMPING);
            plState = PS_JUMPING;
            plPosY--;
            plJumpStep = 1;

            goto updateJumping;
          }

          if (inputMoveDown && inputJump || collCheck != CR_CLIMBABLE)
          {
            plBlockJumping = true;
            plState = PS_FALLING;
            plFallingSpeed = 0;
            plAnimationFrame = 6;

            goto updateShooting;
          }

          if (inputMoveDown)
          {
            plAnimationFrame = 25;
          }
          else if (
            !inputFire && (inputMoveLeft || inputMoveRight) && !hadCollision)
          {
            if (plWalkAnimTicksDue)
            {
              plAnimationFrame++;
            }

            if (plAnimationFrame >= 25)
            {
              plAnimationFrame = 21;
            }
          }
          else
          {
            plAnimationFrame = 20;
          }

          if (inputMoveUp)
          {
            plAnimationFrame = 28;

            goto updateShooting;
          }
        }

        if (plState == PS_BLOWN_BY_FAN)
        {
          byte collisionCheck = CheckWorldCollision(
            MD_UP, plActorId, 0, plPosX, plPosY - 1);

          if (collisionCheck != CR_COLLISION)
          {
            plPosY--;
          }

          collisionCheck = CheckWorldCollision(
            MD_UP, plActorId, 0, plPosX, plPosY - 1);

          if (collisionCheck != CR_COLLISION)
          {
            plPosY--;
          }
        }

updateFalling:
        if (plState == PS_FALLING)
        {
          if (!inputJump && plBlockJumping)
          {
            plBlockJumping = false;
          }

          if (plFallingSpeed <= 4)
          {
            if (plFallingSpeed < 4)
            {
              plFallingSpeed++;
            }

            if (plFallingSpeed && CheckWorldCollision(
              MD_UP, plActorId, 0, plPosX, plPosY) == CR_CLIMBABLE)
            {
              plAnimationFrame = 20;
              plState = PS_HANGING;
              PlaySound(SND_ATTACH_CLIMBABLE);
              plPosY++;

              goto updateShooting;
            }

            if (
              CheckWorldCollision(
                MD_DOWN, plActorId, 0, plPosX, plPosY + 1) == CR_COLLISION)
            {
              if (plFallingSpeed == 4)
              {
                plState = PS_RECOVERING;
                plAnimationFrame = 5;

                goto updateShooting;
              }
              else
              {
                plState = PS_NORMAL;

                goto updateNormal;
              }
            }
            else
            {
              plPosY++;
            }

            plAnimationFrame = 7;
          }

          if (plFallingSpeed == 4)
          {
            if (CheckWorldCollision(
              MD_UP, plActorId, 0, plPosX, plPosY) == CR_CLIMBABLE)
            {
              plAnimationFrame = 20;
              plState = PS_HANGING;
              PlaySound(SND_ATTACH_CLIMBABLE);
              plPosY++;

              goto updateShooting;
            }

            if (
              CheckWorldCollision(
                MD_DOWN, plActorId, 0, plPosX, plPosY + 1) == CR_COLLISION)
            {
              if (plFallingSpeed == 4)
              {
                plState = PS_RECOVERING;
                plAnimationFrame = 5;

                goto updateShooting;
              }
              else
              {
                plState = PS_NORMAL;

                goto updateNormal;
              }
            }
            else
            {
              plPosY++;
            }

            plAnimationFrame = 8;
          }
        }

updateJumping:
        if (plState == PS_JUMPING)
        {
          static byte PL_JUMP_ARC[] = { 0, 2, 2, 1, 1, 1, 0, 0, 0 };

          register word collisionCheck;

          if (!inputJump && plBlockJumping)
          {
            plBlockJumping = false;
          }

          if (plJumpStep && plJumpStep < 3)
          {
            collisionCheck = CheckWorldCollision(
              MD_UP, plActorId, 0, plPosX, plPosY - 2);

            if (collisionCheck == CR_CLIMBABLE)
            {
              plAnimationFrame = 20;
              plPosY--;
              plState = PS_HANGING;
              PlaySound(SND_ATTACH_CLIMBABLE);

              goto updateHanging;
            }
            else if (collisionCheck == CR_COLLISION)
            {
              plJumpStep = 4;
              doFlip = false;
            }

            plAnimationFrame = 6;
          }

          if (plJumpStep < 9)
          {
            collisionCheck = CheckWorldCollision(
              MD_UP, plActorId, 0, plPosX, plPosY - 1);

            if (collisionCheck == CR_CLIMBABLE)
            {
              plAnimationFrame = 20;
              plState = PS_HANGING;
              PlaySound(SND_ATTACH_CLIMBABLE);

              goto updateHanging;
            }

            if (plJumpStep < 6 && collisionCheck == CR_COLLISION)
            {
              plFallingSpeed = 0;
              doFlip = false;
              plState = PS_FALLING;

              goto updateFalling;
            }
            else
            {
              plPosY -= PL_JUMP_ARC[plJumpStep];

              if (plJumpStep == 3 && (!inputJump || plAttachedSpider1))
              {
                plJumpStep = 6;
              }

              if (!plJumpStep)
              {
                plAnimationFrame = 5;
              }

              if (
                plJumpStep == 2 &&
                !doFlip &&
                plAttachedSpider1 == 0 &&
                plAttachedSpider2 == 0 &&
                plAttachedSpider3 == 0)
              {
                doFlip = !(RandomNumber() % 6);

                if (doFlip)
                {
                  plAnimationFrame = 8;
                }
                else
                {
                  plAnimationFrame = 6;
                }
              }

              if (doFlip)
              {
                plAnimationFrame++;

                if (
                  plAnimationFrame == 16 ||
                  (inputMoveLeft == false && inputMoveRight == false))
                {
                  plAnimationFrame = 6;
                  doFlip = false;
                }
              }

              plJumpStep++;

              goto updateClimbingLadder;
            }
          }

          plFallingSpeed = 0;
          doFlip = false;
          plState = PS_FALLING;

          goto updateFalling;
        }

updateClimbingLadder:
        if (plState == PS_CLIMBING_LADDER)
        {
          // [PERF] Missing `static` causes a copy operation here
          const byte LADDER_CLIMB_ANIM[] = { 35, 36};

          plFallingSpeed = 0;

          if (!inputJump && plBlockJumping)
          {
            plBlockJumping = false;
          }

          if (
            inputJump && !plBlockJumping &&
            CheckWorldCollision(MD_UP, plActorId, 36, plPosX, plPosY - 1) !=
              CR_COLLISION)
          {
            if (inputMoveLeft)
            {
              plPosX--;
            }

            if (inputMoveRight)
            {
              plPosX++;
            }

            plBlockJumping = true;
            plState = PS_JUMPING;
            PlaySound(SND_DUKE_JUMPING);
            plJumpStep = 1;

            goto updateJumping;
          }

          if (
            inputMoveUp &&
            CheckWorldCollision(MD_UP, plActorId, 36, plPosX, plPosY - 1) ==
              CR_LADDER)
          {
            plPosY--;

            if (plPosY % 2)
            {
              plAnimationFrame = LADDER_CLIMB_ANIM[
                plLadderAnimationStep = !plLadderAnimationStep];
            }
          }

          if (inputMoveDown)
          {
            if (CheckWorldCollision(
              MD_DOWN, plActorId, 36, plPosX, plPosY + 1) == CR_LADDER)
            {
              plPosY++;

              if (plPosY % 2)
              {
                plAnimationFrame = LADDER_CLIMB_ANIM[
                  plLadderAnimationStep = !plLadderAnimationStep];
              }
            }
            else
            {
              plFallingSpeed = 0;
              doFlip = false;
              plState = PS_FALLING;

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
    if (inputFire)
    {
      if (!retConveyorBeltCheckResult)
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
      plRapidFireIsActiveFrame = false;
    }

    if (kbKeyState[SCANCODE_PGUP] || kbKeyState[SCANCODE_PGDOWN])
    {
      vertScrollCooldown = 0;
    }

    if (plState != PS_DYING && plState != PS_CLIMBING_LADDER)
    {
      UpdatePlayer_Shooting();
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Camera movement (scrolling)
  //////////////////////////////////////////////////////////////////////////////

  // [NOTE] This is by far the most complex & hard to understand part of this
  // function. "Spaghetti code" is quite fitting here, as the actual logic is
  // hard to see amidst the mess of nested if-statements and repetitive code.

  // Horizontal
  if (plState == PS_USING_SHIP)
  {
    if (gmCameraPosX > 0 && plPosX - gmCameraPosX < 11)
    {
      gmCameraPosX--;
    }
    else if (gmCameraPosX < mapWidth - VIEWPORT_WIDTH &&
      plPosX - gmCameraPosX > 13)
    {
      gmCameraPosX++;
    }

    if (gmCameraPosX > 0 && plPosX - gmCameraPosX < 11)
    {
      gmCameraPosX--;
    }
    else if (gmCameraPosX < mapWidth - VIEWPORT_WIDTH &&
      plPosX - gmCameraPosX > 13)
    {
      gmCameraPosX++;
    }
  }
  else
  {
    if (gmCameraPosX > 0 && plPosX - gmCameraPosX < 10)
    {
      gmCameraPosX--;

      if (gmCameraPosX > 0 && plPosX - gmCameraPosX < 10)
      {
        gmCameraPosX--;
      }
    }
    else if (gmCameraPosX < mapWidth - VIEWPORT_WIDTH &&
      plPosX - gmCameraPosX > 18)
    {
      gmCameraPosX++;

      if (gmCameraPosX < mapWidth - VIEWPORT_WIDTH &&
        plPosX - gmCameraPosX > 18)
      {
        gmCameraPosX++;
      }
    }
  }

  // Vertical movement up, manual (normal state)
  if (
    plState == PS_NORMAL &&
    inputMoveUp &&
    gmCameraPosY != 0 &&
    plPosY - gmCameraPosY < 19 &&
    !plBlockLookingUp &&
    !plOnElevator)
  {
    if (gmCameraPosY < 2)
    {
      gmCameraPosY--;
    }
    else if (plPosY - gmCameraPosY < 18 && gmCameraPosY > 1)
    {
      gmCameraPosY -= 2;
    }

    if (plPosY - gmCameraPosY == 18)
    {
      gmCameraPosY--;
    }
  }

  // Vertical movement, automated
  if (
    plState == PS_USING_SHIP ||
    plState == PS_CLIMBING_LADDER ||
    plState == PS_USING_JETPACK ||
    plState == PS_BLOWN_BY_FAN ||
    plState == PS_RIDING_ELEVATOR ||
    (retConveyorBeltCheckResult && !inputMoveUp && !inputMoveDown))
  {
    if (gmCameraPosY > 0 && plPosY - gmCameraPosY < 11)
    {
      gmCameraPosY--;
    }
    else
    {
      if (gmCameraPosY < mapBottom - 19 && plPosY - gmCameraPosY > 12)
      {
        gmCameraPosY++;
      }

      if (
        plState == PS_RIDING_ELEVATOR &&
        gmCameraPosY < mapBottom - 19 && plPosY - gmCameraPosY > 12)
      {
        gmCameraPosY++;
      }
    }

    if (gmCameraPosY > 0 && plPosY - gmCameraPosY < 11)
    {
      gmCameraPosY--;
    }
    else if (gmCameraPosY < mapBottom - VIEWPORT_HEIGHT && plPosY - gmCameraPosY > 12)
    {
      gmCameraPosY++;
    }
  }
  else
  {
    // Vertical movement down, manual
    if (
      inputMoveDown &&
      (plState == PS_NORMAL || plState == PS_HANGING) &&
      !plOnElevator)
    {
      if (plState == PS_NORMAL && vertScrollCooldown)
      {
        vertScrollCooldown--;

        goto done;
      }
      else
      {
        if (plPosY - gmCameraPosY > 4 && gmCameraPosY + 19 < mapBottom)
        {
          gmCameraPosY++;
        }

        if (plPosY - gmCameraPosY > 4 && gmCameraPosY + 19 < mapBottom)
        {
          gmCameraPosY++;
        }
      }
    }
    // Vertical movement up, manual (hanging from a pipe)
    else if (inputMoveUp && plState == PS_HANGING && gmCameraPosY != 0)
    {
      if (vertScrollCooldown)
      {
        vertScrollCooldown--;

        goto done;
      }
      else
      {
        if (gmCameraPosY < 2)
        {
          gmCameraPosY--;
        }
        else
        {
          if (plPosY - gmCameraPosY < 18 && gmCameraPosY > 1)
          {
            gmCameraPosY -= 2;
          }

          if (plPosY - gmCameraPosY == 19)
          {
            gmCameraPosY++;
          }
        }
      }
    }

    // Some extra adjustments & special cases
    if (plPosY > 4096)
    {
      gmCameraPosY = 0;
    }
    else if (
      plState == PS_JUMPING && gmCameraPosY > 2 && plPosY - 2 < gmCameraPosY)
    {
      gmCameraPosY -= 2;
    }
    else
    {
      if (gmCameraPosY > 0 && plPosY - gmCameraPosY < 6)
      {
        gmCameraPosY--;
      }
      else if (gmCameraPosY < mapBottom - 18)
      {
        if (plPosY - gmCameraPosY > 18 && gmCameraPosY < mapBottom - 19)
        {
          gmCameraPosY++;
        }

        if (plPosY - gmCameraPosY > 18 && gmCameraPosY < mapBottom - 19)
        {
          gmCameraPosY++;
        }
      }
      else if (plPosY - gmCameraPosY >= 19)
      {
        gmCameraPosY++;
      }
    }
  }

done:
  plBlockLookingUp = false;
}


void WaitAndUpdatePlayer(void)
{
  WaitTicks(12 - gmSpeedIndex);

  ReadInput();
  UpdatePlayer();
}
