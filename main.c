#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_ttf/SDL_textengine.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_net/SDL_net.h>
#include <SDL3_rtf/SDL_rtf.h>

#define uint8 unsigned char
#define uint16 unsigned short
#define uint32 unsigned int
#define uint64 unsigned long

#define PRINTERROR printf("%s", SDL_GetError());

#define PI 3.14159265359
#define WIDTH 1280
#define HEIGHT 720

enum State { /* Possible Game States */
  MENU = 0,
  GAME = 1,
  PAUSED = 2,
  OVER = 3,
  SCORES = 4
};

enum Power { /* Possible PowerUps */
  SHIELD = 0,
  HEALTH = 1,
  INFFUEL = 2,
  INFBULL = 3
};


enum State gameState = MENU;

SDL_Window *gWindow = NULL;
SDL_Renderer *gRenderer = NULL;
SDL_FRect gCamRect = {0, 0, WIDTH, HEIGHT};

TTF_Font *jetBrainsMono;
TTF_TextEngine *gTextEngine;

/* Timer Definitions */
/* Struct */
typedef struct Timer
{
  uint32 startTicks;
  uint32 pausedTicks;

  uint32 ticks;

  _Bool started;
  _Bool paused;

} Timer;

/* Functions */
Timer timerInit()
{
  Timer timer;

  timer.startTicks = 0;
  timer.pausedTicks = 0;
  timer.started = false;
  timer.paused = false;
  timer.ticks = 0;

  return timer;
}

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

  timer->ticks = 0;
  timer->startTicks = 0;
  timer->pausedTicks = 0;
}

void timerPause(Timer *timer)
{
  if (timer->started && !timer->paused)
  {
    timer->paused = true;

    timer->pausedTicks = SDL_GetTicks() - timer->startTicks;
    timer->startTicks = 0;
  }
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
typedef struct DeltaTimer
{
  uint32 frameStart;
  uint32 frameEnd;

  double delta;
  double deltaInSecs;

} DeltaTimer;

/* Functions */
void deltaCalc(DeltaTimer *dTimer)
{
  dTimer->frameEnd = dTimer->frameStart;
  dTimer->frameStart = SDL_GetPerformanceCounter();

  dTimer->delta = ((dTimer->frameStart - dTimer->frameEnd) * 1000) / (double)SDL_GetPerformanceFrequency();
  dTimer->deltaInSecs = dTimer->delta / 1000;
}

/* Bullet Definitions */
/* Struct */
typedef struct Bullet
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
  uint32 bulletNum;

} Bullet;

/* Linked List */
typedef struct BulletList
{
  Bullet bullet;
  struct BulletList *nextBullet;

} BulletList;

void bulletDestroy(Bullet *bullet, struct BulletList *bullets)
{
  BulletList *bulletPtr, *prevPtr;
  prevPtr = bullets;
  bulletPtr = bullets->nextBullet;
  while (bulletPtr != NULL)
  {
    if (bulletPtr->bullet.bulletNum == bullet->bulletNum)
    {
      prevPtr->nextBullet = bulletPtr->nextBullet;
      free(bulletPtr);
      break;
    }
    prevPtr = bulletPtr;
    bulletPtr = bulletPtr->nextBullet;
  }
}

/* PowerUp Definitions */
/* Struct */
typedef struct PowerUp
{
  float width;
  float height;
  float posX;
  float posY;

  uint32 powerNum;

  SDL_FRect rect;

  enum Power powerUp;
} PowerUp;

typedef struct PowerUpList {

  PowerUp power;
  struct PowerUpList *nextPowerUp;
  
} PowerUpList;

/* Functions */
struct PowerUpList powerUpInit()
{
  PowerUpList powerUps;

  powerUps.power.width = 0;
  powerUps.power.height = 0;
  powerUps.power.posX = -200;
  powerUps.power.posY = -200;
  powerUps.power.powerNum = 0;
  powerUps.power.rect.w = powerUps.power.width;
  powerUps.power.rect.h = powerUps.power.height;
  powerUps.power.rect.x = powerUps.power.posX;
  powerUps.power.rect.y = powerUps.power.posY;

  powerUps.nextPowerUp = NULL;
  
  return powerUps;
}

void powerUpDestroy(PowerUp *powerUp, PowerUpList *powerUps)
{
  PowerUpList *powerPtr, *prevPtr;
  powerPtr = powerUps->nextPowerUp;
  prevPtr = powerUps;

  while (powerPtr != NULL)
  {
    if (powerUp->powerNum == powerPtr->power.powerNum)
    {
      prevPtr->nextPowerUp = powerPtr->nextPowerUp;
      free(powerPtr);
      break;
    }
    prevPtr = powerPtr;
    powerPtr = powerPtr->nextPowerUp;
  }
}

/* Player Definitions*/
/* Struct */
typedef struct Player
{
  short health; /* 3 */

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

  float maxAfterBurnerFuel; /* 100 */
  float afterBurnerRefuelRate; /* 5 */
  float afterBurnerDepletionRate; /* 10 */
  float afterBurnerFuel;
  _Bool afterBurnerOverheat;
  Timer overHeatTimer;
  int afterBurnerCooldown; /* 5 Seconds */

  float leapDistance; /* 100 */

  SDL_FRect rect;

  SDL_Texture *icon;

  uint8 magSize; /* 10 */
  uint32 bulletNum;

  BulletList bullets;

  _Bool reloading;
  uint32 reloadTime;
  Timer bulletTimer;

  uint64 score;
  TTF_Text *scoreText;

  bool shield;
  bool heal;
  bool infBulls;
  bool infFuel;

  Timer shieldTimer;
  uint32 shieldTime;
  Timer infBullsTimer;
  uint32 infBullsTime;
  Timer infFuelTimer;
  uint32 infFuelTime;

} Player;

/* Functions */
void playerInit(Player *player)
{
  player->score = 0;

  player->width = 50;
  player->height = 50;

  player->posX = 10;
  player->posY = 10;
  player->rot = 90;

  player->maxMoveSpeed = 17.5;
  player->minMoveSpeed = 0.1;
  player->maxRotSpeed = 0.1;
  player->minRotSpeed = 0.01;

  player->currentMoveSpeed = 0;
  player->currentRotSpeed = 0;
  player->rotVel = 0;
  player->moving = false;
  player->braking = false;
  player->afterburning = false;

  player->moveSpeedMulti = 0.009;
  player->moveSpeedDecay = 0.9995;

  player->moveVelMulti = 0.009;
  player->moveVelDecay = 0.9995;

  player->rotVelMulti = 0.005;
  player->rotVelDecay = 0.999;

  player->velX = 0;
  player->velY = 0;

  player->minVel = 0.1;
  player->maxVelX = 0;
  player->maxVelY = 0;

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
  player->magSize = 11;
  player->bulletNum = 0;

  Timer bulletTimer = timerInit();

  player->reloadTime = 5000;
  player->reloading = false;

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
  player->shield = false;
  player->shieldTime = 5000;
  player->shieldTimer = timerInit();
  player->heal = false;
  player->infBulls = false;
  player->infBullsTime = 5000;
  player->infBullsTimer = timerInit();
  player->infFuel = false;
  player->infFuelTime = 5000;
  player->infFuelTimer = timerInit();
}

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

  /* Linked List For Optimised Allocation */
  BulletList *bulletPtr, *prevPtr, *newBullet;
  uint bulletNum = 1;
  prevPtr = &player->bullets;
  bulletPtr = player->bullets.nextBullet;
  newBullet = (BulletList*) malloc(sizeof(BulletList));

  while (bulletPtr != NULL)
  {
    if (bulletNum == bulletPtr->bullet.bulletNum)
    {
      prevPtr = bulletPtr;
      bulletPtr = bulletPtr->nextBullet;
      bulletNum++;
    }
    else
    {
      break;
    }
    
    
  }
  
  newBullet->bullet = bullet;
  newBullet->bullet.bulletNum = bulletNum;
  newBullet->nextBullet = bulletPtr;
  prevPtr->nextBullet = newBullet;
  
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
  
  BulletList *bulletPtr = &player->bullets;
  while(bulletPtr != NULL)
  {
    /* Moving the bullets */
    bulletPtr->bullet.posX += bulletPtr->bullet.velX * bulletPtr->bullet.speed * delta;
    bulletPtr->bullet.posY += bulletPtr->bullet.velY * bulletPtr->bullet.speed * delta;
    
    bulletPtr->bullet.rect.x = bulletPtr->bullet.posX;
    bulletPtr->bullet.rect.y = bulletPtr->bullet.posY;
    
    if (!SDL_HasRectIntersectionFloat(&bulletPtr->bullet.rect, &gCamRect) && bulletPtr->bullet.damage != -1)
    {
      bulletDestroy(&bulletPtr->bullet, &player->bullets);
    }

    bulletPtr = bulletPtr->nextBullet;
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

    case SDLK_F1:
      playerLogs(*player);
      break;

    case SDLK_J:
      if (player->afterBurnerFuel > 0 && !player->afterBurnerOverheat)
      {
        player->afterburning = true;
      }
      break;

    case SDLK_K:
      if (!player->reloading || player->infBullsTimer.started)
      {
        playerShoot(player);
      }
      break;
    
    case SDLK_L:
      if (!player->afterBurnerOverheat && player->afterBurnerFuel >= player->maxAfterBurnerFuel && !player->infFuelTimer.started);
      {
        player->posX += player->maxVelX * player->leapDistance;
        player->posY += player->maxVelY * player->leapDistance;
        player->afterBurnerOverheat = true;
        player->afterBurnerFuel = 0;
      }
      break;

    case SDLK_ESCAPE:
      gameState = PAUSED;
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

    case SDLK_J:
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

  /* Afterburner */
  if (!player->afterburning && player->afterBurnerFuel < player->maxAfterBurnerFuel)
  {
    player->afterBurnerFuel += player->afterBurnerRefuelRate;
  }
  else if (player->afterburning && player->afterBurnerFuel > 0 && !player->infFuelTimer.started)
  {
    player->afterBurnerFuel -= player->afterBurnerDepletionRate;
  }
  else if (player->afterBurnerFuel <= 0)
  {
    player->afterBurnerOverheat = true;
    player->afterburning = false;
  }
  
  if (player->afterBurnerOverheat && player->afterBurnerFuel >= player->maxAfterBurnerFuel)
  {
    player->afterBurnerOverheat = false;
  }
}

void playerPowerUpHandler(Player *player, PowerUpList *powerUps)
{
  PowerUpList *powerPtr = powerUps->nextPowerUp;
  while (powerPtr != NULL)
  {
    if (SDL_HasRectIntersectionFloat(&player->rect, &powerPtr->power.rect))
    {
      if (powerPtr->power.powerUp == SHIELD)
      {
        player->shield = true;
      }
      else if (powerPtr->power.powerUp == HEALTH)
      {
        player->heal = true;
      }
      else if (powerPtr->power.powerUp == INFBULL)
      {
        player->infBulls = true;
      }
      else if (powerPtr->power.powerUp == INFFUEL)
      {
        player->infFuel = true;
      }
      
      powerUpDestroy(&powerPtr->power, powerUps);
    }
    powerPtr = powerPtr->nextPowerUp;
  }
  


  if (player->shield) 
  {
    timerStart(&player->shieldTimer);
    player->shield = false;
  }
  if (player->shieldTimer.started && player->shieldTimer.ticks > player->shieldTime)
  {
    timerStop(&player->shieldTimer);
  }

  if (player->heal)
  {
    player->health++;
    player->heal = false;
  }

  if (player->infBulls)
  {
    timerStart(&player->infBullsTimer);
    player->infBulls = false;
  }
  if (player->infBullsTimer.started && player->infBullsTimer.ticks > player->infBullsTime)
  {
    timerStop(&player->infBullsTimer);
  }

  if (player->infFuel) 
  {
    timerStart(&player->infFuelTimer);
    player->infFuel = false;
  }
  if (player->infFuelTimer.started && player->infFuelTimer.ticks > player->infFuelTime) 
  {
    timerStop(&player->infFuelTimer); 
  }

  timerCalcTicks(&player->shieldTimer);
  timerCalcTicks(&player->infBullsTimer);
  timerCalcTicks(&player->infFuelTimer);
}

void playerTextHandler(Player *player)
{
  /* Score */
  char score[11]; /* Number of characters in "Score: 999" */
  snprintf(score, 11, "Score: %u", player->score); /* To join string with int */
  player->scoreText = TTF_CreateText(gTextEngine, jetBrainsMono, score, 0);
}

void playerRender(Player player)
{
  /* Player Render */
  SDL_FRect destRect; /* Creating destination rect to render player*/
  destRect.x = player.rect.x;
  destRect.y = player.rect.y;
  destRect.w = 50;
  destRect.h = 50;
  SDL_RenderTextureRotated(gRenderer, player.icon, NULL, &destRect, player.rot, NULL, SDL_FLIP_NONE);

  /* Render Stats */
  /* Score */
  TTF_DrawRendererText(player.scoreText, WIDTH - strlen(player.scoreText->text) * 10, 10);

  /* PowerUps */
  SDL_FRect ShieldRect = {20, 20, 10, 10};
  SDL_FRect InfBulletsRect = {40, 20, 10, 10};
  SDL_FRect InfFuelRect = {60, 20, 10, 10};

  if (player.shieldTimer.started)
  {
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 255, 255);
    SDL_RenderFillRect(gRenderer, &ShieldRect);
  }
  if (player.infBullsTimer.started)
  {
    SDL_SetRenderDrawColor(gRenderer, 255, 255, 0, 255);
    SDL_RenderFillRect(gRenderer, &InfBulletsRect);
  }
  if (player.infFuelTimer.started)
  {
    SDL_SetRenderDrawColor(gRenderer, 255, 0, 255, 255);
    SDL_RenderFillRect(gRenderer, &InfFuelRect);
  }
  
}

void playerDestroy(Player *player)
{
  player->posX = -300;
  player->posY = -300;

  player->width = 0;
  player->height = 0;

  player->minMoveSpeed = 0;
  player->maxMoveSpeed = 0;

  player->moving = false;
}

/* Asteroid Definitions */
/* Struct */
typedef struct Asteroid
{
  short width; /* 20 */
  short height; /* 20 */
  
  float posX;
  float posY;

  _Float16 velX;
  _Float16 velY;

  _Float16 speed;

  short health;
  uint32 astNum;

  SDL_FRect rect;
} Asteroid;

typedef struct AsteroidList
{
  Asteroid asteroid;
  struct AsteroidList *nextAsteroid;
} AsteroidList;

/* PowerUp Functions */

void powerUpSpawn(Asteroid *asteroid, PowerUpList *powerUps)
{
  int powerUp = SDL_rand(4);
  PowerUp power;
  power.width = 16;
  power.height = 16;
  power.posX = asteroid->posX + asteroid->width/2;
  power.posY = asteroid->posY + asteroid->height/2;
  power.rect.w = power.width;
  power.rect.h = power.height;
  power.rect.x = power.posX;
  power.rect.y = power.posY;
  power.powerUp = (enum Power) powerUp;

  PowerUpList *powerPtr, *prevPtr, *newPower;
  powerPtr = powerUps->nextPowerUp;
  prevPtr = powerUps;
  newPower = (PowerUpList*) malloc(sizeof(PowerUpList));
  uint32 powerNum = 1;

  while (powerPtr != NULL)
  {
    if (powerPtr->power.powerNum == powerNum)
    {
      prevPtr = powerPtr;
      powerPtr = powerPtr->nextPowerUp;
      powerNum++;
    }
    else
    {
      break;
    }
  }

  newPower->power = power;
  newPower->power.powerNum = powerNum;
  newPower->nextPowerUp = powerPtr;
  prevPtr->nextPowerUp = newPower;
}

/* Functions */
AsteroidList asteroidInit()
{
  AsteroidList asteroids;
  asteroids.asteroid.astNum = 0;
  asteroids.asteroid.width = 0;
  asteroids.asteroid.height = 0;
  asteroids.asteroid.health = -1;
  asteroids.asteroid.rect.x = -100;
  asteroids.asteroid.rect.y = -100;
  asteroids.asteroid.rect.w = 0;
  asteroids.asteroid.rect.h = 0;
  asteroids.nextAsteroid = NULL;

  return asteroids;
}

void asteroidSpawn(AsteroidList *asteroids)
{
  Asteroid asteroid; /* SDL_rand(Number Of Outcomes) + lowerValue -> lowerValue to NumberOfOutcome - 1*/
  asteroid.health = SDL_rand(3) + 1; /* 1 - 3 */

  asteroid.width = SDL_rand(30) + 40; /* 40 - 69 */
  asteroid.height = SDL_rand(30) + 40; /* 40 - 69 */

  asteroid.speed = SDL_rand(100) + 100;

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

  AsteroidList *astPtr, *prevPtr, *newAst;
  astPtr = asteroids->nextAsteroid;
  prevPtr = asteroids;
  newAst = (AsteroidList*) malloc(sizeof(AsteroidList));
  uint astNum = 1;

  while (astPtr != NULL)
  {
    if (astPtr->asteroid.astNum == astNum)
    {
      astNum++;
      prevPtr = astPtr;
      astPtr = astPtr->nextAsteroid;
    }
    else
    {
      break;
    }
  }

  newAst->asteroid = asteroid;
  newAst->asteroid.astNum = astNum;
  newAst->nextAsteroid = astPtr;
  prevPtr->nextAsteroid = newAst;
  
}

void asteroidDestroy(Asteroid *asteroid, AsteroidList *asteroids)
{
  AsteroidList *astPtr, *prevPtr;
  astPtr = asteroids->nextAsteroid;
  prevPtr = asteroids;

  while (astPtr != NULL)
  {
    if (astPtr->asteroid.astNum == asteroid->astNum)
    {
      prevPtr->nextAsteroid = astPtr->nextAsteroid;
      free(astPtr);
      break;
    }
    prevPtr = astPtr;
    astPtr = astPtr->nextAsteroid;
  }
}

void asteroidHandler(AsteroidList *asteroids, PowerUpList *powerUps, Player *player, Timer *spawnTimer, double delta)
{
  /* Asteroids - Bullet Collision Detector & Destroyer */
  AsteroidList *astPtr = asteroids->nextAsteroid;
  while(astPtr != NULL)
  {
    BulletList *bullPtr = player->bullets.nextBullet;
    while (bullPtr != NULL)
    {
      if (SDL_HasRectIntersectionFloat(&astPtr->asteroid.rect, &bullPtr->bullet.rect) && astPtr->asteroid.health > -1)
      {
        astPtr->asteroid.health -= bullPtr->bullet.damage; /* Reduce Health */
        if (astPtr->asteroid.health == 0)
        {
          int powerChance = SDL_rand(10);
          if (powerChance != 10)
          {
            powerUpSpawn(&astPtr->asteroid, powerUps);
          }
         
          asteroidDestroy(&astPtr->asteroid, asteroids); /* Destroy Objects */
          bulletDestroy(&bullPtr->bullet, &player->bullets);

          player->score++;
        }
      }
      bullPtr = bullPtr->nextBullet;
    }
    astPtr = astPtr->nextAsteroid;
  }

  /* Asteroid Spawner */
  if (spawnTimer->ticks > SDL_rand(3000) + 2000)
  {
    asteroidSpawn(asteroids); 
    timerStop(spawnTimer); /* Stop and Start to Reset Timer */
    timerStart(spawnTimer);
  }

  /* Asteroid Movement */
  astPtr = asteroids->nextAsteroid;
  while (astPtr != NULL)
  {
    astPtr->asteroid.posX += astPtr->asteroid.velX * astPtr->asteroid.speed * delta;
    astPtr->asteroid.posY += astPtr->asteroid.velY * astPtr->asteroid.speed * delta;

    /* Screen Looping */
    if (astPtr->asteroid.posX + astPtr->asteroid.width < 0)
    {
      astPtr->asteroid.posX = WIDTH; /* Left to Right */
    }

    else if (astPtr->asteroid.posX > WIDTH)
    {
      astPtr->asteroid.posX = 0 - astPtr->asteroid.width; /* Right to Left */
    }

    if (astPtr->asteroid.posY + astPtr->asteroid.height < 0)
    {
      astPtr->asteroid.posY = HEIGHT; /* Top to Bottom */
    }

    else if (astPtr->asteroid.posY > HEIGHT)
    {
      astPtr->asteroid.posY = 0 - astPtr->asteroid.height; /* Bottom to Top */
    }
    
    astPtr->asteroid.rect.x = astPtr->asteroid.posX;
    astPtr->asteroid.rect.y = astPtr->asteroid.posY;

    astPtr = astPtr->nextAsteroid;
  }

  timerCalcTicks(spawnTimer);
}


/* Buttons */
/* Structure */
typedef struct Button
{
  float width;
  float height;
  float posX;
  float posY; 

  bool hovered;
  bool clicked;

  SDL_FRect rect;

} Button;

/* Functions */
bool buttonStateUpdater(Button *button)
{
  float mouseX;
  float mouseY;
  SDL_MouseButtonFlags mouseState = SDL_GetMouseState(&mouseX, &mouseY); //Getting The State of the mouse

  if (mouseX > button->posX && mouseX < button->posX + button->width && //Checking X Collision
      mouseY > button->posY && mouseY < button->posY + button->height ) //Checking Y Collision
  {
    button->hovered = true;
    if (mouseState == SDL_BUTTON_LMASK)
    {
      button->clicked = true;
    }
    else
    {
      button->clicked = false;
    }
    
  }
  else
  {
    button->hovered = false;
    button->clicked = false;
  }
}

void gameOver(Player *player)
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
      printf("SDL_Image could not be initialized! SDL_Image Error %s\n", SDL_GetError());
      success = false;
    }
    else
    {
      if (TTF_Init() < 0)
      {
        printf("SDL_ttf could not be initialized! SDL_ttf Error: %s\n", SDL_GetError());
        success = false;
      }
      else
      {  
        gWindow = SDL_CreateWindow("Astroids-P1", WIDTH, HEIGHT, SDL_WINDOW_BORDERLESS);
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
            gTextEngine = TTF_CreateRendererTextEngine(gRenderer);
            if (gTextEngine == NULL)
            {
              printf("TextEngine could not be created! SDL_ttf Error: %s\n", SDL_GetError());
            }
            else
            {
              jetBrainsMono = TTF_OpenFont("Assets/JetBrainsMono-Medium.ttf", HEIGHT/50);
              if (jetBrainsMono == NULL)
              {
                printf("'JetBrainsMono-Medium.ttf' could not be loaded! SDL_ttf Error: %s\n", SDL_GetError());
              }
              else
              {
                /* code */
              }
              
            }
          }
        }
      }
    }
  }

  return success;
}

bool load(Player *player) /* Get Game Objects and load their respective assets */
{
  bool success = true;

  player->icon = SDL_CreateTextureFromSurface(gRenderer, IMG_Load("Assets/Crystal.png"));
  if (player->icon == NULL)
  {
    printf("'Crystal.png' could not be loaded! SDL_image Error: %s\n", SDL_GetError());
  }

  return success;
}

void draw(Player *player, AsteroidList *asteroids, PowerUpList *powerUps, Button buttons[])
{
  if (gameState == MENU)
  {
    char buttonsText[3][10] = {"Play", "Scores", "Quit"};
    SDL_SetRenderDrawColor(gRenderer, 0x22, 0x22, 0x11, 0xFF);
    SDL_RenderClear(gRenderer);

    for (uint8 i = 0; i < 3; i++) //Number of Buttons
    {
      TTF_Text *buttonText = TTF_CreateText(gTextEngine, jetBrainsMono, buttonsText[i], strlen(buttonsText[i]));
      
      if (buttons[i].hovered)
      {
        SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
        TTF_SetTextColor(buttonText, 255, 255, 255, 255);
      }
      else
      {
        SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255);
        TTF_SetTextColor(buttonText, 0, 0, 0, 255);
      }
      
      SDL_RenderFillRect(gRenderer, &buttons[i].rect);
      TTF_DrawRendererText(buttonText, buttons[i].posX + buttons[i].width/2 - strlen(buttonText->text) * 5,
                                       buttons[i].posY + buttons[i].height/2 - HEIGHT/75);
    }
  }
  
  else if (gameState == GAME)
  {
    SDL_SetRenderDrawColor(gRenderer, 0x22, 0x22, 0x11, 0xFF);
    SDL_RenderClear(gRenderer);

    SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
    //SDL_RenderFillRect(gRenderer, &player.rect);
    
    BulletList *bullPtr = &player->bullets;
    while (bullPtr != NULL)
    {
      if (!SDL_HasRectIntersectionFloat(&player->rect, &bullPtr->bullet.rect) && bullPtr->bullet.damage != -1)
      {
        SDL_RenderRect(gRenderer, &bullPtr->bullet.rect); /* Render Bullets only when they're a bit away from you*/
      }

      bullPtr = bullPtr->nextBullet;
    }

    AsteroidList *astPtr = asteroids;
    while(astPtr != NULL) /* Render Asteroids */
    {
      if (astPtr->asteroid.health != -1)
      {
        SDL_RenderFillRect(gRenderer, &astPtr->asteroid.rect);
      }
      astPtr = astPtr->nextAsteroid;
    }

    PowerUpList *powerPtr = powerUps;
    while (powerPtr != NULL)
    {
      if (powerPtr->power.powerUp == SHIELD)
      {
        SDL_SetRenderDrawColor(gRenderer, 0, 0, 255, 255);
      }
      else if (powerPtr->power.powerUp == HEALTH)
      {
        SDL_SetRenderDrawColor(gRenderer, 255, 0, 0, 255);
      }
      else if (powerPtr->power.powerUp == INFBULL)
      {
        SDL_SetRenderDrawColor(gRenderer, 0, 255, 255, 255);
      }
      else if (powerPtr->power.powerUp == INFFUEL)
      {
        SDL_SetRenderDrawColor(gRenderer, 255, 0, 255, 255);
      }
      
      SDL_RenderFillRect(gRenderer, &powerPtr->power.rect);

      powerPtr = powerPtr->nextPowerUp;
    }
    

    playerTextHandler(player);
    playerRender(*player);
  }

  if (gameState == PAUSED)
  {
    char buttonsText[2][10] = {"Resume", "Menu"};
    SDL_SetRenderDrawColor(gRenderer, 0x22, 0x22, 0x11, 0x77);
    SDL_RenderFillRect(gRenderer, NULL);

    for (uint8 i = 0; i < 2; i++) //Number of Buttons
    {
      TTF_Text *buttonText = TTF_CreateText(gTextEngine, jetBrainsMono, buttonsText[i], strlen(buttonsText[i]));
      
      if (buttons[i].hovered)
      {
        SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
        TTF_SetTextColor(buttonText, 255, 255, 255, 255);
      }
      else
      {
        SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255);
        TTF_SetTextColor(buttonText, 0, 0, 0, 255);
      }
      
      SDL_RenderFillRect(gRenderer, &buttons[i].rect);
      TTF_DrawRendererText(buttonText, buttons[i].posX + buttons[i].width/2 - strlen(buttonText->text) * 5,
                                       buttons[i].posY + buttons[i].height/2 - HEIGHT/75);
    }
  }
  

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

  bool run = true;
  while (run)
  {
    if (gameState == MENU) /* Main Menu */
    {
      /* Buttons */
      Button buttons[3];

      Button playButton; /* Play */
      playButton.clicked = 0;
      playButton.hovered = 0;
      playButton.width = 200;
      playButton.height = 50;
      playButton.posX = (WIDTH - playButton.width)/2;
      playButton.posY = 1*HEIGHT/4 - playButton.height/2;
      playButton.rect.w = playButton.width;
      playButton.rect.h = playButton.height;
      playButton.rect.x = playButton.posX;
      playButton.rect.y = playButton.posY;

      Button scoreButton; /* Scores */
      scoreButton.clicked = 0;
      scoreButton.hovered = 0;
      scoreButton.width = 200;
      scoreButton.height = 50;
      scoreButton.posX = (WIDTH - scoreButton.width)/2;
      scoreButton.posY = 2*HEIGHT/4 - scoreButton.height/2;
      scoreButton.rect.w = scoreButton.width;
      scoreButton.rect.h = scoreButton.height;
      scoreButton.rect.x = scoreButton.posX;
      scoreButton.rect.y = scoreButton.posY;
      
      Button quitButton; /* Quit */
      quitButton.clicked = 0;
      quitButton.hovered = 0;
      quitButton.width = 200;
      quitButton.height = 50;
      quitButton.posX = (WIDTH - quitButton.width)/2;
      quitButton.posY = 3*HEIGHT/4 - quitButton.height/2;
      quitButton.rect.w = quitButton.width;
      quitButton.rect.h = quitButton.height;
      quitButton.rect.x = quitButton.posX;
      quitButton.rect.y = quitButton.posY;

      buttons[0] = playButton;
      buttons[1] = scoreButton;
      buttons[2] = quitButton;

      SDL_Event e;
      bool exited = false;

      while (!exited)
      {
        while (SDL_PollEvent(&e) != 0)
        {
          if (e.type == SDL_EVENT_QUIT)
          {
            exited = true;
            run = false;
          }
        }

        for (uint8 i = 0; i < 3; i++)
        {
          buttonStateUpdater(&buttons[i]);
        }

        if (buttons[0].clicked == true)
        {
          gameState = GAME;
        }

        else if (buttons[2].clicked == true)
        {
          exited = true;
          run = false;
        }

        if (gameState != MENU)
        {
          break;
        }
        
        draw(NULL, NULL, NULL, buttons);
      }
    }

    else if (gameState == GAME)
    {
      /* Initializing Game Objects */
      /* Delta Timer */
      DeltaTimer dTimer;

      dTimer.frameStart = SDL_GetPerformanceCounter();
      dTimer.frameEnd = 0;
      dTimer.delta = 0;

      /* Player */
      Player player;
      playerInit(&player);
      
      /* Asteroids */
      AsteroidList asteroids = asteroidInit();


      Timer astSpawnTimer;
      timerStart(&astSpawnTimer);

      PowerUpList powerUps = powerUpInit();

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
            run = false;
          }

          playerEventHandler(e, &player);
        }

        if (gameState == PAUSED)
        {
          Button buttons[2];

          Button resumeButton;
          resumeButton.clicked = 0;
          resumeButton.hovered = 0;
          resumeButton.width = 200;
          resumeButton.height = 50;
          resumeButton.posX = (WIDTH - resumeButton.width)/2;
          resumeButton.posY = 1*HEIGHT/3 - resumeButton.height/2;
          resumeButton.rect.w = resumeButton.width;
          resumeButton.rect.h = resumeButton.height;
          resumeButton.rect.x = resumeButton.posX;
          resumeButton.rect.y = resumeButton.posY;

          Button menuButton;
          menuButton.clicked = 0;
          menuButton.hovered = 0;
          menuButton.width = 200;
          menuButton.height = 50;
          menuButton.posX = (WIDTH - menuButton.width)/2;
          menuButton.posY = 2*HEIGHT/3 - menuButton.height/2;
          menuButton.rect.w = menuButton.width;
          menuButton.rect.h = menuButton.height;
          menuButton.rect.x = menuButton.posX;
          menuButton.rect.y = menuButton.posY;

          buttons[0] = resumeButton;
          buttons[1] = menuButton;

          SDL_Event e;
          bool paused = true;

          while (paused)
          {
            while (SDL_PollEvent(&e) != 0)
            {
              if (e.type == SDL_EVENT_QUIT)
              {
                paused = false;
                run = false;
              }
              
              if (e.type == SDL_EVENT_KEY_DOWN && e.key.repeat == false)
              {
                if (e.key.key == SDLK_ESCAPE)
                {
                  gameState = GAME;
                  paused = false;
                }
              }
            }
            for (uint8 i = 0; i < 2; i++)
            {
              buttonStateUpdater(&buttons[i]);
            }

            if (buttons[0].clicked)
            {
              gameState = GAME;
              paused = false;
            }

            if (buttons[1].clicked)
            {
              gameState = MENU;
              paused = false;
              exited = true;
            }

            if (gameState != PAUSED)
            {
              break;
            }
            
            draw(NULL, NULL, NULL, buttons);

          }
        }
        deltaCalc(&dTimer);

        playerMovementHandler(&player, dTimer.deltaInSecs);
        playerBulletHander(&player, dTimer.deltaInSecs);
        playerPowerUpHandler(&player, &powerUps);
        asteroidHandler(&asteroids, &powerUps, &player, &astSpawnTimer, dTimer.deltaInSecs);

        if (gameState != GAME)
        {
          break;
        }
        
        draw(&player, &asteroids, &powerUps, NULL); /* Draw, Blit and Render */
      }
    }

    if (gameState == OVER)
    {
      
    }

  }

  quit(); // Quit
  return 0;
}