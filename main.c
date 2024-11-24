#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#define PI 3.14159265359
#define WIDTH 1280
#define HEIGHT 720
#define NULLPTR &NULL
#define ASTLIMIT 100

SDL_Window *gWindow = NULL;
SDL_Renderer *gRenderer = NULL;
SDL_FRect gCamRect = {0, 0, WIDTH, HEIGHT};

/* Timer Definitions */
/* Struct */
typedef struct
{
  Uint32 startTicks;
  Uint32 pausedTicks;

  Uint32 ticks;

  _Bool started;
  _Bool paused;

} Timer;

/* Functions */
void timerStart(Timer *timer)
{
  timer->started = true;
  timer->paused = false;

  timer->startTicks = SDL_GetTicks();
  timer->pausedTicks = 0;
}

void timerStop(Timer *timer)
{
  timer->started = false;
  timer->paused = false;

  timer->startTicks = 0;
  timer->pausedTicks = 0;
}

void timerPause(Timer *timer)
{
  if (timer->started && !timer->paused)
    timer->paused = true;

    timer->pausedTicks = SDL_GetTicks() - timer->startTicks;
    timer->startTicks = 0;
}

void timerCalcTicks(Timer *timer)
{
  if (timer->started)
  {
    if (timer->paused)
    {
      timer->ticks = timer->pausedTicks;
    }
    else
    {
      timer->ticks = SDL_GetTicks() - timer->startTicks;
    }
  }
}

/* Delta Timer Definitions */
/* Struct */
typedef struct
{
  Uint32 frameStart;
  Uint32 frameEnd;

  double delta;
  double deltaInSecs;

} DeltaTimer;

/* Functions */
double deltaCalc(DeltaTimer *dTimer)
{
  dTimer->frameEnd = dTimer->frameStart;
  dTimer->frameStart = SDL_GetPerformanceCounter();

  dTimer->delta = ((dTimer->frameStart - dTimer->frameEnd) * 1000) / (double)SDL_GetPerformanceFrequency();
  dTimer->deltaInSecs = dTimer->delta / 1000;
}

/* Bullet Definitions */
/* Struct */
typedef struct
{
  int width; /* 10 */
  int height; /* 10 */

  float posX;
  float posY;

  int damage; /* 1 */

  float speed; /* 1000 */

  float velX; /* Player.maxVelX */
  float velY; /* Player.maxVelY */

  SDL_FRect rect;

} Bullet;

void bulletDestroy(Bullet *bullet)
{
  bullet->posX = -100;
  bullet->posY = -100;
  bullet->speed = 0;
  bullet->width = 0;
  bullet->height = 0;
  bullet->damage = -1;
}

void bulletsInit(Bullet *bullets, uint8_t bulletCount)
{
  for (uint8_t i = 0; i < bulletCount; i++)
  {
    (bullets+i)->posX = -100;
    (bullets+i)->posY = -100;
    (bullets+i)->speed = 0;
    (bullets+i)->width = 0;
    (bullets+i)->height = 0;
    (bullets+i)->damage = -1;
  }
  
}

/* Asteroid Definitions */
/* Struct */
typedef struct 
{
  short width; /* 20 */
  short height; /* 20 */
  
  float posX;
  float posY;

  _Float16 velX;
  _Float16 velY;

  _Float16 speed;

  short health;

  SDL_FRect rect;
} Asteroid;

/* Functions */
void asteroidInit(Asteroid *asteroids)
{
  for (uint8_t i = 0; i < ASTLIMIT; i++)
  {
    asteroids[i].health = -1;
    asteroids[i].width = 0;
    asteroids[i].height = 0;
    asteroids[i].posX = -200;
    asteroids[i].posY = -200;
    asteroids[i].speed = 0;
  }
}

void asteroidSpawn(Asteroid *asteroids, Timer *spawnTimer, Uint8 *astNum)
{
  Asteroid asteroid; /* SDL_rand(Number Of Outcomes) + lowerValue -> lowerValue to NumberOfOutcome - 1*/
  asteroid.health = SDL_rand(3) + 1; /* 1 - 3 */

  asteroid.width = SDL_rand(30) + 20; /* 20 - 49 */
  asteroid.height = SDL_rand(30) + 20; /* 20 - 49 */

  asteroid.speed = 100;

  switch (SDL_rand(4))
  {
  case 0: /* Up */
    asteroid.posX = SDL_rand(WIDTH);
    asteroid.posY = 0 - asteroid.height;
    
    asteroid.velX = (SDL_rand(20) - 10)/(float) 10; /* -1.0 -> 0.9 */
    if (asteroid.velX >= 0) asteroid.velX += 0.1; /* -1.0 -> -0.1 U 0.1 -> 1.0 */
    asteroid.velY = (SDL_rand(10) + 1)/(float) 10; /* 0.1 -> 1.0 */
    break;
  
  case 1: /* Right */
    asteroid.posX = WIDTH;
    asteroid.posY = SDL_rand(HEIGHT);

    asteroid.velX = (SDL_rand(10) - 10)/(float) 10; /* -1.0 -> -0.1 */
    asteroid.velY = (SDL_rand(20) - 10)/(float) 10; /* -1.0 -> 0.9 */
    if (asteroid.velY >= 0) asteroid.velY += 0.1; /* -1.0 -> -0.1 U 0.1 -> 1.0 */
    break;

  case 2: /* Bottom */
    asteroid.posX = SDL_rand(WIDTH);
    asteroid.posY = HEIGHT;

    asteroid.velX = (SDL_rand(20) - 10)/(float) 10; /* -1.0 -> 0.9 */
    if (asteroid.velX >= 0) asteroid.velX += 0.1; /* -1.0 -> -0.1 U 0.1 -> 1.0 */
    asteroid.velY = (SDL_rand(10) - 10)/(float) 10; /* -1.0 -> -0.1 */
    break;
  
  case 3: /* Left */
    asteroid.posX = 0 - asteroid.width;
    asteroid.posY = SDL_rand(HEIGHT);

    asteroid.velX = (SDL_rand(20) + 1)/(float) 10; /* 0.1 -> 1.0 */
    asteroid.velY = (SDL_rand(20) - 10)/(float) 10; /* -1.0 -> 0.9 */
    if (asteroid.velY >= 0) asteroid.velY += 0.1; /* -1.0 -> -0.1 U 0.1 -> 1.0 */
    break;

  default:
    break;
  }
  
  asteroid.rect.x = asteroid.posX;
  asteroid.rect.y = asteroid.posY;
  asteroid.rect.w = asteroid.width;
  asteroid.rect.h = asteroid.height;

  asteroids[*astNum] = asteroid;
  (*astNum)++;
}

void asteroidDestroy(Asteroid *asteroid)
{
  asteroid->width = 0;
  asteroid->height = 0;
  asteroid->health = -1;
  asteroid->rect.x = -100;
  asteroid->rect.y = -100;
  asteroid->rect.w = 0;
  asteroid->rect.h = 0;
}

void asteroidHandler(Asteroid *asteroids, Bullet *bullets, Timer *spawnTimer, Uint8 *astNum, double delta)
{
  /* Asteroids - Bullet Collision Detector & Destroyer */
  for (uint8_t ast = 0; ast < ASTLIMIT; ast++)
  {
    for (uint8_t bull = 0; bull < 10; bull++)
    {
      if (SDL_HasRectIntersectionFloat(&(asteroids + ast)->rect, &(bullets + bull)->rect))
      {
        (asteroids + ast)->health -= (bullets + bull)->damage; /* Reduce Health */
        if ((asteroids + ast)->health == 0)
        {
          asteroidDestroy(asteroids + ast); /* Destroy Objects */
          bulletDestroy(bullets + bull);
        }
      }
    }
  }

  /* Asteroid Spawner */
  if (spawnTimer->ticks > SDL_rand(5000) + 10000 && (*astNum) < 30)
  {
    if ((*astNum) > 30)
    {
      (*astNum) = 0;
    }

    asteroidSpawn(asteroids, spawnTimer, astNum);
    timerStop(spawnTimer);
    timerStart(spawnTimer);
  }

  /* Asteroid Movement */
  for (uint8_t i = 0; i < ASTLIMIT; i++)
  {
    asteroids[i].posX += asteroids[i].velX * asteroids[i].speed * delta;
    asteroids[i].posY += asteroids[i].velY * asteroids[i].speed * delta;

    /* Screen Looping */
    if (asteroids[i].posX + asteroids[i].width < 0)
    {
      asteroids[i].posX = WIDTH; /* Left to Right */
    }

    else if (asteroids[i].posX > WIDTH)
    {
      asteroids[i].posX = 0 - asteroids[i].width; /* Right to Left */
    }

    if (asteroids[i].posY + asteroids[i].height < 0)
    {
      asteroids[i].posY = HEIGHT; /* Top to Bottom */
    }

    else if (asteroids[i].posY > HEIGHT)
    {
      asteroids[i].posY = 0 - asteroids[i].height; /* Bottom to Top */
    }
    
  

    asteroids[i].rect.x = asteroids[i].posX;
    asteroids[i].rect.y = asteroids[i].posY;
  }

  timerCalcTicks(spawnTimer);
}

/* Player Definitions*/
/* Struct */
typedef struct
{
  short width;  /* 10 */
  short height; /* 10 */

  float posX;
  float posY;
  float rot;

  float maxMoveSpeed; /* 17.5, 30 when afterburning */
  float minMoveSpeed; /* 0.1 */
  float currentMoveSpeed;

  float maxRotSpeed; /* 0.15 */
  float minRotSpeed; /* 0.01 */
  float currentRotSpeed;

  _Bool moving;
  _Bool braking;
  _Bool afterburning;
  float velX;
  float velY;
  float rotVel;

  float moveSpeedMulti; /* 0.09 */   /* Speed added/multiplied when moving */
  float moveSpeedDecay; /* 0.9995, 0.999 when braking */   /* Speed decay when not moving */

  float rotVelMulti; /* 0.05 */ /* rotVelocity added/multiplied when rotating */
  float rotVelDecay; /* 0.999, 0.995 when braking */ /* rotVelocity decay when not rotating */

  float moveVelMulti;  /* 0.09 */ /* Velocity added/multiplied when moving */
  float moveVelDecay;  /* 0.9995, 0.999 when braking */ /* Velocity decay when not moving */

  float minVel; /* 0.01 - Used by both velX and velY*/
  float maxVelX; /* sin of rot */
  float maxVelY; /* cos of rot */

  SDL_FRect rect;

  SDL_Texture *icon;

  Uint8 magSize; /* 10 */
  int bulletNum;

  Bullet bullets[11]; /* magSize + 1 -> endByte*/

  _Bool reloading;
  Uint32 reloadTime;
  Timer bulletTimer;

  Uint64 score;

} Player;

/* Functions */
void playerLogs(Player player)
{
  printf("posX: %f\n", player.posX);
  printf("posY: %f\n", player.posY);
  printf("rot: %f\n", player.rot);
  printf("rotVel: %f\n", player.rotVel);
  printf("velX: %f\n", player.velX);
  printf("maxVelX: %f\n", player.maxVelX);
  printf("velY: %f\n", player.velY);
  printf("currentMoveSpeed: %f\n", player.currentMoveSpeed);
  printf("currentRotSpeed: %f\n", player.currentRotSpeed);
  printf("moving: %i\n", player.moving);
}

void playerShoot(Player *player)
{
  Bullet bullet; /* Creating A Bullet*/

  /* Initialization of bullet */
  bullet.width = 10;
  bullet.height = 10;

  bullet.damage = 1;
  bullet.speed = 1000;

  bullet.velX = player->maxVelX;
  bullet.velY = player->maxVelY;

  bullet.posX = player->posX + player->width/2 - bullet.width/2; /* Center of X */
  bullet.posY = player->posY + player->height/2 - bullet.height/2; /* Center of Y */

  bullet.rect.w = bullet.width;
  bullet.rect.h = bullet.height;
  bullet.rect.x = bullet.posX;
  bullet.rect.y = bullet.posY;

  player->bullets[player->bulletNum] = bullet;
  player->bulletNum++;
}

void playerBulletHander(Player *player, double delta)
{
  if (player->bulletNum > player->magSize - 2 && !player->reloading)
  {
    player->reloading = true;
    timerStart(&player->bulletTimer);
  }

  if (player->bulletTimer.ticks > player->reloadTime && player->reloading)
  {
    timerStop(&player->bulletTimer);
    player->reloading = false;
    player->bulletNum = 0;
  }
  
  for (int i = 0; i < player->magSize - 1; i++)
  {
    /* Moving the bullets */
    player->bullets[i].posX += player->bullets[i].velX * player->bullets[i].speed * delta;
    player->bullets[i].posY += player->bullets[i].velY * player->bullets[i].speed * delta;
    
    player->bullets[i].rect.x = player->bullets[i].posX;
    player->bullets[i].rect.y = player->bullets[i].posY;
    
    if (!SDL_HasRectIntersectionFloat(&player->bullets[i].rect, &gCamRect) && player->bullets[i].damage != -1)
    {
      bulletDestroy(&player->bullets[i]);
    }
  }

  timerCalcTicks(&player->bulletTimer);
}

void playerEventHandler(SDL_Event e, Player *player)
{
  if (e.type == SDL_EVENT_KEY_DOWN & e.key.repeat == 0)
  {
    switch (e.key.key)
    {
    case SDLK_W:
      player->moving = true;
      break;

    case SDLK_S:
      player->braking = true;
      break;

    case SDLK_A:
      player->rotVel -= 1;
      break;

    case SDLK_D:
      player->rotVel += 1;
      break;

    case SDLK_L:
      playerLogs(*player);
      break;

    case SDLK_LSHIFT:
      player->afterburning = true;
      break;

    case SDLK_SPACE:
      if (!player->reloading)
      {
        playerShoot(player);
      }
      break;

    default:
      break;
    }
  }
  else if (e.type == SDL_EVENT_KEY_UP & e.key.repeat == 0)
  {
    switch (e.key.key)
    {
    case SDLK_W:
      player->moving = false;
      break;

    case SDLK_S:
      player->braking = false;
      break;

    case SDLK_D:
      player->rotVel -= 1;
      break;

    case SDLK_A:
      player->rotVel += 1;
      break;

    case SDLK_LSHIFT:
      player->afterburning = false;

    default:
      break;
    }
  }
}

void playerMovementHandler(Player *player, double delta)
{
  /* currentRotSpeed calculation */
  if (player->rotVel > 0) /* Holding Right */
  {
    if (player->currentRotSpeed != 0) /* Not At Rest */
    {
      if (player->currentRotSpeed + player->currentRotSpeed * player->rotVelMulti <= player->maxRotSpeed & player->currentRotSpeed > 0)
      {
        player->currentRotSpeed += player->currentRotSpeed * player->rotVelMulti; /* Adding rotSpeed */
      }
      else if (player->currentRotSpeed < 0)
      {
        player->currentRotSpeed -= player->currentRotSpeed * player->rotVelMulti; /* Subtracting rotSpeed */
      }
      else if (player->currentRotSpeed + player->currentRotSpeed * player->rotVelMulti >= player->maxRotSpeed)
      {
        player->currentRotSpeed = player->maxRotSpeed; /* Limiting To maxRotSpeed */
      }

      if (player->currentRotSpeed < player->minRotSpeed & player->currentRotSpeed > -player->minRotSpeed)
      {
        player->currentRotSpeed = 0; /* If small enough, make it 0 */
      }

    }
    else
    {
      player->currentRotSpeed = player->minRotSpeed; /* At Rest */
    }
  }
  else if (player->rotVel < 0) /* Holding Left */
  {
    if (player->currentRotSpeed != 0) /* Not At Rest */
    {
      if (player->currentRotSpeed - player->currentRotSpeed * player->rotVelMulti >= -(player->maxRotSpeed) & player->currentRotSpeed < 0)
      {
        player->currentRotSpeed += player->currentRotSpeed * player->rotVelMulti;
      }
      else if (player->currentRotSpeed - player->currentRotSpeed * player->rotVelMulti <= -(player->maxRotSpeed))
      {
        player->currentRotSpeed = -(player->maxRotSpeed);
      }
      else if (player->currentRotSpeed > 0)
      {
        player->currentRotSpeed -= player->currentRotSpeed * player->rotVelMulti;
      }

      if (player->currentRotSpeed < player->minRotSpeed & player->currentRotSpeed > -player->minRotSpeed)
      {
        player->currentRotSpeed = 0; /* If small enough, make it 0 */
      }
    }
    else
    {
      player->currentRotSpeed = -player->minRotSpeed; /* At rest */
    }
  }
  else if (player->rotVel == 0 && player->currentRotSpeed != 0) /* Totally At Rest */
  {
    player->currentRotSpeed *= player->rotVelDecay; /* Reduce rotVel */

    if (player->currentRotSpeed < player->minRotSpeed & player->currentRotSpeed > -player->minRotSpeed)
    {
      player->currentRotSpeed = 0; /* If small enough, make it 0 (works better than i expected lol)*/
    }
  }

  /* Rotating the ship */
  if (player->rot + player->currentRotSpeed >= 360) /* Cycle from 360 to 0 */
  {
    player->rot = player->rot + player->currentRotSpeed - 360;
  }
  else
  {
    player->rot += player->currentRotSpeed * ( delta * 1000 ); /* delta * 1000 -> Normalization */
  }

  if (player->rot + player->currentRotSpeed <= 0) /* Cycle from 0 to 360 */
  {
    player->rot = 360 + player->rot + player->currentRotSpeed;
  }
  else
  {
    player->rot += player->currentRotSpeed * ( delta * 1000 );
  }

  

  /* Calculating currentMoveSpeed */
  if (player->moving)
  {
    if (player->currentMoveSpeed != 0)
    {
      if (player->currentMoveSpeed + player->currentMoveSpeed * player->moveSpeedMulti < player->maxMoveSpeed)
      {
        player->currentMoveSpeed += player->currentMoveSpeed * player->moveSpeedMulti;
      }
      else if (player->currentMoveSpeed + player->currentMoveSpeed * player->moveSpeedMulti >= player->maxMoveSpeed)
      {
        player->currentMoveSpeed = player->maxMoveSpeed;
      }
    }
    else
    {
      player->currentMoveSpeed = player->minMoveSpeed;
    }
  }
  else
  {
    if (player->currentMoveSpeed >= player->minMoveSpeed)
    {
      player->currentMoveSpeed *= player->moveSpeedDecay;
    }
    else
    {
      player->currentMoveSpeed = 0;
    }
  }

  /* Calculating max of velX and velY, also force on both Axes through trigonometry */
  player->maxVelX = SDL_sinf(player->rot * PI / 180); /* rot * PI / 180 to convert to radians */
  player->maxVelY = -SDL_cosf(player->rot * PI / 180); /* Due to Y coordinates of SDL, a minus is required */

  /* This entire structure is from the rotVel update section. Refer there if you have doubts */
  if (player->moving) /* Update velX and velY only when moving */
  {
    if (player->maxVelX > 0) /* if maxVelX is +ve */
    {
      if (player->velX != 0)
      {
        if (player->velX + player->velX * player->moveVelMulti <= player->maxVelX & player->velX > 0)
        {
          player->velX += player->velX * player->moveVelMulti;
        }
        else if (player->velX + player->velX * player->moveVelMulti > player->maxVelX)
        {
          player->velX = player->maxVelX;
        }
        else if (player->velX < 0)
        {
          player->velX -= player->velX * player->moveVelMulti;
        }
        if (player->velX < player->minVel & player->velX > -player->minVel)
        {
          player->velX = 0;
        }
      }
      else
      {
        player->velX = player->minVel;
      }
    }
    else if (player->maxVelX < 0) /* if maxVelX is -ve */
    {
      if (player->velX != 0)
      {
        if (player->velX + player->velX * player->moveVelMulti >= player->maxVelX & player->velX < 0)
        {
          player->velX += player->velX * player->moveVelMulti;
        }
        else if (player->velX + player->velX * player->moveVelMulti < player->maxVelX)
        {
          player->velX = player->maxVelX;
        }
        else if (player->velX > 0)
        {
          player->velX -= player->velX * player->moveVelMulti;
        }
        if (player->velX < player->minVel & player->velX > -player->minVel)
        {
          player->velX = 0;
        } 
      }
      else
      {
        player->velX = -player->minVel;
      }
    }

    if (player->maxVelY > 0) /* if maxVelY is +ve */
    {
      if (player->velY != 0)
      {
        if (player->velY + player->velY * player->moveVelMulti <= player->maxVelY & player->velY > 0)
        {
          player->velY += player->velY * player->moveVelMulti;
        }
        else if (player->velY + player->velY * player->moveVelMulti > player->maxVelY)
        {
          player->velY = player->maxVelY;
        }
        else if (player->velY < 0)
        {
          player->velY -= player->velY * player->moveVelMulti;
        }
        if (player->velY < player->minVel & player->velY > -player->minVel)
        {
          player->velY = 0;
        }
      }
      else
      {
        player->velY = player->minVel;
      }
    }
    else if (player->maxVelY < 0) /* if maxVelY is -ve */
    {
      if (player->velY != 0)
      {
        if (player->velY + player->velY * player->moveVelMulti >= player->maxVelY & player->velY < 0)
        {
          player->velY += player->velY * player->moveVelMulti;
        }
        else if (player->velY + player->velY * player->moveVelMulti < player->maxVelY)
        {
          player->velY = player->maxVelY;
        }
        else if (player->velY > 0)
        {
          player->velY -= player->velY * player->moveVelMulti;
        }
        if (player->velY < player->minVel & player->velY > -player->minVel)
        {
          player->velY = 0;
        } 
      }
      else
      {
        player->velY = -player->minVel;
      }
    }

    
  }
  else
  {
    if (player->velX != 0)
    {
      player->velX *= player->moveVelDecay;
    }
    if (player->velX > -delta & player->velX < delta)
    {
      player->velX = 0;
    }
    

    if (player->velY != 0)
    {
      player->velY *= player->moveVelDecay;
    }
    if (player->velY > -delta & player->velY < delta)
    {
      player->velY = 0;
    }
  }
  
  if (player->braking) /* Brakes */
  {
    player->moveSpeedDecay = 0.999;
    player->moveVelDecay = 0.999;
    player->rotVelDecay = 0.995;
  }
  else
  {
    player->moveSpeedDecay = 0.9995;
    player->moveVelDecay = 0.9995;
    player->rotVelDecay = 0.999;
  }

  if (player->afterburning) /* Afterburner */
  {
    player->maxMoveSpeed = 30;
  }
  else
  {
    player->maxMoveSpeed = 17.5;
  }
  
  /* Screen Looping */
  /* X-Axis */
  if (player->posX > WIDTH) /* Right to left */
  {
    player->posX = 0 - player->width;
  }
  else if (player->posX + player->width < 0) /* Left to Right */
  {
    player->posX = WIDTH;
  }

  /* Y-Axis */
  if (player->posY > HEIGHT) /* Down to Up */
  {
    player->posY = 0 - player->height;
  }
  else if (player->posY + player->height < 0) /* Up to Down */
  {
    player->posY = HEIGHT;
  }

  player->posX += player->currentMoveSpeed * (delta * 10) * player->velX;
  player->posY += player->currentMoveSpeed * (delta * 10) * player->velY;

  player->rect.x = player->posX;
  player->rect.y = player->posY;

  /*Test*/
  
}

void renderPlayer(Player player)
{
  SDL_FRect destRect;
  destRect.x = player.rect.x;
  destRect.y = player.rect.y;
  destRect.w = 50;
  destRect.h = 50;
  SDL_RenderTextureRotated(gRenderer, player.icon, NULL, &destRect, player.rot, NULL, SDL_FLIP_NONE);
}

void playerDestroy(Player *player)
{
  
}

bool init()
{
  bool success = true;


  /* Initializing SDL3 */
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    printf("SDL could not be initialized! SDL Error: %s\n", SDL_GetError());
    success = false;
  }
  else
  {
    if (IMG_Init(IMG_INIT_PNG - IMG_INIT_JPG) == 0)
    {
      printf("SDL_Image could not be initialized! SDL_Image Error %s\n", IMG_GetError());
      success = false;
    }
    else
    {
      gWindow = SDL_CreateWindow("Astroids-P1", WIDTH, HEIGHT, SDL_WINDOW_FULLSCREEN);
      if (gWindow == NULL)
      {
        printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
        success = false;
      }
      else
      {
        gRenderer = SDL_CreateRenderer(gWindow, NULL);
        if (gRenderer == NULL)
        {
          printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
          success = false;
        }
        else
        {
          SDL_SetRenderDrawColor(gRenderer, 0x22, 0x22, 0x11, 0xFF);
        }
      }
    }
  }

  return success;
}

bool load(Player *player) /* Get Game Objects and load their respective assets */
{
  bool success = true;

  player->icon = SDL_CreateTextureFromSurface(gRenderer, IMG_Load("Crystal.png"));
  if (player->icon == NULL)
  {
    printf("'Crystal.png' could not be loaded! SDL_image Error: %s\n", IMG_GetError());
  }

  return success;
}

void draw(Player player, Asteroid asteroids[])
{
  SDL_SetRenderDrawColor(gRenderer, 0x22, 0x22, 0x11, 0xFF);
  SDL_RenderClear(gRenderer);

  SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
  //SDL_RenderFillRect(gRenderer, &player.rect);
  
  for (uint8_t i = 0; i < player.magSize - 1; i++)
  {
    if (!SDL_HasRectIntersectionFloat(&player.rect, &player.bullets[i].rect) && player.bullets[i].damage != -1)
    {
      SDL_RenderRect(gRenderer, &player.bullets[i].rect); /* Render Bullets only when they're a bit away from you*/
    }
  }

  for (uint8_t i = 0; i < ASTLIMIT; i++) /* Render Asteroids */
  {
    if (asteroids[i].health != -1)
    {
    SDL_RenderFillRect(gRenderer, &asteroids[i].rect);
    }
  }


  renderPlayer(player);

  SDL_RenderPresent(gRenderer);
}

void quit()
{
  SDL_DestroyWindow(gWindow);
  SDL_DestroyRenderer(gRenderer);
  SDL_Quit();
}

int main(int argc, char *args[])
{
  init(); /* Initialize */

  /* Initializing Game Objects */
  /* Delta Timer */
  DeltaTimer dTimer;

  dTimer.frameStart = SDL_GetPerformanceCounter();
  dTimer.frameEnd = 0;
  dTimer.delta = 0;

  /* Player */
  Player player;

  player.width = 50;
  player.height = 50;

  player.posX = 10;
  player.posY = 10;
  player.rot = 90;

  player.maxMoveSpeed = 17.5;
  player.minMoveSpeed = 0.1;
  player.maxRotSpeed = 0.15;
  player.minRotSpeed = 0.01;

  player.currentMoveSpeed = 0;
  player.currentRotSpeed = 0;
  player.rotVel = 0;
  player.moving = false;
  player.braking = false;
  player.afterburning = false;

  player.moveSpeedMulti = 0.009;
  player.moveSpeedDecay = 0.9995;

  player.moveVelMulti = 0.009;
  player.moveVelDecay = 0.9995;

  player.rotVelMulti = 0.005;
  player.rotVelDecay = 0.999;

  player.velX = 0;
  player.velY = 0;

  player.minVel = 0.1;
  player.maxVelX = 0;
  player.maxVelY = 0;

  player.rect.w = player.width;
  player.rect.h = player.height;
  player.rect.x = player.posX;
  player.rect.y = player.posY;

  /* Player Bullets */
  player.magSize = 11;
  player.bulletNum = 0;

  player.bulletTimer.startTicks = 0;
  player.bulletTimer.pausedTicks = 0;
  player.bulletTimer.started = 0;
  player.bulletTimer.paused = 0;
  player.bulletTimer.ticks = 0;

  player.reloadTime = 5000;

  player.icon = NULL;

  bulletsInit(player.bullets, player.magSize - 1);

  /* Asteroids */
  Asteroid asteroids[ASTLIMIT + 1];

  Uint8 astNum = 0;
  asteroidInit(asteroids);

  Timer astSpawnTimer;
  timerStart(&astSpawnTimer);

  load(&player);

  SDL_Event e;
  bool exited = false;

  while (!exited)
  {
    while (SDL_PollEvent(&e) != 0)
    {
      if (e.type == SDL_EVENT_QUIT)
      {
        exited = true;
      }

      playerEventHandler(e, &player);
    }
    deltaCalc(&dTimer);

    playerMovementHandler(&player, dTimer.deltaInSecs);
    playerBulletHander(&player, dTimer.deltaInSecs);
    asteroidHandler(asteroids, player.bullets, &astSpawnTimer, &astNum, dTimer.deltaInSecs);

    draw(player, asteroids); /* Draw, Blit and Render */
  }

  quit(); // Quit
  return 0;
}