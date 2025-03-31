#include "asteroid.h"
#include "includes.h"
#include "objects.h"

AsteroidNode asteroidInit(void) {
  AsteroidNode asteroids;
  asteroids.asteroid.astNum = 0;
  asteroids.asteroid.width = 0;
  asteroids.asteroid.height = 0;
  asteroids.asteroid.rect.x = -100;
  asteroids.asteroid.rect.y = -100;
  asteroids.asteroid.rect.w = 0;
  asteroids.asteroid.rect.h = 0;
  asteroids.nextAsteroid = NULL;

  return asteroids;
}

void asteroidSpawn(AsteroidNode *asteroids, Asteroid *refAsteroid,
                   Sprite *spriteList) {
  Asteroid asteroid; /* SDL_rand(Number Of Outcomes) + lowerValue -> lowerValue
                        to NumberOfOutcome - 1*/

  if (refAsteroid == NULL) /* Normal Spawn */
  {
    asteroid.size = (enum Size)SDL_rand(3);
    asteroid.color = SDL_rand(2);
  } else /* Spawn Based On Destroyed Asteroid */
  {
    asteroid.size = refAsteroid->size - 1;
    asteroid.color = refAsteroid->color;
  }
  char *spriteName = NULL;
  char color[2][6] = {"Brown", "Grey"};
  if (asteroid.size == SMALL) {
    SDL_asprintf(&spriteName, "meteor%s_tiny%i.png", color[asteroid.color],
                 SDL_rand(2) + 1);
    asteroid.width = 30;
    asteroid.height = 30;
    asteroid.speed = SDL_rand(100) + 300;
    asteroid.rotVel = SDL_rand(40) - 20;
  } else if (asteroid.size == NORMAL) {
    SDL_asprintf(&spriteName, "meteor%s_med%i.png", color[asteroid.color],
                 SDL_rand(2) + 1);
    asteroid.width = 80;
    asteroid.height = 80;
    asteroid.speed = SDL_rand(100) + 200;
    asteroid.rotVel = SDL_rand(20) - 10;
  } else {
    SDL_asprintf(&spriteName, "meteor%s_big%i.png", color[asteroid.color],
                 SDL_rand(4) + 1);
    asteroid.width = 200;
    asteroid.height = 200;
    asteroid.speed = SDL_rand(100) + 100;
    asteroid.rotVel = SDL_rand(10) - 5;
  }

  asteroid.spriteRect = getSpriteRect(spriteList, spriteName);

  switch (SDL_rand(4)) {
  case 0: /* Up */
    asteroid.posX = SDL_rand(WIDTH);
    asteroid.posY = 0 - asteroid.height;

    asteroid.velX = (SDL_rand(20) - 10) / (float)10; /* -1.0 -> 0.9 */
    if (asteroid.velX >= 0)
      asteroid.velX += 0.1; /* -1.0 -> -0.1 U 0.1 -> 1.0 */
    asteroid.velY = (SDL_rand(10) + 1) / (float)10; /* 0.1 -> 1.0 */
    break;

  case 1: /* Right */
    asteroid.posX = WIDTH;
    asteroid.posY = SDL_rand(HEIGHT);

    asteroid.velX = (SDL_rand(10) - 10) / (float)10; /* -1.0 -> -0.1 */
    asteroid.velY = (SDL_rand(20) - 10) / (float)10; /* -1.0 -> 0.9 */
    if (asteroid.velY >= 0)
      asteroid.velY += 0.1; /* -1.0 -> -0.1 U 0.1 -> 1.0 */
    break;

  case 2: /* Bottom */
    asteroid.posX = SDL_rand(WIDTH);
    asteroid.posY = HEIGHT;

    asteroid.velX = (SDL_rand(20) - 10) / (float)10; /* -1.0 -> 0.9 */
    if (asteroid.velX >= 0)
      asteroid.velX += 0.1; /* -1.0 -> -0.1 U 0.1 -> 1.0 */
    asteroid.velY = (SDL_rand(10) - 10) / (float)10; /* -1.0 -> -0.1 */
    break;

  case 3: /* Left */
    asteroid.posX = 0 - asteroid.width;
    asteroid.posY = SDL_rand(HEIGHT);

    asteroid.velX = (SDL_rand(20) + 1) / (float)10;  /* 0.1 -> 1.0 */
    asteroid.velY = (SDL_rand(20) - 10) / (float)10; /* -1.0 -> 0.9 */
    if (asteroid.velY >= 0)
      asteroid.velY += 0.1; /* -1.0 -> -0.1 U 0.1 -> 1.0 */
    break;

  default:
    break;
  }

  if (refAsteroid != NULL) /* Spawn Around Destroyed Asteroid */
  {
    asteroid.posX = refAsteroid->posX + SDL_rand(refAsteroid->width);
    asteroid.posY = refAsteroid->posY + SDL_rand(refAsteroid->height);
  }

  asteroid.rot = SDL_rand(360);
  asteroid.rect.x = asteroid.posX;
  asteroid.rect.y = asteroid.posY;
  asteroid.rect.w = asteroid.width;
  asteroid.rect.h = asteroid.height;

  AsteroidNode *astPtr, *prevPtr, *newAst;
  astPtr = asteroids->nextAsteroid;
  prevPtr = asteroids;
  newAst = (AsteroidNode *)malloc(sizeof(AsteroidNode));
  uint astNum = 1;

  while (astPtr != NULL) {
    if (astPtr->asteroid.astNum == astNum) {
      astNum++;
      prevPtr = astPtr;
      astPtr = astPtr->nextAsteroid;
    } else {
      break;
    }
  }

  newAst->asteroid = asteroid;
  newAst->asteroid.astNum = astNum;
  newAst->nextAsteroid = astPtr;
  prevPtr->nextAsteroid = newAst;
}

void asteroidDestroy(Asteroid *asteroid, AsteroidNode *asteroids,
                     Sprite *spriteList, Mix_Chunk *astDestroySfx) {
  Mix_PlayChannel(1, astDestroySfx, 0);
  if (asteroid->size != SMALL) /* Spawn Asteroids Of Larger Asteroid */
  {
    uint8 num = SDL_rand(4) + 3; /* Number Of Asteroids to Spawn */
    for (uint8 astNum = 0; astNum < num; astNum++) {
      asteroidSpawn(asteroids, asteroid, spriteList);
    }
  }

  AsteroidNode *astPtr, *prevPtr;
  astPtr = asteroids->nextAsteroid;
  prevPtr = asteroids;
  while (astPtr != NULL) {
    if (astPtr->asteroid.astNum == asteroid->astNum) {
      prevPtr->nextAsteroid = astPtr->nextAsteroid;
      free(astPtr);
      break;
    }
    prevPtr = astPtr;
    astPtr = astPtr->nextAsteroid;
  }
}

void asteroidHandler(AsteroidNode *asteroids, PowerUpNode *powerUps,
                     Player *player, Timer *spawnTimer, int *spawnCount,
                     int spawnTime, double delta, Sprite *spriteList,
                     Mix_Chunk *astDestroySfx) {
  /* Asteroids - Bullet Collision Detector & Destroyer */
  AsteroidNode *astPtr = asteroids->nextAsteroid;
  while (astPtr != NULL) {
    bool destroyed = false;
    BulletNode *bullPtr = player->bullets.nextBullet;
    while (bullPtr != NULL && !destroyed) {
      if (SDL_HasRectIntersectionFloat(&astPtr->asteroid.rect,
                                       &bullPtr->bullet.rect)) {
        int powerChance = SDL_rand(20);
        if (powerChance == 10) {
          powerUpSpawn(&astPtr->asteroid, powerUps);
        }

        asteroidDestroy(&astPtr->asteroid, asteroids, spriteList,
                        astDestroySfx); /* Destroy Objects */
        bulletDestroy(&bullPtr->bullet, &player->bullets);
        destroyed = true;

        player->score++;
      }
      bullPtr = bullPtr->nextBullet;
    }

    if (!destroyed) {
      if (SDL_HasRectIntersectionFloat(&astPtr->asteroid.rect, &player->rect)) {
        asteroidDestroy(&astPtr->asteroid, asteroids, spriteList,
                        astDestroySfx);
        if (!player->shieldTimer.started) {
          player->armor--;
          player->shield = true;
        }
      }
    }

    astPtr = astPtr->nextAsteroid;
  }

  /* Asteroid Spawner */
  if (spawnTimer->ticks > SDL_rand(3000) + spawnTime) {
    asteroidSpawn(asteroids, NULL, spriteList);
    (*spawnCount)++;
    timerReset(spawnTimer); /* Reset Timer */
  }

  /* Asteroid Movement */
  astPtr = asteroids->nextAsteroid;
  while (astPtr != NULL) {
    astPtr->asteroid.posX +=
        astPtr->asteroid.velX * astPtr->asteroid.speed * delta;
    astPtr->asteroid.posY +=
        astPtr->asteroid.velY * astPtr->asteroid.speed * delta;
    astPtr->asteroid.rot += astPtr->asteroid.rotVel;
    if (astPtr->asteroid.rot < 0) {
      astPtr->asteroid.rot = 360 + astPtr->asteroid.rot;
    } else if (astPtr->asteroid.rot > 360) {
      astPtr->asteroid.rot = astPtr->asteroid.rot - 360;
    }

    /* Screen Looping */
    if (astPtr->asteroid.posX + astPtr->asteroid.width < 0) {
      astPtr->asteroid.posX = WIDTH; /* Left to Right */
    }

    else if (astPtr->asteroid.posX > WIDTH) {
      astPtr->asteroid.posX = 0 - astPtr->asteroid.width; /* Right to Left */
    }

    if (astPtr->asteroid.posY + astPtr->asteroid.height < 0) {
      astPtr->asteroid.posY = HEIGHT; /* Top to Bottom */
    }

    else if (astPtr->asteroid.posY > HEIGHT) {
      astPtr->asteroid.posY = 0 - astPtr->asteroid.height; /* Bottom to Top */
    }

    astPtr->asteroid.rect.x = astPtr->asteroid.posX;
    astPtr->asteroid.rect.y = astPtr->asteroid.posY;

    astPtr = astPtr->nextAsteroid;
  }

  timerCalcTicks(spawnTimer);
}
