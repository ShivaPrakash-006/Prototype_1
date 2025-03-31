#include "player.h"
#include "includes.h"
#include "objects.h"

void playerInit(Player *player) {
  player->armor = 3;
  player->armorText = NULL;
  player->score = 0;

  player->width = 50;
  player->height = 50;

  player->posX = 10;
  player->posY = 10;
  player->rot = 90;

  player->moveSpeed = 300;
  player->rotSpeed = 300;

  player->rotVel = 0;
  player->moving = false;
  player->afterburning = false;

  player->velX = SDL_sinf(player->rot * PI / 180);
  player->velY = -SDL_cosf(player->rot * PI / 180);

  player->rect.w = player->width;
  player->rect.h = player->height;
  player->rect.x = player->posX;
  player->rect.y = player->posY;

  player->maxAfterBurnerFuel = 1000;
  player->afterBurnerRefuelRate = 0.1;
  player->afterBurnerDepletionRate = 0.5;
  player->afterBurnerFuel = player->maxAfterBurnerFuel;
  player->afterBurnerOverheat = false;
  player->afterBurnerCooldown = 5000; /* 5 Secs */

  player->leapDistance = 100;

  /* Player Bullets */
  player->shooting = false;
  player->bulletTimer = timerInit();
  timerStart(&player->bulletTimer);
  player->shootDelay = 200;

  player->icon = NULL;

  player->bullets.bullet.posX = -150;
  player->bullets.bullet.posY = -150;
  player->bullets.bullet.speed = 0;
  player->bullets.bullet.width = 0;
  player->bullets.bullet.height = 0;
  player->bullets.bullet.damage = -1;
  player->bullets.bullet.bulletNum = 0;
  player->bullets.nextBullet = NULL;

  /* PowerUps */
  player->shield = true;
  player->shieldTime = 5000;
  player->shieldTimer = timerInit();
  player->shieldBlinker = timerInit();
  player->shieldBlink = false;
  player->repair = false;
  player->infFuel = false;
  player->infFuelTime = 5000;
  player->infFuelTimer = timerInit();
  player->multiBullet = false;
  player->multiBullTime = 5000;
  player->multiBullTimer = timerInit();

  player->gameTimer = timerInit();
  timerStart(&player->gameTimer);
}

void playerLogs(Player player) {
  printf("posX: %f\n", player.posX);
  printf("posY: %f\n", player.posY);
  printf("rot: %f\n", player.rot);
  printf("rotVel: %f\n", player.rotVel);
  printf("moving: %i\n", player.moving);
  printf("velX: %f\n", player.velX);
  printf("velY: %f\n", player.velY);
}

void playerShoot(Player *player, int num) {
  Bullet bullet; /* Creating A Bullet*/

  /* Initialization of bullet */
  bullet.width = 10;
  bullet.height = 40;

  bullet.damage = 1;
  bullet.speed = 1000;

  if (num == 1) {
    bullet.rot = player->rot + 10;
  } else if (num == 2) {
    bullet.rot = player->rot - 10;
  } else if (num == 3) {
    bullet.rot = player->rot + 20;
  } else if (num == 4) {
    bullet.rot = player->rot - 20;
  }
  bullet.velX = SDL_sinf(bullet.rot * PI / 180);
  bullet.velY = -SDL_cosf(bullet.rot * PI / 180);

  bullet.posX =
      player->posX + player->width / 2.f - bullet.width / 2.f; /* Center of X */
  bullet.posY = player->posY + player->height / 2.f -
                bullet.height / 2.f; /* Center of Y */

  bullet.rect.w = bullet.width;
  bullet.rect.h = bullet.height;
  bullet.rect.x = bullet.posX;
  bullet.rect.y = bullet.posY;

  bullet.rot = player->rot;
  bullet.center.x = bullet.rect.w;
  bullet.center.y = 0;

  bullet.lifeTimer = timerInit();
  bullet.lifeTime = 1500; /* 1.5 secs */
  timerStart(&bullet.lifeTimer);

  /* Linked List For Optimised Allocation */
  BulletNode *bulletPtr, *prevPtr, *newBullet;
  uint bulletNum = 1;
  prevPtr = &player->bullets;
  bulletPtr = player->bullets.nextBullet;
  newBullet = (BulletNode *)malloc(sizeof(BulletNode));

  while (bulletPtr != NULL) {
    if (bulletNum == bulletPtr->bullet.bulletNum) {
      prevPtr = bulletPtr;
      bulletPtr = bulletPtr->nextBullet;
      bulletNum++;
    } else {
      break;
    }
  }

  newBullet->bullet = bullet;
  newBullet->bullet.bulletNum = bulletNum;
  newBullet->nextBullet = bulletPtr;
  prevPtr->nextBullet = newBullet;
}

void playerBulletHander(Player *player, double delta, Mix_Chunk *shootSfx) {
  if (player->shooting && player->bulletTimer.ticks > player->shootDelay) {
    Mix_PlayChannel(1, shootSfx, 0);
    playerShoot(player, 1);
    playerShoot(player, 2);
    if (player->multiBullTimer.started) {
      playerShoot(player, 3);
      playerShoot(player, 4);
    }
    timerReset(&player->bulletTimer);
  }

  BulletNode *bulletPtr = &player->bullets;
  while (bulletPtr != NULL) {
    /* Moving the bullets */
    bulletPtr->bullet.posX +=
        bulletPtr->bullet.velX * bulletPtr->bullet.speed * delta;
    bulletPtr->bullet.posY +=
        bulletPtr->bullet.velY * bulletPtr->bullet.speed * delta;

    bulletPtr->bullet.rect.x = bulletPtr->bullet.posX;
    bulletPtr->bullet.rect.y = bulletPtr->bullet.posY;

    /* Looping */
    if (bulletPtr->bullet.posX + bulletPtr->bullet.width < 0) {
      bulletPtr->bullet.posX = WIDTH; /* Left to Right */
    }

    else if (bulletPtr->bullet.posX > WIDTH) {
      bulletPtr->bullet.posX = 0 - bulletPtr->bullet.width; /* Right to Left */
    }

    if (bulletPtr->bullet.posY + bulletPtr->bullet.height < 0) {
      bulletPtr->bullet.posY = HEIGHT; /* Top to Bottom */
    }

    else if (bulletPtr->bullet.posY > HEIGHT) {
      bulletPtr->bullet.posY = 0 - bulletPtr->bullet.height; /* Bottom to Top */
    }

    timerCalcTicks(&bulletPtr->bullet.lifeTimer);
    if (bulletPtr->bullet.lifeTimer.ticks > bulletPtr->bullet.lifeTime) {
      bulletDestroy(&bulletPtr->bullet, &player->bullets);
    }

    bulletPtr = bulletPtr->nextBullet;
  }

  timerCalcTicks(&player->bulletTimer);
}

void playerEventHandler(SDL_Event e, Player *player, enum State *gameState) {
  if (e.type == SDL_EVENT_KEY_DOWN & e.key.repeat == 0) {
    switch (e.key.key) {
    case SDLK_W:
      player->moving = true;
      break;

    case SDLK_A:
      player->rotVel -= 1;
      break;

    case SDLK_D:
      player->rotVel += 1;
      break;

    case SDLK_F1:
      playerLogs(*player);
      break;

    case SDLK_J:
      if (player->afterBurnerFuel > 0 && !player->afterBurnerOverheat)
        player->afterburning = true;
      break;

    case SDLK_K:
      player->shooting = true;
      break;

    case SDLK_L:
      if ((!player->afterBurnerOverheat && player->afterBurnerFuel > 0) ||
          player->infFuelTimer.started) {
        player->posX += player->velX * player->leapDistance;
        player->posY += player->velY * player->leapDistance;
        player->afterBurnerOverheat = true;
        player->afterBurnerFuel = 0;
      }
      break;

    case SDLK_ESCAPE:
      *gameState = PAUSED;
      break;

    default:
      break;
    }
  } else if (e.type == SDL_EVENT_KEY_UP & e.key.repeat == 0) {
    switch (e.key.key) {
    case SDLK_W:
      player->moving = false;
      break;

    case SDLK_D:
      player->rotVel -= 1;
      break;

    case SDLK_A:
      player->rotVel += 1;
      break;

    case SDLK_J:
      player->afterburning = false;
      break;

    case SDLK_K:
      player->shooting = false;
      break;

    default:
      break;
    }
  }
}

void playerMovementHandler(Player *player, double delta) {
  /* Rotation */
  if (player->rotVel > 0) /* Holding Right */
    player->rot += player->rotSpeed * delta;
  else if (player->rotVel < 0) /* Holding Left */
    player->rot -= player->rotSpeed * delta;
  if (player->rotVel != 0) /* Calculating velX and velY after rotating */
  {
    /* Calculating velX and velY, also force on both Axes through trigonometry
     */
    player->velX = SDL_sinf(player->rot * PI /
                            180); /* rot * PI / 180 to convert to radians */
    player->velY =
        -SDL_cosf(player->rot * PI /
                  180); /* Due to Y coordinates of SDL, a minus is required */
  }

  /* Screen Looping */
  /* X-Axis */
  if (player->posX > WIDTH) /* Right to left */
    player->posX = 0 - player->width;
  else if (player->posX + player->width < 0) /* Left to Right */
    player->posX = WIDTH;

  /* Y-Axis */
  if (player->posY > HEIGHT) /* Down to Up */
    player->posY = 0 - player->height;
  else if (player->posY + player->height < 0) /* Up to Down */
    player->posY = HEIGHT;

  if (player->moving) {
    if (player->afterburning) /* Afterburner */
      player->moveSpeed = 400;
    else
      player->moveSpeed = 300;

    player->posX += player->moveSpeed * delta * player->velX;
    player->posY += player->moveSpeed * delta * player->velY;

    player->rect.x = player->posX;
    player->rect.y = player->posY;
  }

  /* Afterburner */
  if (!player->afterburning &&
      player->afterBurnerFuel < player->maxAfterBurnerFuel)
    player->afterBurnerFuel += player->afterBurnerRefuelRate;
  else if (player->afterburning && player->afterBurnerFuel > 0 &&
           !player->infFuelTimer.started)
    player->afterBurnerFuel -= player->afterBurnerDepletionRate;
  else if (player->afterBurnerFuel <= 0) {
    player->afterBurnerOverheat = true;
    player->afterburning = false;
  }

  if (player->afterBurnerOverheat &&
      player->afterBurnerFuel >= player->maxAfterBurnerFuel)
    player->afterBurnerOverheat = false;
}

void playerPowerUpHandler(Player *player, PowerUpNode *powerUps,
                          SDL_Texture *spriteSheet, Sprite *spriteList,
                          Mix_Chunk *shieldUpSfx, Mix_Chunk *shieldDownSfx) {
  PowerUpNode *powerPtr = powerUps->nextPowerUp;
  while (powerPtr != NULL) {
    if (SDL_HasRectIntersectionFloat(&player->rect, &powerPtr->power.rect)) {
      if (powerPtr->power.powerUp == SHIELD) {
        player->shield = true;
      } else if (powerPtr->power.powerUp == ARMOR) {
        player->repair = true;
      } else if (powerPtr->power.powerUp == INFFUEL) {
        player->infFuel = true;
      } else if (powerPtr->power.powerUp == MULTIBULLET) {
        player->multiBullet = true;
      }

      powerUpDestroy(&powerPtr->power, powerUps);
    }
    powerPtr = powerPtr->nextPowerUp;
  }

  if (player->shield) {
    Mix_PlayChannel(1, shieldUpSfx, 0);
    timerStart(&player->shieldTimer);
    player->shield = false;
  }
  // When shieldTimer gonna end, start blinking
  if (!player->shieldBlinker.started && player->shieldTimer.started &&
      player->shieldTimer.ticks > player->shieldTime - 1000) {
    timerStart(&player->shieldBlinker);
  }
  if (player->shieldTimer.started && player->shieldBlinker.started &&
      player->shieldBlinker.ticks > 200) {
    player->shieldBlink = !player->shieldBlink;
    timerReset(&player->shieldBlinker);
  }
  if (player->shieldTimer.started &&
      player->shieldTimer.ticks > player->shieldTime) {
    Mix_PlayChannel(1, shieldDownSfx, 0);
    timerStop(&player->shieldTimer);
    timerStop(&player->shieldBlinker);
    player->shieldBlink = false;
  }

  if (player->repair) {
    player->armor++;
    player->repair = false;
  }

  if (player->infFuel) {
    timerStart(&player->infFuelTimer);
    player->infFuel = false;
  }
  if (player->infFuelTimer.started &&
      player->infFuelTimer.ticks > player->infFuelTime) {
    timerStop(&player->infFuelTimer);
  }

  if (player->multiBullet) {
    player->shootDelay = 100;
    timerStart(&player->multiBullTimer);
    player->multiBullet = false;
  }
  if (player->multiBullTimer.started &&
      player->multiBullTimer.ticks > player->multiBullTime) {
    player->shootDelay = 200;
    timerStop(&player->multiBullTimer);
  }

  timerCalcTicks(&player->shieldTimer);
  timerCalcTicks(&player->shieldBlinker);
  timerCalcTicks(&player->infFuelTimer);
  timerCalcTicks(&player->multiBullTimer);
}

void playerTextHandler(Player *player, TTF_TextEngine *gTextEngine,
                       TTF_Font *kenVectorFont) {
  /* Score */
  char *score; /* Number of characters in "Score: 999" */
  SDL_asprintf(&score, "Score: %lu",
               player->score); /* To join string with int */
  player->scoreText = TTF_CreateText(gTextEngine, kenVectorFont, score, 0);

  char *armor;
  SDL_asprintf(&armor, " - %i", player->armor);
  player->armorText = TTF_CreateText(gTextEngine, kenVectorFont, armor, 0);
}

void playerRender(Player *player, SDL_Renderer *gRenderer,
                  SDL_Texture *spriteSheet, Sprite *spriteList) {
  /* Render Stats */
  /* Armor */
  SDL_FRect playerSpriteRect =
      getSpriteRect(spriteList, "playerShip2_blue.png");
  SDL_FRect armorIconRect = {10, 10, 20, 20};
  SDL_RenderTexture(gRenderer, spriteSheet, &playerSpriteRect, &armorIconRect);
  TTF_DrawRendererText(player->armorText, 35, 12.5);

  /* Score */
  TTF_DrawRendererText(player->scoreText,
                       WIDTH - strlen(player->scoreText->text) * 10, 10);

  /* PowerUps */
  SDL_FRect shieldSpriteRect = getSpriteRect(spriteList, "shield3.png");
  SDL_FRect shieldRect = shieldSpriteRect;
  shieldRect.x = player->posX + player->width / 2.f - shieldRect.w / 2.f;
  shieldRect.y = player->posY + player->height / 2.f - shieldRect.h / 2.f;
  SDL_FRect infFuelRect = {60, 20, 10, 10};

  if (player->shieldTimer.started) {
    if (player->shieldBlinker.started && !player->shieldBlink)
      SDL_RenderTextureRotated(gRenderer, spriteSheet, &shieldSpriteRect,
                               &shieldRect, player->rot, NULL, SDL_FLIP_NONE);
    else if (!player->shieldBlinker.started)
      SDL_RenderTextureRotated(gRenderer, spriteSheet, &shieldSpriteRect,
                               &shieldRect, player->rot, NULL, SDL_FLIP_NONE);
  }

  /* Player Render */
  /* Player Icon */
  /* Stiching Two Textures Together */
  if (player->afterburning && player->moving) {
    SDL_FRect fireSpriteRect = getSpriteRect(spriteList, "fire15.png");
    SDL_Texture *fireTexture = SDL_CreateTexture(
        gRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
        player->rect.w, player->rect.h + fireSpriteRect.h);

    SDL_SetRenderTarget(gRenderer, fireTexture);

    SDL_FRect fireRect = {player->width / 2.f - fireSpriteRect.w / 2.f,
                          player->height, fireSpriteRect.w, fireSpriteRect.h};

    SDL_RenderTexture(gRenderer, spriteSheet, &fireSpriteRect, &fireRect);

    SDL_SetRenderTarget(gRenderer, NULL);

    SDL_FRect renderRect = {player->posX - fireSpriteRect.h,
                            player->posY - fireSpriteRect.h,
                            player->width + fireSpriteRect.h * 2,
                            player->height + fireSpriteRect.h * 2};
    SDL_FPoint playerCenter = {};
    SDL_SetRenderDrawColor(gRenderer, 255, 0, 255, 255);
    SDL_RenderTextureRotated(gRenderer, fireTexture, NULL, &renderRect,
                             player->rot, NULL, SDL_FLIP_NONE);
  }
  SDL_RenderTextureRotated(gRenderer, spriteSheet, &playerSpriteRect,
                           &player->rect, player->rot, NULL, SDL_FLIP_NONE);

  SDL_SetRenderDrawColor(gRenderer, 255, 0, 255, 255);
  // SDL_RenderRect(gRenderer, &player.rect);
}

void playerDestroy(Player *player) {
  player->posX = -300;
  player->posY = -300;

  player->width = 0;
  player->height = 0;

  player->moveSpeed = 0;

  player->moving = false;
}
