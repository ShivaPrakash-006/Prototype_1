#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "deps/cJSON.h"
#include <libxml/parser.h>
#include <libxml/tree.h>
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
#define PRINTIMGERROR(PATH) printf("'%s' could not be loaded. SDL_image Error: %s", PATH, SDL_GetError());
#define PI 3.14159265359
#define WIDTH 1280
#define HEIGHT 720
#define SPRITEMAX 77

enum State
{ /* Possible Game States */
  MENU = 0,
  GAME = 1,
  PAUSED = 2,
  OVER = 3,
  SCORES = 4
};

enum Power
{ /* Possible PowerUps */
  SHIELD = 0,
  ARMOR = 1,
  INFFUEL = 2,
};

enum Size
{
  SMALL = 0,
  NORMAL = 1,
  LARGE = 2
};

enum Sort
{
  SCORE = 0,
  TIME = 1,
  NAME = 2
};

enum State gameState = MENU;

SDL_Window *gWindow = NULL;
SDL_Renderer *gRenderer = NULL;

SDL_Texture *menuBack1 = NULL;
SDL_Texture *menuBack2 = NULL;
SDL_Texture *scoreBack = NULL;
SDL_Texture *gameBack = NULL;
SDL_Texture *pauseBack = NULL;
SDL_Texture *overBack = NULL;

SDL_Texture *spriteSheet = NULL;

TTF_Font *kenVectorFont = NULL;
TTF_TextEngine *gTextEngine = NULL;

/* Sprtie Definitions */
/* Struct */
typedef struct Sprite
{
  char name[50];
  int width, height, x, y;
} Sprite;

Sprite spriteList[SPRITEMAX] = {};

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
Timer timerInit(void)
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

void timerReset(Timer *timer)
{
  timerStop(timer);
  timerStart(timer);
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
} DeltaTimer;

/* Functions */
void deltaCalc(DeltaTimer *dTimer)
{
  dTimer->frameEnd = dTimer->frameStart;
  dTimer->frameStart = SDL_GetPerformanceCounter();

  dTimer->delta = (dTimer->frameStart - dTimer->frameEnd) / (double)SDL_GetPerformanceFrequency();
}

/* Bullet Definitions */
/* Struct */
typedef struct Bullet
{
  int width;  /* 10 */
  int height; /* 10 */

  float posX;
  float posY;

  int damage; /* 1 */

  float speed; /* 1000 */

  float velX;
  float velY;

  SDL_FRect rect;
  uint32 bulletNum;

  uint32 lifeTime;
  Timer lifeTimer;

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

typedef struct PowerUpList
{

  PowerUp power;
  struct PowerUpList *nextPowerUp;

} PowerUpList;

/* Functions */
struct PowerUpList powerUpInit(void)
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
  short armor; /* 3 */

  short width;  /* 10 */
  short height; /* 10 */

  float posX;
  float posY;
  float rot;

  float moveSpeed;

  float rotSpeed;

  _Bool moving;
  _Bool afterburning;
  float velX;
  float velY;
  float rotVel;

  float maxAfterBurnerFuel;       /* 100 */
  float afterBurnerRefuelRate;    /* 5 */
  float afterBurnerDepletionRate; /* 10 */
  float afterBurnerFuel;
  _Bool afterBurnerOverheat;
  Timer overHeatTimer;
  int afterBurnerCooldown; /* 5 Seconds */

  float leapDistance; /* 100 */

  SDL_FRect rect;

  SDL_Texture *icon;

  _Bool shooting;

  BulletList bullets;

  Timer bulletTimer;
  uint32 shootDelay; /* 200 */

  uint64 score;
  TTF_Text *scoreText;

  bool shield;
  bool repair;
  bool infFuel;

  Timer shieldTimer;
  uint32 shieldTime;
  Timer infFuelTimer;
  uint32 infFuelTime;

  Timer gameTimer;
} Player;

/* Functions */
void playerInit(Player *player)
{
  player->armor = 3;
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
  player->shield = false;
  player->shieldTime = 5000;
  player->shieldTimer = timerInit();
  player->repair = false;
  player->infFuel = false;
  player->infFuelTime = 5000;
  player->infFuelTimer = timerInit();

  player->gameTimer = timerInit();
  timerStart(&player->gameTimer);
}

void playerLogs(Player player)
{
  printf("posX: %f\n", player.posX);
  printf("posY: %f\n", player.posY);
  printf("rot: %f\n", player.rot);
  printf("rotVel: %f\n", player.rotVel);
  printf("moving: %i\n", player.moving);
  printf("velX: %f\n", player.velX);
  printf("velY: %f\n", player.velY);
}

void playerShoot(Player *player, int num)
{
  Bullet bullet; /* Creating A Bullet*/

  /* Initialization of bullet */
  bullet.width = 10;
  bullet.height = 10;

  bullet.damage = 1;
  bullet.speed = 1000;

  if (num == 1)
  {
    bullet.velX = SDL_sinf((player->rot + 10) * PI / 180);
    bullet.velY = -SDL_cosf((player->rot + 10) * PI / 180);
  }
  else
  {
    bullet.velX = SDL_sinf((player->rot - 10) * PI / 180);
    bullet.velY = -SDL_cosf((player->rot - 10) * PI / 180);
  }

  bullet.posX = player->posX + player->width / 2 - bullet.width / 2;   /* Center of X */
  bullet.posY = player->posY + player->height / 2 - bullet.height / 2; /* Center of Y */

  bullet.rect.w = bullet.width;
  bullet.rect.h = bullet.height;
  bullet.rect.x = bullet.posX;
  bullet.rect.y = bullet.posY;

  bullet.lifeTimer = timerInit();
  bullet.lifeTime = 1500; /* 1.5 secs */
  timerStart(&bullet.lifeTimer);

  /* Linked List For Optimised Allocation */
  BulletList *bulletPtr, *prevPtr, *newBullet;
  uint bulletNum = 1;
  prevPtr = &player->bullets;
  bulletPtr = player->bullets.nextBullet;
  newBullet = (BulletList *)malloc(sizeof(BulletList));

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
}

void playerBulletHander(Player *player, double delta)
{
  if (player->shooting && player->bulletTimer.ticks > player->shootDelay)
  {
    playerShoot(player, 1);
    playerShoot(player, 2);
    timerReset(&player->bulletTimer);
  }

  BulletList *bulletPtr = &player->bullets;
  while (bulletPtr != NULL)
  {
    /* Moving the bullets */
    bulletPtr->bullet.posX += bulletPtr->bullet.velX * bulletPtr->bullet.speed * delta;
    bulletPtr->bullet.posY += bulletPtr->bullet.velY * bulletPtr->bullet.speed * delta;

    bulletPtr->bullet.rect.x = bulletPtr->bullet.posX;
    bulletPtr->bullet.rect.y = bulletPtr->bullet.posY;

    /* Looping */
    if (bulletPtr->bullet.posX + bulletPtr->bullet.width < 0)
    {
      bulletPtr->bullet.posX = WIDTH; /* Left to Right */
    }

    else if (bulletPtr->bullet.posX > WIDTH)
    {
      bulletPtr->bullet.posX = 0 - bulletPtr->bullet.width; /* Right to Left */
    }

    if (bulletPtr->bullet.posY + bulletPtr->bullet.height < 0)
    {
      bulletPtr->bullet.posY = HEIGHT; /* Top to Bottom */
    }

    else if (bulletPtr->bullet.posY > HEIGHT)
    {
      bulletPtr->bullet.posY = 0 - bulletPtr->bullet.height; /* Bottom to Top */
    }

    timerCalcTicks(&bulletPtr->bullet.lifeTimer);
    if (bulletPtr->bullet.lifeTimer.ticks > bulletPtr->bullet.lifeTime)
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
      if ((!player->afterBurnerOverheat && player->afterBurnerFuel > 0) || player->infFuelTimer.started)
      {
        player->posX += player->velX * player->leapDistance;
        player->posY += player->velY * player->leapDistance;
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

    case SDLK_D:
      player->rotVel -= 1;
      break;

    case SDLK_A:
      player->rotVel += 1;
      break;

    case SDLK_J:
      player->afterburning = false;

    case SDLK_K:
      player->shooting = false;

    default:
      break;
    }
  }
}

void playerMovementHandler(Player *player, double delta)
{
  /* Rotation */
  if (player->rotVel > 0) /* Holding Right */
    player->rot += player->rotSpeed * delta;
  else if (player->rotVel < 0) /* Holding Left */
    player->rot -= player->rotSpeed * delta;
  if (player->rotVel != 0) /* Calculating velX and velY after rotating */
  {
    /* Calculating velX and velY, also force on both Axes through trigonometry */
    player->velX = SDL_sinf(player->rot * PI / 180);  /* rot * PI / 180 to convert to radians */
    player->velY = -SDL_cosf(player->rot * PI / 180); /* Due to Y coordinates of SDL, a minus is required */
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

  if (player->moving)
  {
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
  if (!player->afterburning && player->afterBurnerFuel < player->maxAfterBurnerFuel)
    player->afterBurnerFuel += player->afterBurnerRefuelRate;
  else if (player->afterburning && player->afterBurnerFuel > 0 && !player->infFuelTimer.started)
    player->afterBurnerFuel -= player->afterBurnerDepletionRate;
  else if (player->afterBurnerFuel <= 0)
  {
    player->afterBurnerOverheat = true;
    player->afterburning = false;
  }

  if (player->afterBurnerOverheat && player->afterBurnerFuel >= player->maxAfterBurnerFuel)
    player->afterBurnerOverheat = false;
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
      else if (powerPtr->power.powerUp == ARMOR)
      {
        player->repair = true;
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

  if (player->repair)
  {
    player->armor++;
    player->repair = false;
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
  timerCalcTicks(&player->infFuelTimer);
}

void playerTextHandler(Player *player)
{
  /* Score */
  char *score;                                      /* Number of characters in "Score: 999" */
  SDL_asprintf(&score, "Score: %u", player->score); /* To join string with int */
  player->scoreText = TTF_CreateText(gTextEngine, kenVectorFont, score, 0);
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
  SDL_FRect InfFuelRect = {60, 20, 10, 10};

  if (player.shieldTimer.started)
  {
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 255, 255);
    SDL_RenderFillRect(gRenderer, &ShieldRect);
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

  player->moveSpeed = 0;

  player->moving = false;
}

/* Asteroid Definitions */
/* Struct */
typedef struct Asteroid
{
  enum Size size;

  short width;  /* 20 */
  short height; /* 20 */

  float posX;
  float posY;

  float velX;
  float velY;
  float rotVel;

  float speed;

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
  PowerUp power;
  power.width = 16;
  power.height = 16;
  power.posX = asteroid->posX + asteroid->width / 2;
  power.posY = asteroid->posY + asteroid->height / 2;
  power.rect.w = power.width;
  power.rect.h = power.height;
  power.rect.x = power.posX;
  power.rect.y = power.posY;
  power.powerUp = (enum Power)SDL_rand(4);

  PowerUpList *powerPtr, *prevPtr, *newPower;
  powerPtr = powerUps->nextPowerUp;
  prevPtr = powerUps;
  newPower = (PowerUpList *)malloc(sizeof(PowerUpList));
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
AsteroidList asteroidInit(void)
{
  AsteroidList asteroids;
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

void asteroidSpawn(AsteroidList *asteroids, Asteroid *refAsteroid)
{
  Asteroid asteroid; /* SDL_rand(Number Of Outcomes) + lowerValue -> lowerValue to NumberOfOutcome - 1*/

  if (refAsteroid == NULL) /* Normal Spawn */
  {
    asteroid.size = (enum Size)SDL_rand(3);
  }
  else /* Spawn Based On Destroyed Asteroid */
  {
    asteroid.size = refAsteroid->size - 1;
  }

  if (asteroid.size == SMALL)
  {
    asteroid.width = 30;
    asteroid.height = 30;
    asteroid.speed = SDL_rand(100) + 300;
  }
  else if (asteroid.size == NORMAL)
  {
    asteroid.width = 80;
    asteroid.height = 80;
    asteroid.speed = SDL_rand(100) + 200;
  }
  else
  {
    asteroid.width = 200;
    asteroid.height = 200;
    asteroid.speed = SDL_rand(100) + 100;
  }

  switch (SDL_rand(4))
  {
  case 0: /* Up */
    asteroid.posX = SDL_rand(WIDTH);
    asteroid.posY = 0 - asteroid.height;

    asteroid.velX = (SDL_rand(20) - 10) / (float)10; /* -1.0 -> 0.9 */
    if (asteroid.velX >= 0)
      asteroid.velX += 0.1;                         /* -1.0 -> -0.1 U 0.1 -> 1.0 */
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
      asteroid.velX += 0.1;                          /* -1.0 -> -0.1 U 0.1 -> 1.0 */
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

  asteroid.rect.x = asteroid.posX;
  asteroid.rect.y = asteroid.posY;
  asteroid.rect.w = asteroid.width;
  asteroid.rect.h = asteroid.height;

  AsteroidList *astPtr, *prevPtr, *newAst;
  astPtr = asteroids->nextAsteroid;
  prevPtr = asteroids;
  newAst = (AsteroidList *)malloc(sizeof(AsteroidList));
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
  if (asteroid->size != SMALL) /* Spawn Asteroids Of Larger Asteroid */
  {
    uint8 num = SDL_rand(4) + 3; /* Number Of Asteroids to Spawn */
    for (uint8 astNum = 0; astNum < num; astNum++)
    {
      asteroidSpawn(asteroids, asteroid);
    }
  }

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

void asteroidHandler(AsteroidList *asteroids, PowerUpList *powerUps, Player *player, Timer *spawnTimer, int *spawnCount, int spawnTime, double delta )
{
  /* Asteroids - Bullet Collision Detector & Destroyer */
  AsteroidList *astPtr = asteroids->nextAsteroid;
  while (astPtr != NULL)
  {
    bool destroyed = false;
    BulletList *bullPtr = player->bullets.nextBullet;
    while (bullPtr != NULL && !destroyed)
    {
      if (SDL_HasRectIntersectionFloat(&astPtr->asteroid.rect, &bullPtr->bullet.rect))
      {
        int powerChance = SDL_rand(20);
        if (powerChance == 10)
        {
          powerUpSpawn(&astPtr->asteroid, powerUps);
        }

        asteroidDestroy(&astPtr->asteroid, asteroids); /* Destroy Objects */
        bulletDestroy(&bullPtr->bullet, &player->bullets);
        destroyed = true;

        player->score++;
      }
      bullPtr = bullPtr->nextBullet;
    }

    if (!destroyed)
    {
      if (SDL_HasRectIntersectionFloat(&astPtr->asteroid.rect, &player->rect))
      {
        asteroidDestroy(&astPtr->asteroid, asteroids);
        if (!player->shieldTimer.started)
        {
          player->armor--;
        }
      }
    }

    astPtr = astPtr->nextAsteroid;
  }

  /* Asteroid Spawner */
  if (spawnTimer->ticks > SDL_rand(3000) + spawnTime)
  {
    asteroidSpawn(asteroids, NULL);
    (*spawnCount)++;
    timerReset(spawnTimer); /* Reset Timer */
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
  TTF_Text *text;

} Button;

/* Functions */
bool buttonStateUpdater(Button *button)
{
  float mouseX;
  float mouseY;
  SDL_MouseButtonFlags mouseState = SDL_GetMouseState(&mouseX, &mouseY); // Getting The State of the mouse

  if (mouseX > button->posX && mouseX < button->posX + button->width && // Checking X Collision
      mouseY > button->posY && mouseY < button->posY + button->height)  // Checking Y Collision
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

typedef struct ScoreObj {
  char username[50];
  int score;
  int time;
} ScoreObj;

char* extractScores(char *fileName)
{
  FILE *scoreJson = fopen(fileName, "r");
  if (scoreJson == NULL)
  {
    printf("Unable to open '%s'.", fileName);
    return NULL;
  }

  fseek(scoreJson, 0, SEEK_END);
  int length = ftell(scoreJson);
  rewind(scoreJson);

  char *jsonData = (char*) malloc(length + 1);
  fread(jsonData, 1, length, scoreJson);
  jsonData[length] = '\0';

  return jsonData;
}

char* updateScores(char *jsonData, char *username, int score, int time)
{
  cJSON *root = jsonData[0] != '\0' ? cJSON_Parse(jsonData) : cJSON_CreateObject();

  cJSON *userScores = cJSON_GetObjectItem(root, "Scores");
  if (userScores == NULL)
  {
    userScores = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "Scores", userScores);
  }

  cJSON *scoreObj = cJSON_CreateObject();
  cJSON_AddStringToObject(scoreObj, "Username", username);
  cJSON_AddNumberToObject(scoreObj, "Score", score);
  cJSON_AddNumberToObject(scoreObj, "Time", time);
  cJSON_AddItemToArray(userScores, scoreObj);

  // SORT DATA HERE
  
  jsonData = cJSON_PrintUnformatted(root);

  cJSON_Delete(root);
  return jsonData;
}

bool saveScores(char *jsonData, char *fileName)
{
  bool success = true;

  FILE *scoresJson = fopen(fileName, "w");
  if (scoresJson == NULL)
  {
    printf("Unable to open '%s'.", fileName);
    success = false;
  }
  else
  {
    fputs(jsonData, scoresJson);
    fclose(scoresJson);
  }

  return success;
}

int compareScores(const void *a, const void *b)
{
  const cJSON *scoreA = *(const cJSON**)a;
  const cJSON *scoreB = *(const cJSON**)b;

  int score1 = cJSON_GetObjectItem(scoreA, "Score")->valueint;
  int score2 = cJSON_GetObjectItem(scoreB, "Score")->valueint;

  return score2 - score1;
}

int compareTime(const void *a, const void *b)
{
  const cJSON *scoreA = *(const cJSON**)a;
  const cJSON *scoreB = *(const cJSON**)b;

  int time1 = cJSON_GetObjectItem(scoreA, "Time")->valueint;
  int time2 = cJSON_GetObjectItem(scoreB, "Time")->valueint;

  return time2 - time1;
}

int compareName(const void *a, const void *b)
{
  const cJSON *scoreA = *(const cJSON**)a;
  const cJSON *scoreB = *(const cJSON**)b;

  const char *name1 = cJSON_GetObjectItem(scoreA, "Username")->valuestring;
  const char *name2 = cJSON_GetObjectItem(scoreB, "Username")->valuestring;

  return strncmp(name1, name2, 50);
}

void sortScores(cJSON *jsonData, enum Sort type)
{
  cJSON *scores = cJSON_GetObjectItem(jsonData, "Scores");
  int count = cJSON_GetArraySize(scores);

  if (count < 2) return;

  cJSON **scoreList = malloc(count * sizeof(cJSON *));
  for (int i = 0; i < count; i++)
    scoreList[i] = cJSON_GetArrayItem(scores, i);
  
  if (type == SCORE)
    qsort(scoreList, count, sizeof(cJSON*), compareScores);
  else if (type == TIME)
    qsort(scoreList, count, sizeof(cJSON*), compareTime);
  else 
    qsort(scoreList, count, sizeof(cJSON*), compareName);

  cJSON *sortedScores = cJSON_CreateArray();
  for (int i = 0; i < count; i++)
    cJSON_AddItemToArray(sortedScores, cJSON_Duplicate(scoreList[i], 1));
  
  cJSON_ReplaceItemInObject(jsonData, "Scores", sortedScores);
  free(scoreList);
}

bool parseXML(const char* fileName)
{
  bool success = false;
  xmlDoc *spriteXML = xmlReadFile(fileName, NULL, 0);
  if (spriteXML == NULL)
  {
    printf("'%s' could not be loaded!\n", fileName);
    success = false;
  }

  xmlNode *root = xmlDocGetRootElement(spriteXML);
  xmlNode *curNode = root->children;
  int spriteNum = 0;

  while (curNode != NULL)
  {
    if (curNode->type == XML_ELEMENT_NODE)
    {
      Sprite *sprite = &spriteList[spriteNum];
      
      xmlChar *name = xmlGetProp(curNode, (const xmlChar*)"name");
      xmlChar *x = xmlGetProp(curNode, (const xmlChar*)"x");
      xmlChar *y = xmlGetProp(curNode, (const xmlChar*)"y");
      xmlChar *width = xmlGetProp(curNode, (const xmlChar*)"width");
      xmlChar *height = xmlGetProp(curNode, (const xmlChar*)"height");

      snprintf(sprite->name, sizeof(sprite->name), "%s", name);
      sprite->x = atoi((char*) x);
      sprite->y = atoi((char*) y);
      sprite->width = atoi((char*) width);
      sprite->height = atoi((char*) height);
    
      xmlFree(name);
      xmlFree(x);
      xmlFree(y);
      xmlFree(width);
      xmlFree(height); 

      spriteNum++;
    }
    curNode = curNode->next;
  }
  xmlFreeDoc(spriteXML);

  return success;
}

bool init(void)
{
  printf("=== Start Of Program ===\n");
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
            SDL_SetRenderVSync(gRenderer, 1);
            SDL_SetRenderDrawColor(gRenderer, 0x22, 0x22, 0x11, 0xFF);
            gTextEngine = TTF_CreateRendererTextEngine(gRenderer);
            if (gTextEngine == NULL)
            {
              printf("TextEngine could not be created! SDL_ttf Error: %s\n", SDL_GetError());
            }
            else
            {
              kenVectorFont = TTF_OpenFont("Assets/Fonts/kenvector_future_thin.ttf", HEIGHT / 50);
              if (kenVectorFont == NULL)
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

  if (player != NULL)
  {
    player->icon = SDL_CreateTextureFromSurface(gRenderer, IMG_Load("Assets/Crystal.png"));
    if (player->icon == NULL)
    {
      printf("'Crystal.png' could not be loaded! SDL_image Error: %s\n", SDL_GetError());
    }
  }

  menuBack1 = SDL_CreateTextureFromSurface(gRenderer, IMG_Load("Assets/Backgrounds/menu1.png"));
  if (menuBack1 == NULL)
  {
    printf("'Assets/Backgrounds/menu1.png' could not be loaded! SDL_image Error: %s\n", SDL_GetError());
    success = false;
  }
  menuBack2 = SDL_CreateTextureFromSurface(gRenderer, IMG_Load("Assets/Backgrounds/menu2.png"));
  if (menuBack2 == NULL)
  {
    printf("'Assets/Backgrounds/menu2.png' could not be loaded! SDL_image Error: %s\n", SDL_GetError());
    success = false;
  }

  scoreBack = SDL_CreateTextureFromSurface(gRenderer, IMG_Load("Assets/Backgrounds/scores.png"));
  if (scoreBack == NULL)
  {
    printf("'Assets/Backgrounds/scores.png' could not be loaded! SDL_image Error: %s\n", SDL_GetError());
    success = false;
  }

  gameBack = SDL_CreateTextureFromSurface(gRenderer, IMG_Load("Assets/Backgrounds/game.png"));
  if (gameBack == NULL)
  {
    printf("'Assets/Backgrounds/game.png' could not be loaded! SDL_image Error: %s\n", SDL_GetError());
    success = false;
  }

  pauseBack = SDL_CreateTextureFromSurface(gRenderer, IMG_Load("Assets/Backgrounds/paused.png"));
  if (pauseBack == NULL)
  {
    printf("'Assets/Backgrounds/paused.png' could not be loaded! SDL_image Error: %s\n", SDL_GetError());
    success = false;
  }

  overBack = SDL_CreateTextureFromSurface(gRenderer, IMG_Load("Assets/Backgrounds/over.png"));
  if (overBack == NULL)
  {
    printf("'Assets/Backgrounds/over.png' could not be loaded! SDL_image Error: %s\n", SDL_GetError());
    success = false;
  }

  spriteSheet = SDL_CreateTextureFromSurface(gRenderer, IMG_Load("Assets/sheet.png"));
  if (spriteSheet == NULL)
  {
    printf("'Assets/sheet.png' could not be loaded! SDL_image Error: %s\n", SDL_GetError());
    success = false;
  }

  parseXML("Assets/sheet.xml");

  return success;
}

void drawMenu(Button *buttons, float f1PosX, float f2PosX)
{
  char buttonsText[3][10] = {"Play", "Scores", "Quit"};
  SDL_SetRenderDrawColor(gRenderer, 0x06, 0x12, 0x21, 0xFF);
  SDL_RenderClear(gRenderer);

  SDL_FRect tempRect = {f1PosX, 0.f, WIDTH, HEIGHT}; //Every Image has Same Dimension
  SDL_RenderTexture(gRenderer, menuBack1, NULL, &tempRect);
  tempRect.x = f1PosX - WIDTH; 
  SDL_RenderTexture(gRenderer, menuBack1, NULL, &tempRect);
  tempRect.x = f2PosX; 
  SDL_RenderTexture(gRenderer, menuBack2, NULL, &tempRect);
  tempRect.x = f2PosX - WIDTH;
  SDL_RenderTexture(gRenderer, menuBack2, NULL, &tempRect);

    for (uint8 i = 0; i < 3; i++) // Number of Buttons
    {
      TTF_Text *buttonText = TTF_CreateText(gTextEngine, kenVectorFont, buttonsText[i], strlen(buttonsText[i]));
      int textHeight, textWidth;
      TTF_GetTextSize(buttonText, &textWidth, &textHeight);
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
      TTF_DrawRendererText(buttonText, buttons[i].posX + buttons[i].width / 2 - textWidth / 2,
                            buttons[i].posY + buttons[i].height / 2 - textHeight / 2);
    }

    SDL_RenderPresent(gRenderer);
  }

  void drawGame(Player *player, AsteroidList *asteroids, PowerUpList *powerUps, TTF_Text *fpsText)
  {
    SDL_SetRenderDrawColor(gRenderer, 0x22, 0x22, 0x11, 0xFF);
    SDL_RenderClear(gRenderer);

    
    float backWidth, backHeight;
    SDL_GetTextureSize(scoreBack, &backWidth, &backHeight);
    SDL_FRect backRect = {0, 0, backWidth, backHeight};
    for (int i = 0; i < WIDTH; i+= backWidth)
      for (int j = 0; j < HEIGHT; j+= backHeight)
      {
        backRect.x = i;
        backRect.y = j;
        SDL_RenderTexture(gRenderer, gameBack, NULL, &backRect);
      }

    SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

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
    while (astPtr != NULL) /* Render Asteroids */
    {
      SDL_RenderFillRect(gRenderer, &astPtr->asteroid.rect);
      astPtr = astPtr->nextAsteroid;
    }

    PowerUpList *powerPtr = powerUps;
    while (powerPtr != NULL)
    {
      if (powerPtr->power.powerUp == SHIELD)
      {
        SDL_SetRenderDrawColor(gRenderer, 0, 0, 255, 255);
      }
      else if (powerPtr->power.powerUp == ARMOR)
      {
        SDL_SetRenderDrawColor(gRenderer, 255, 0, 0, 255);
      }
      else if (powerPtr->power.powerUp == INFFUEL)
      {
        SDL_SetRenderDrawColor(gRenderer, 255, 0, 255, 255);
      }

      SDL_RenderFillRect(gRenderer, &powerPtr->power.rect);

      powerPtr = powerPtr->nextPowerUp;
    }

    TTF_DrawRendererText(fpsText, WIDTH - 70, HEIGHT - 20);

    playerTextHandler(player);
    playerRender(*player);

    SDL_RenderPresent(gRenderer);
  }

  void drawPaused(Button *buttons)
  {
    char buttonsText[2][10] = {"Resume", "Menu"};
    SDL_SetRenderDrawColor(gRenderer, 0x22, 0x22, 0x11, 0x77);
    SDL_RenderFillRect(gRenderer, NULL);
    
    float backWidth, backHeight;
    SDL_GetTextureSize(scoreBack, &backWidth, &backHeight);
    SDL_FRect backRect = {0, 0, backWidth, backHeight};
    for (int i = 0; i < WIDTH; i+= backWidth)
      for (int j = 0; j < HEIGHT; j+= backHeight)
      {
        backRect.x = i;
        backRect.y = j;
        SDL_RenderTexture(gRenderer, pauseBack, NULL, &backRect);
      }

    for (uint8 i = 0; i < 2; i++) // Number of Buttons
    {
      TTF_Text *buttonText = TTF_CreateText(gTextEngine, kenVectorFont, buttonsText[i], strlen(buttonsText[i]));
      int textHeight, textWidth;
      TTF_GetTextSize(buttonText, &textWidth, &textHeight);
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
      TTF_DrawRendererText(buttonText, buttons[i].posX + buttons[i].width / 2 - textWidth / 2,
                            buttons[i].posY + buttons[i].height / 2 - textHeight / 2);
    }

    SDL_RenderPresent(gRenderer);
  }

  void drawOver(Button *buttons, TTF_Text **texts)
  {
    SDL_SetRenderDrawColor(gRenderer, 0x22, 0x22, 0x11, 0xFF);
    SDL_RenderClear(gRenderer);


    float backWidth, backHeight;
    SDL_GetTextureSize(scoreBack, &backWidth, &backHeight);
    SDL_FRect backRect = {0, 0, backWidth, backHeight};
    for (int i = 0; i < WIDTH; i+= backWidth)
      for (int j = 0; j < HEIGHT; j+= backHeight)
      {
        backRect.x = i;
        backRect.y = j;
        SDL_RenderTexture(gRenderer, overBack, NULL, &backRect);
      }

    SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
    int textHeight, textWidth;
    TTF_GetTextSize(texts[0], &textWidth, &textHeight);
    TTF_DrawRendererText(texts[0], WIDTH / 2 - textWidth / 2, HEIGHT / 4 - textHeight / 2);
    TTF_GetTextSize(texts[1], &textWidth, &textHeight);
    TTF_DrawRendererText(texts[1], WIDTH / 2 - textWidth / 2, 3 * HEIGHT / 4 - textHeight / 2);
    TTF_SetFontSize(kenVectorFont, HEIGHT / 50);
    TTF_GetTextSize(texts[2], &textWidth, &textHeight);
    TTF_DrawRendererText(texts[2], WIDTH / 2 - textWidth / 2, 7 * HEIGHT / 8 - textHeight / 2);
    TTF_SetFontSize(kenVectorFont, 3 * HEIGHT / 50);

    char buttonsText[2][10] = {"Menu", "Replay"};
    for (uint8 i = 0; i < 2; i++) // Number of Buttons
    {
      TTF_Text *buttonText = TTF_CreateText(gTextEngine, kenVectorFont, buttonsText[i], strlen(buttonsText[i]));
      TTF_GetTextSize(buttonText, &textWidth, &textHeight);
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
      TTF_DrawRendererText(buttonText, buttons[i].posX + buttons[i].width / 2 - textWidth / 2,
                            buttons[i].posY + buttons[i].height / 2 - textHeight / 2);
    }

    SDL_RenderPresent(gRenderer);
  }

  void drawScores(Button *buttons, TTF_Text **texts, ScoreObj *scores)
  {
    SDL_SetRenderDrawColor(gRenderer, 0x22, 0x22, 0x11, 0xFF);
    SDL_RenderClear(gRenderer);

    float backWidth, backHeight;
    SDL_GetTextureSize(scoreBack, &backWidth, &backHeight);
    SDL_FRect backRect = {0, 0, backWidth, backHeight};
    for (int i = 0; i < WIDTH; i+= backWidth)
      for (int j = 0; j < HEIGHT; j+= backHeight)
      {
        backRect.x = i;
        backRect.y = j;
        SDL_RenderTexture(gRenderer, scoreBack, NULL, &backRect);
      }

    SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
    if (texts[3] != NULL)
    {
      int textHeight, textWidth;
      TTF_SetFontSize(kenVectorFont, HEIGHT / 50);
      TTF_GetTextSize(texts[0], &textWidth, &textHeight);
      TTF_DrawRendererText(texts[0], WIDTH / 2 - textWidth / 2, 3 *HEIGHT / 4 - textHeight / 2);
      TTF_SetFontSize(kenVectorFont, 3 * HEIGHT / 50);
      TTF_GetTextSize(texts[1], &textWidth, &textHeight);
      TTF_DrawRendererText(texts[1], WIDTH / 2 - textWidth / 2, 2 * HEIGHT / 4 - textHeight / 2);
    }
    else
    {
      TTF_SetFontSize(kenVectorFont, HEIGHT/50);
      for (uint8 i = 0; i < 2; i++) // Number of Buttons
      {
        int textHeight, textWidth;
        TTF_GetTextSize(buttons[i].text, &textWidth, &textHeight);
        if (buttons[i].hovered)
        {
          SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
          TTF_SetTextColor(buttons[i].text, 255, 255, 255, 255);
        }
        else
        {
          SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255);
          TTF_SetTextColor(buttons[i].text, 0, 0, 0, 255);
        }

        SDL_RenderFillRect(gRenderer, &buttons[i].rect);
        TTF_DrawRendererText(buttons[i].text, buttons[i].posX + buttons[i].width / 2 - textWidth / 2,
                              buttons[i].posY + buttons[i].height / 2 - textHeight / 2);
      }

      int textWidth, textHeight;
      TTF_GetTextSize(texts[2], &textWidth, &textHeight);
      TTF_DrawRendererText(texts[2], WIDTH/2 - textWidth/2, 50 - textHeight/2);
      TTF_SetFontSize(kenVectorFont, 3 * HEIGHT / 50);
      
      char *tempStr;
      TTF_Text *tempText = NULL;
      
      SDL_asprintf(&tempStr, "Username");
      tempText = TTF_CreateText(gTextEngine, kenVectorFont, tempStr, 0);
      TTF_GetTextSize(tempText, &textWidth, &textHeight);
      TTF_DrawRendererText(tempText, 10, (2 * HEIGHT / 11) - (textHeight / 2));
      SDL_asprintf(&tempStr, "Asteroids Destroyed");
      tempText = TTF_CreateText(gTextEngine, kenVectorFont, tempStr, 0);
      TTF_GetTextSize(tempText, &textWidth, &textHeight);
      TTF_DrawRendererText(tempText, WIDTH/2 - textWidth/2, (2 * HEIGHT / 11) - (textHeight / 2));
      SDL_asprintf(&tempStr, "Time Survived");
      tempText = TTF_CreateText(gTextEngine, kenVectorFont, tempStr, 0);
      TTF_GetTextSize(tempText, &textWidth, &textHeight);
      TTF_DrawRendererText(tempText, WIDTH - (10 + textWidth), (2 * HEIGHT / 11) - (textHeight / 2));
      
      for (int i = 0; i < 8; i++)
      {
        if (scores[i].username[0] != '\0')
        {
          SDL_asprintf(&tempStr, scores[i].username);
          tempText = TTF_CreateText(gTextEngine, kenVectorFont, tempStr, 0);
          TTF_GetTextSize(tempText, &textWidth, &textHeight);
          TTF_DrawRendererText(tempText, 10, ((i + 3)* HEIGHT / 11) - (textHeight / 2));
          SDL_asprintf(&tempStr, "%i", scores[i].score);
          tempText = TTF_CreateText(gTextEngine, kenVectorFont, tempStr, 0);
          TTF_GetTextSize(tempText, &textWidth, &textHeight);
          TTF_DrawRendererText(tempText, WIDTH/2 - textWidth/2, ((i + 3) * HEIGHT / 11) - (textHeight / 2));
          SDL_asprintf(&tempStr, "%i", scores[i].time);
          tempText = TTF_CreateText(gTextEngine, kenVectorFont, tempStr, 0);
          TTF_GetTextSize(tempText, &textWidth, &textHeight);
          TTF_DrawRendererText(tempText, WIDTH - (10 + textWidth), ((i + 3) * HEIGHT / 11) - (textHeight / 2));
        }
      }
    }


    SDL_RenderPresent(gRenderer);
  }

  void quit(void)
  {
    SDL_DestroyWindow(gWindow);
    SDL_DestroyRenderer(gRenderer);
    SDL_Quit();
  }

int main(int argc, char *args[])
{
  bool run = false;
  if (init()) /* Initialize */
    if (load(NULL)) /* Load Assets */
      run = true;

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
      playButton.posX = (WIDTH - playButton.width) / 2;
      playButton.posY = 1 * HEIGHT / 4 - playButton.height / 2;
      playButton.rect.w = playButton.width;
      playButton.rect.h = playButton.height;
      playButton.rect.x = playButton.posX;
      playButton.rect.y = playButton.posY;

      Button scoreButton; /* Scores */
      scoreButton.clicked = 0;
      scoreButton.hovered = 0;
      scoreButton.width = 200;
      scoreButton.height = 50;
      scoreButton.posX = (WIDTH - scoreButton.width) / 2;
      scoreButton.posY = 2 * HEIGHT / 4 - scoreButton.height / 2;
      scoreButton.rect.w = scoreButton.width;
      scoreButton.rect.h = scoreButton.height;
      scoreButton.rect.x = scoreButton.posX;
      scoreButton.rect.y = scoreButton.posY;

      Button quitButton; /* Quit */
      quitButton.clicked = 0;
      quitButton.hovered = 0;
      quitButton.width = 200;
      quitButton.height = 50;
      quitButton.posX = (WIDTH - quitButton.width) / 2;
      quitButton.posY = 3 * HEIGHT / 4 - quitButton.height / 2;
      quitButton.rect.w = quitButton.width;
      quitButton.rect.h = quitButton.height;
      quitButton.rect.x = quitButton.posX;
      quitButton.rect.y = quitButton.posY;

      buttons[0] = playButton;
      buttons[1] = scoreButton;
      buttons[2] = quitButton;

      float f1PosX = 0;
      float f1Speed = 10;
      float f2PosX = 0;
      float f2Speed = 8;

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

        else if (buttons[1].clicked == true)
        {
          gameState = SCORES;
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

        f1PosX += f1Speed;
        if (f1PosX > WIDTH) f1PosX = 0;
        f2PosX += f2Speed;
        if (f2PosX > WIDTH) f2PosX = 0;

        drawMenu(buttons, f1PosX, f2PosX);
      }
    }

    else if (gameState == GAME) /* Gameplay */
    {
      /* Initializing Game Objects */
      /* Delta Timer */
      DeltaTimer dTimer;

      dTimer.frameStart = SDL_GetPerformanceCounter();
      dTimer.frameEnd = 0;
      dTimer.delta = 0;

      uint64 frameStart;
      uint64 frameEnd;
      float fps;

      /* Player */
      Player player;
      playerInit(&player);

      /* Asteroids */
      AsteroidList asteroids = asteroidInit();

      Timer astSpawnTimer;
      int astSpawnCount = 0;
      int astSpawnTime = 5000;
      timerStart(&astSpawnTimer);

      PowerUpList powerUps = powerUpInit();

      load(&player);

      SDL_Event e;
      bool exited = false;
      bool replay = false;

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

        frameStart = SDL_GetPerformanceCounter();

        deltaCalc(&dTimer);
        timerCalcTicks(&player.gameTimer);

        playerMovementHandler(&player, dTimer.delta);
        playerBulletHander(&player, dTimer.delta);
        playerPowerUpHandler(&player, &powerUps);
        asteroidHandler(&asteroids, &powerUps, &player, &astSpawnTimer, &astSpawnCount, astSpawnTime, dTimer.delta);
        if (astSpawnCount > 10 && astSpawnCount > 500)
          astSpawnTime -= 100;


        if (player.armor < 0)
        {
          gameState = OVER;
        }

        if (gameState == OVER)
        {
          TTF_SetFontSize(kenVectorFont, 3 * HEIGHT / 50);
          char *tempText;
          SDL_asprintf(&tempText, "You destroyed %li asteroids \nin %i minutes and %i seconds", player.score, (player.gameTimer.ticks / 1000) / 60, (player.gameTimer.ticks / 1000) % 60);
          TTF_Text *texts[3] = {};
          texts[0] = TTF_CreateText(gTextEngine, kenVectorFont, tempText, 0);
          tempText = "Type the username. Press Escape to not save the score. Press Enter to save the score.\n(Empty Username won't be stored!)";
          texts[2] = TTF_CreateText(gTextEngine, kenVectorFont, tempText, 0);

          Button buttons[2];

          buttons[0].clicked = 0;
          buttons[0].width = 250;
          buttons[0].height = 100;
          buttons[0].hovered = 0;
          buttons[0].posX = WIDTH / 3 - buttons[0].width / 2;
          buttons[0].posY = 2 * HEIGHT / 4;
          buttons[0].rect.x = buttons[0].posX;
          buttons[0].rect.y = buttons[0].posY;
          buttons[0].rect.w = buttons[0].width;
          buttons[0].rect.h = buttons[0].height;

          buttons[1].clicked = 0;
          buttons[1].width = 250;
          buttons[1].height = 100;
          buttons[1].hovered = 0;
          buttons[1].posX = 2 * WIDTH / 3 - buttons[1].width / 2;
          buttons[1].posY = 2 * HEIGHT / 4;
          buttons[1].rect.x = buttons[1].posX;
          buttons[1].rect.y = buttons[1].posY;
          buttons[1].rect.w = buttons[1].width;
          buttons[1].rect.h = buttons[1].height;

          bool textInput = true;
          SDL_Rect textArea = {WIDTH / 2, 3 * HEIGHT / 4, 200, 60};
          char playerName[50] = {};
          int nameCursor = 0;
          SDL_SetTextInputArea(gWindow, NULL, 0);
          SDL_StartTextInput(gWindow);

          SDL_Event e;
          bool next = false;
          while (!next)
          {
            while (SDL_PollEvent(&e) != 0)
            {
              if (e.type == SDL_EVENT_QUIT)
              {
                next = true;
                run = false;
              }

              else if (e.type == SDL_EVENT_TEXT_INPUT && nameCursor < sizeof(playerName) - 1)
              {
                SDL_strlcat(playerName, e.text.text, sizeof(playerName));
                nameCursor = strlen(playerName);
              }

              else if (e.type == SDL_EVENT_KEY_DOWN && textInput)
              {
                if (e.key.key == SDLK_RETURN)
                {
                  SDL_StopTextInput(gWindow);
                  textInput = false;
                }
                else if (e.key.key == SDLK_ESCAPE)
                {
                  strcpy(playerName, "");
                  SDL_StopTextInput(gWindow);
                  textInput = false;
                }
                else if (e.key.key == SDLK_BACKSPACE && nameCursor > 0)
                {
                  nameCursor--;
                  playerName[nameCursor] = '\0';
                }
              }
            }
            texts[1] = TTF_CreateText(gTextEngine, kenVectorFont, playerName, 0);

            for (int i = 0; i < 2; i++)
            {
              buttonStateUpdater(&buttons[i]);
            }

            if (buttons[0].clicked)
              gameState = MENU;

            if (buttons[1].clicked)
            {
              replay = true;
              gameState = GAME;
            }

            if (gameState != OVER)
            {
              if (strcmp(playerName, "") != 0)
              {
                char *jsonData = extractScores("scores.json");
                jsonData = updateScores(jsonData, playerName, player.score, player.gameTimer.ticks / 1000);
                saveScores(jsonData, "scores.json");
              }
              
              TTF_SetFontSize(kenVectorFont, HEIGHT / 50);
              break;
            }

            drawOver(buttons, texts);
          }
        }
        else if (gameState == PAUSED)
        {
          Button buttons[2];

          Button resumeButton;
          resumeButton.clicked = 0;
          resumeButton.hovered = 0;
          resumeButton.width = 200;
          resumeButton.height = 50;
          resumeButton.posX = (WIDTH - resumeButton.width) / 2;
          resumeButton.posY = 1 * HEIGHT / 3.f - resumeButton.height / 2;
          resumeButton.rect.w = resumeButton.width;
          resumeButton.rect.h = resumeButton.height;
          resumeButton.rect.x = resumeButton.posX;
          resumeButton.rect.y = resumeButton.posY;

          Button menuButton;
          menuButton.clicked = 0;
          menuButton.hovered = 0;
          menuButton.width = 200;
          menuButton.height = 50;
          menuButton.posX = (WIDTH - menuButton.width) / 2;
          menuButton.posY = 2 * HEIGHT / 3.f - menuButton.height / 2;
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

            drawPaused(buttons);
          }
        }

        if (gameState != GAME || replay)
        {
          break;
        }

        char *fpsStr;
        SDL_asprintf(&fpsStr, "%f", 1 / fps);
        TTF_Text *fpsText = TTF_CreateText(gTextEngine, kenVectorFont, fpsStr, 0);

        drawGame(&player, &asteroids, &powerUps, fpsText); /* Draw, Blit and Render */

        frameEnd = SDL_GetPerformanceCounter();
        fps = (frameEnd - frameStart) / (float)SDL_GetPerformanceFrequency();
      }
    }
  
    else if (gameState == SCORES)
    {
      Button buttons[2];

      buttons[0].text = TTF_CreateText(gTextEngine, kenVectorFont, "Sort By Score", 0);
      buttons[0].clicked = 0;
      buttons[0].hovered = 0;
      buttons[0].width = 200;
      buttons[0].height = 50;
      buttons[0].posX = 10;
      buttons[0].posY = 1 * HEIGHT / 11 - buttons[0].height / 2;
      buttons[0].rect.w = buttons[0].width;
      buttons[0].rect.h = buttons[0].height;
      buttons[0].rect.x = buttons[0].posX;
      buttons[0].rect.y = buttons[0].posY;
      Timer buttonTimer = timerInit();
      timerStart(&buttonTimer);

      buttons[1].text = TTF_CreateText(gTextEngine, kenVectorFont, "Search", 0);
      buttons[1].clicked = 0;
      buttons[1].hovered = 0;
      buttons[1].width = 200;
      buttons[1].height = 50;
      buttons[1].posX = WIDTH - buttons[1].width - 10;
      buttons[1].posY = 1 * HEIGHT / 11 - buttons[1].height / 2;
      buttons[1].rect.w = buttons[1].width;
      buttons[1].rect.h = buttons[1].height;
      buttons[1].rect.x = buttons[1].posX;
      buttons[1].rect.y = buttons[1].posY;

      char *jsonData = extractScores("scores.json"); /* Grab Data */
      enum Sort sortType = SCORE;
      cJSON *root = NULL;
      cJSON *scoreArr = NULL;
      if (jsonData == NULL) /* Doesn't Exist -> Create New */
      {
        root = cJSON_CreateObject();
        scoreArr = cJSON_CreateArray();
        cJSON_AddItemToObject(root, "Scores", scoreArr);
      }
      else /* Exist -> Use Existing*/
      {
        root = cJSON_Parse(jsonData);
        sortScores(root, sortType);
        scoreArr = cJSON_GetObjectItem(root, "Scores");
      }

      free(jsonData);
      
      int arrSize = cJSON_GetArraySize(scoreArr);
      ScoreObj scores[8]; /* 8 Scores At A Time */
      int scoreCursor = 0;
      TTF_Text *texts[4] = {};

      char tempText[] = "Enter Username to get Scores. Leave empty to get all.";
      texts[0] = TTF_CreateText(gTextEngine, kenVectorFont, tempText, 0);
      texts[2] = TTF_CreateText(gTextEngine, kenVectorFont, "Press Esc to go back to menu", 0);
      texts[3] = NULL;

      char username[50] = {};
      int nameCursor = 0;
      bool textInput = false;

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

          else if (e.type == SDL_EVENT_TEXT_INPUT && nameCursor < sizeof(username) - 1)
          {
            SDL_strlcat(username, e.text.text, sizeof(username));
            nameCursor = strlen(username);
          }

          else if (e.type == SDL_EVENT_KEY_DOWN && textInput)
          {
            if (e.key.key == SDLK_RETURN || e.key.key == SDLK_ESCAPE)
            {
              if (e.key.key == SDLK_ESCAPE)
                strcpy(username, "");
              SDL_StopTextInput(gWindow);
              textInput = false;
              texts[1] = TTF_CreateText(gTextEngine, kenVectorFont, username, 0);
              texts[3] = NULL;
            }
            else if (e.key.key == SDLK_BACKSPACE && nameCursor > 0)
            {
              nameCursor--;
              username[nameCursor] = '\0';
            }
          }

          else if (e.type == SDL_EVENT_KEY_DOWN && e.key.repeat == 0)
          {
            if (e.key.key == SDLK_ESCAPE)
              gameState = MENU;
          }
        
          else if (e.type == SDL_EVENT_MOUSE_WHEEL && !textInput)
          {
            if (!(scoreCursor - (int) e.wheel.y < 0) && !(scoreCursor - (int) e.wheel.y > arrSize - 8 /* Number of Scores on Screen*/))
              scoreCursor -= (int) e.wheel.y;
          }
        }
        if (textInput)
          texts[1] = TTF_CreateText(gTextEngine, kenVectorFont, username, 0);
        else
        {
          for (int i = 0; i < 2; i++)
          {
            buttonStateUpdater(&buttons[i]);
          }

          if (buttons[0].clicked && buttonTimer.ticks > 200)
          {
            if (sortType == SCORE) /* Cycle through sort types */
            {
              sortType = TIME; 
              buttons[0].text = TTF_CreateText(gTextEngine, kenVectorFont, "Sort By Time", 0);
            }
            else if (sortType == TIME) 
            {
              sortType = NAME;
              buttons[0].text = TTF_CreateText(gTextEngine, kenVectorFont, "Sort By Name", 0);
            }
            else if (sortType == NAME) 
            {
              sortType = SCORE;
              buttons[0].text = TTF_CreateText(gTextEngine, kenVectorFont, "Sort By Score", 0);
            }
            sortScores(root, sortType);
            scoreArr = cJSON_GetObjectItem(root, "Scores"); /* Regrab array */
            timerReset(&buttonTimer);
          }

          if (buttons[1].clicked)
          {
            texts[3] = TTF_CreateText(gTextEngine, kenVectorFont, "!", 0);
            textInput = true;
            SDL_StartTextInput(gWindow);
          }

          if (username[0] == '\0')
          {
            for (int i = 0; i < 8; i++)
            {
              cJSON *jScoreObj = cJSON_GetArrayItem(scoreArr, scoreCursor + i);
              if (jScoreObj != NULL)
              {
                cJSON *jUsername = cJSON_GetObjectItem(jScoreObj, "Username");
                char *userName = jUsername->valuestring;
                cJSON *jScore = cJSON_GetObjectItem(jScoreObj, "Score");
                int score = jScore->valueint;
                cJSON *jTime = cJSON_GetObjectItem(jScoreObj, "Time");
                int time = jTime->valueint;
                strcpy(scores[i].username, userName);
                scores[i].score = score;
                scores[i].time = time;
              }
              else
              {
                strcpy(scores[i].username, "");
                scores[i].score = 0;
                scores[i].time = 0;
              }
            }
          }
          else
          {
            cJSON *userArr = cJSON_CreateArray();
            cJSON *userScore = NULL;
            int arrSize = cJSON_GetArraySize(scoreArr);
            for (int i = 0; i < arrSize; i++)
            {
              userScore = cJSON_GetArrayItem(scoreArr, i);
              if (strncmp(cJSON_GetObjectItem(userScore, "Username")->valuestring, username, 50) == 0)
                cJSON_AddItemReferenceToArray(userArr, userScore);
            }
            

            for (int i = 0; i < 8; i++)
            {
              cJSON *jScoreObj = cJSON_GetArrayItem(userArr, scoreCursor + i);
              if (jScoreObj != NULL)
              {
                cJSON *jUsername = cJSON_GetObjectItem(jScoreObj, "Username");
                char *userName = jUsername->valuestring;
                cJSON *jScore = cJSON_GetObjectItem(jScoreObj, "Score");
                int score = jScore->valueint;
                cJSON *jTime = cJSON_GetObjectItem(jScoreObj, "Time");
                int time = jTime->valueint;
                strcpy(scores[i].username, userName);
                scores[i].score = score;
                scores[i].time = time;
              }
              else
              {
                strcpy(scores[i].username, "");
                scores[i].score = 0;
                scores[i].time = 0;
              }
            }
          }
        }

        if (gameState != SCORES)
        {
          TTF_SetFontSize(kenVectorFont, HEIGHT/50);
          break;
        }

        timerCalcTicks(&buttonTimer);
        drawScores(buttons, texts, scores);
      }
    }
  }

  quit(); // Quit
  printf("=== End Of Program ===\n");
  return 0;
}
