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

#define PRINTERROR printf("%s", SDL_GetError());

#define PI 3.14159265359
#define WIDTH 1280
#define HEIGHT 720
#define ASTLIMIT 100

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
typedef struct
{
  Uint32 frameStart;
  Uint32 frameEnd;

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
  int bulletNum;

} Bullet;

/* Linked List */
struct BulletList
{

  Bullet bullet;
  struct BulletList *nextBullet;

};

void bulletDestroy(Bullet *bullet, struct BulletList *bullets)
{
  struct BulletList *bulletPtr, *prevPtr;
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
typedef struct {
  float width;
  float height;
  float posX;
  float posY;

  SDL_FRect rect;

  enum Power powerUp;
} PowerUp;

/* Player Definitions*/
/* Struct */
typedef struct
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

  Uint8 magSize; /* 10 */
  int bulletNum;

  struct BulletList bullets;

  _Bool reloading;
  Uint32 reloadTime;
  Timer bulletTimer;

  Uint64 score;
  TTF_Text *scoreText;

  bool shield;
  bool heal;
  bool infBulls;
  bool infFuel;

  Timer shieldTimer;
  Timer infBullsTimer;
  Timer infFuelTimer;

} Player;

/* Functions */
void playerInit(Player *player)
{
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

  player->bulletTimer.startTicks = 0;
  player->bulletTimer.pausedTicks = 0;
  player->bulletTimer.started = 0;
  player->bulletTimer.paused = 0;
  player->bulletTimer.ticks = 0;

  player->reloadTime = 5000;
  player->reloading = false;

  player->icon = NULL;

  Bullet dummy;
  dummy.posX = -150;
  dummy.posY = -150;
  dummy.speed = 0;
  dummy.width = 0;
  dummy.height = 0;
  dummy.damage = -1;
  dummy.bulletNum = 0;
  player->bullets.bullet = dummy;
  player->bullets.nextBullet = NULL;
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
  struct BulletList *bulletPtr, *prevPtr, *newBullet;
  int bulletNum = 1;
  prevPtr = &player->bullets;
  bulletPtr = player->bullets.nextBullet;
  newBullet = (struct BulletList*) malloc(sizeof(struct BulletList));

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
  
  struct BulletList *bulletPtr = &player->bullets;
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
      if (!player->reloading)
      {
        playerShoot(player);
      }
      break;
    
    case SDLK_L:
      if (!player->afterBurnerOverheat && player->afterBurnerFuel >= player->maxAfterBurnerFuel)
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
  else if (player->afterburning && player->afterBurnerFuel > 0)
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

void playerPowerUpHandler(Player *player)
{
  if (player->shield) timerStart(&player->shieldTimer);
  if (player->shieldTimer.started && player->shieldTimer.ticks > 500) timerStop(&player->shieldTimer);

  if (player->infBulls) timerStart(&player->infBullsTimer);
  if (player->shieldTimer.started && player->infBullsTimer.ticks > 500) timerStop(&player->infBullsTimer);

  if (player->infFuel) timerStart(&player->infFuelTimer);
  if (player->infFuelTimer.started && player->infFuelTimer.ticks > 500) timerStop(&player->infFuelTimer); 
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
  SDL_FRect ShieldRect = {10, 10, 20, 20};
  SDL_FRect InfBulletsRect = {10, 10, 40, 20};
  SDL_FRect InfFuelRect = {10, 10, 60, 20};

  if (player.shieldTimer.started)
  {
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 255, 255);
    SDL_RenderFillRect(gRenderer, &ShieldRect);
  }
  else if (player.infBullsTimer.started)
  {
    SDL_SetRenderDrawColor(gRenderer, 255, 255, 0, 255);
    SDL_RenderFillRect(gRenderer, &InfBulletsRect);
  }
  else if (player.infFuelTimer.started)
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

/* PowerUp Functions */
/* Functions */
PowerUp powerUpSpawn(Asteroid *asteroid)
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

  return power;
}

/* Functions */
void asteroidInit(Asteroid *asteroids)
{
  for (uint8_t i = 0; i < ASTLIMIT; i++)
  {
    asteroids[i].health = -1;
    asteroids[i].width = 0;
    asteroids[i].height = 0;
    asteroids[i].posX = -100;
    asteroids[i].posY = -100;
    asteroids[i].speed = 0;
  }
}

void asteroidSpawn(Asteroid *asteroids, Timer *spawnTimer, Uint8 *astNum)
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

void asteroidHandler(Asteroid *asteroids, PowerUp *powerUps, Player *player, Timer *spawnTimer, Uint8 *astNum, Uint8 *powerNum, double delta)
{
  /* Asteroids - Bullet Collision Detector & Destroyer */
  for (uint8_t ast = 0; ast < ASTLIMIT; ast++)
  {
    struct BulletList *ptr = &player->bullets;
    while (ptr != NULL)
    {
      if (SDL_HasRectIntersectionFloat(&(asteroids + ast)->rect, &ptr->bullet.rect) && (asteroids + ast)->health > -1)
      {
        (asteroids + ast)->health -= ptr->bullet.damage; /* Reduce Health */
        if ((asteroids + ast)->health == 0)
        {
          asteroidDestroy(asteroids + ast); /* Destroy Objects */
          bulletDestroy(&ptr->bullet, &player->bullets);
          int powerChance = SDL_rand(10);
          if (powerChance != 10)
          {
            if ((*powerNum) == 9)
            {
              (*powerNum) = 0;
            }
            powerUps[*powerNum] = powerUpSpawn(asteroids + ast);
            (*powerNum)++;
          }
          player->score++;
        }
      }

      ptr = ptr->nextBullet;
    }
  }

  /* Asteroid Spawner */
  if (spawnTimer->ticks > SDL_rand(3000) + 2000 && (*astNum) < ASTLIMIT)
  {
    if ((*astNum) > ASTLIMIT)
    {
      (*astNum) = 0;
    }

    asteroidSpawn(asteroids, spawnTimer, astNum); 
    timerStop(spawnTimer); /* Stop and Start to Reset Timer */
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


/* Buttons */
/* Structure */
typedef struct 
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

void draw(Player *player, Asteroid asteroids[], PowerUp powerUps[], Button buttons[])
{
  if (gameState == MENU)
  {
    char buttonsText[3][10] = {"Play", "Scores", "Quit"};
    SDL_SetRenderDrawColor(gRenderer, 0x22, 0x22, 0x11, 0xFF);
    SDL_RenderClear(gRenderer);

    for (uint8_t i = 0; i < 3; i++) //Number of Buttons
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
    
    struct BulletList *ptr = &player->bullets;
    while (ptr != NULL)
    {
      if (!SDL_HasRectIntersectionFloat(&player->rect, &ptr->bullet.rect) && ptr->bullet.damage != -1)
      {
        SDL_RenderRect(gRenderer, &ptr->bullet.rect); /* Render Bullets only when they're a bit away from you*/
      }

      ptr = ptr->nextBullet;
    }

    for (uint8_t i = 0; i < ASTLIMIT; i++) /* Render Asteroids */
    {
      if (asteroids[i].health != -1)
      {
        SDL_RenderFillRect(gRenderer, &asteroids[i].rect);
      }
    }

    for (uint8_t i = 0; i < 10; i++)
    {
      if (powerUps[i].powerUp == SHIELD)
      {
        SDL_SetRenderDrawColor(gRenderer, 0, 0, 255, 255);
      }
      else if (powerUps[i].powerUp == HEALTH)
      {
        SDL_SetRenderDrawColor(gRenderer, 255, 0, 0, 255);
      }
      else if (powerUps[i].powerUp == INFBULL)
      {
        SDL_SetRenderDrawColor(gRenderer, 0, 255, 255, 255);
      }
      else if (powerUps[i].powerUp == INFFUEL)
      {
        SDL_SetRenderDrawColor(gRenderer, 255, 0, 255, 255);
      }
      
      SDL_RenderFillRect(gRenderer, &powerUps[i].rect);
    }
    

    playerTextHandler(player);
    playerRender(*player);
  }

  if (gameState == PAUSED)
  {
    char buttonsText[2][10] = {"Resume", "Menu"};
    SDL_SetRenderDrawColor(gRenderer, 0x22, 0x22, 0x11, 0x77);
    SDL_RenderFillRect(gRenderer, NULL);

    for (uint8_t i = 0; i < 2; i++) //Number of Buttons
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

        for (uint8_t i = 0; i < 3; i++)
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
      Asteroid asteroids[ASTLIMIT + 1];


      Uint8 astNum = 0;
      asteroidInit(asteroids);

      Timer astSpawnTimer;
      timerStart(&astSpawnTimer);

      PowerUp powerUps[10];
      uint8_t powerNum = 0;

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
            for (uint8_t i = 0; i < 2; i++)
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
        asteroidHandler(asteroids, powerUps, &player, &astSpawnTimer, &astNum, &powerNum, dTimer.deltaInSecs);

        if (gameState != GAME)
        {
          break;
        }
        
        draw(&player, asteroids, powerUps, NULL); /* Draw, Blit and Render */
      }
    }

    if (gameState == OVER)
    {
      
    }

  }

  quit(); // Quit
  return 0;
}