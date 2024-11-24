#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#define PI 3.14159265359
const int WIDTH = 1280;
const int HEIGHT = 720;

SDL_Window *gWindow = NULL;
SDL_Renderer *gRenderer = NULL;

/* Delta Timer Definitions */
/* Struct */
typedef struct
{
  Uint64 frameStart;
  Uint64 frameEnd;

  double delta;
  double deltaInSecs;

} DeltaTimer;

/* Functions */
double deltaCalc(DeltaTimer *dTimer)
{
  (*dTimer).frameEnd = (*dTimer).frameStart;
  (*dTimer).frameStart = SDL_GetPerformanceCounter();

  (*dTimer).delta = (((*dTimer).frameStart - (*dTimer).frameEnd) * 1000) / (double)SDL_GetPerformanceFrequency();
  (*dTimer).deltaInSecs = (*dTimer).delta / 1000;
}

/* Player Definitions*/
/* Struct */
typedef struct
{
  int width;  /* 10 */
  int height; /* 10 */

  float posX;
  float posY;
  float rot;

  float maxMoveSpeed; /* 17.5 */
  float minMoveSpeed; /* 0.1 */
  float currentMoveSpeed;
  float maxRotSpeed; /* 0.15 */
  float minRotSpeed; /* 0.01 */
  float currentRotSpeed;

  _Bool moving;
  float vel;
  float velX;
  float velY;
  float rotVel;
  float velMulti;    /* Velocity added/multiplied when moving */
  float velDecay;    /* Velocity decay when not moving */
  float rotVelMulti; /* rotVelocity added/multiplied when rotating */
  float rotVelDecay; /* rotVelocity decay when not rotating */

  SDL_FRect rect;

  SDL_Texture *icon;

} Player;

/* Functions */
void playerLogs(Player player)
{
  printf("posX: %f\n", player.posX);
  printf("posY: %f\n", player.posY);
  printf("rot: %f\n", player.rot);
  printf("rotVel: %f\n", player.rotVel);
  printf("vel: %f\n", player.vel);
  printf("velX: %f\n", player.velX);
  printf("velY: %f\n", player.velY);
  printf("currentMoveSpeed: %f\n", player.currentMoveSpeed);
  printf("currentRotSpeed: %f\n", player.currentRotSpeed);
  printf("moving: %i\n", player.moving);
}

void playerEventHandler(SDL_Event e, Player *player)
{
  if (e.type == SDL_EVENT_KEY_DOWN & e.key.repeat == 0)
  {
    switch (e.key.key)
    {
    case SDLK_W:
      (*player).moving = true;
      break;

    case SDLK_A:
      (*player).rotVel -= 1;
      break;

    case SDLK_D:
      (*player).rotVel += 1;
      break;

    case SDLK_L:
      playerLogs((*player));
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
      (*player).moving = false;
      break;

    case SDLK_D:
      (*player).rotVel -= 1;
      break;

    case SDLK_A:
      (*player).rotVel += 1;
      break;

    default:
      break;
    }
  }
}

void playerMovementHandler(Player *player, double delta)
{
  /* currentRotSpeed calculation */
  if ((*player).rotVel > 0) /* Holding Right */
  {
    

    if ((*player).currentRotSpeed != 0) /* Not At Rest */
    {
      if ((*player).currentRotSpeed + (*player).currentRotSpeed * (*player).rotVelMulti <= (*player).maxRotSpeed & (*player).currentRotSpeed > 0)
      {
        (*player).currentRotSpeed += (*player).currentRotSpeed * (*player).rotVelMulti; /* Adding rotSpeed */
      }
      else if ((*player).currentRotSpeed - (*player).currentRotSpeed * (*player).rotVelMulti <= (*player).maxRotSpeed & (*player).currentRotSpeed < 0)
      {
        (*player).currentRotSpeed -= (*player).currentRotSpeed * (*player).rotVelMulti; /* Subtracting rotSpeed */
      }
      else if ((*player).currentRotSpeed + (*player).currentRotSpeed * (*player).rotVelMulti >= (*player).maxRotSpeed)
      {
        (*player).currentRotSpeed = (*player).maxRotSpeed; /* Limiting To maxRotSpeed */
      }

      if ((*player).currentRotSpeed < (*player).minRotSpeed & (*player).currentRotSpeed > -(*player).minRotSpeed)
      {
        (*player).currentRotSpeed = 0; /* If small enough, make it 0 */
      }

    }
    else
    {
      (*player).currentRotSpeed = (*player).minRotSpeed; /* At Rest */
    }
  }
  else if ((*player).rotVel < 0) /* Holding Left */
  {
    if ((*player).currentRotSpeed != 0) /* Not At Rest */
    {
      if ((*player).currentRotSpeed - (*player).currentRotSpeed * (*player).rotVelMulti >= -((*player).maxRotSpeed) & (*player).currentRotSpeed < 0)
      {
        (*player).currentRotSpeed += (*player).currentRotSpeed * (*player).rotVelMulti;
      }
      else if ((*player).currentRotSpeed - (*player).currentRotSpeed * (*player).rotVelMulti >= -((*player).maxRotSpeed) & (*player).currentRotSpeed > 0)
      {
        (*player).currentRotSpeed -= (*player).currentRotSpeed * (*player).rotVelMulti;
      }
      else if ((*player).currentRotSpeed - (*player).currentRotSpeed * (*player).rotVelMulti <= -((*player).maxRotSpeed))
      {
        (*player).currentRotSpeed = -((*player).maxRotSpeed);
      }

      if ((*player).currentRotSpeed < (*player).minRotSpeed & (*player).currentRotSpeed > -(*player).minRotSpeed)
      {
        (*player).currentRotSpeed = 0; /* If small enough, make it 0 */
      }
    }
    else
    {
      (*player).currentRotSpeed = -(*player).minRotSpeed; /* At rest */
    }
  }
  else if ((*player).rotVel == 0 && (*player).currentRotSpeed != 0) /* Totally At Rest */
  {
    (*player).currentRotSpeed *= (*player).rotVelDecay; /* Reduce rotVel */

    if ((*player).currentRotSpeed < (*player).minRotSpeed & (*player).currentRotSpeed > -(*player).minRotSpeed)
    {
      (*player).currentRotSpeed = 0; /* If small enough, make it 0 (works better than i expected lol)*/
    }
  }

  /* Rotating the ship */
  if ((*player).rot + (*player).currentRotSpeed >= 360) /* Cycle from 360 to 0 */
  {
    (*player).rot = (*player).rot + (*player).currentRotSpeed - 360;
  }
  else
  {
    (*player).rot += (*player).currentRotSpeed * ( delta * 1000 ); /* delta * 1000 -> Normalization */
  }

  if ((*player).rot + (*player).currentRotSpeed <= 0) /* Cycle from 0 to 360 */
  {
    (*player).rot = 360 + (*player).rot + (*player).currentRotSpeed;
  }
  else
  {
    (*player).rot += (*player).currentRotSpeed * ( delta * 1000 );
  }

  

  /* Calculating currentMoveSpeed */
  if ((*player).moving)
  {
    if ((*player).currentMoveSpeed != 0)
    {
      if ((*player).currentMoveSpeed + (*player).currentMoveSpeed * (*player).velMulti < (*player).maxMoveSpeed)
      {
        (*player).currentMoveSpeed += (*player).currentMoveSpeed * (*player).velMulti;
      }
      else if ((*player).currentMoveSpeed + (*player).currentMoveSpeed * (*player).velMulti >= (*player).maxMoveSpeed)
      {
        (*player).currentMoveSpeed = (*player).maxMoveSpeed;
      }
    }
    else
    {
      (*player).currentMoveSpeed = (*player).minMoveSpeed;
    }
  }
  else
  {
    if ((*player).currentMoveSpeed >= (*player).minMoveSpeed)
    {
      (*player).currentMoveSpeed *= (*player).velDecay;
    }
    else
    {
      (*player).currentMoveSpeed = 0;
    }
  }
  
  if ((*player).moving) /* velX and velY should only update when moving */
  {
    /* Calculating force on X Axis and Y Axis through trigonometry */
    (*player).velX = SDL_sinf((*player).rot * PI / 180); /* rot * PI / 180 to convert to radians */
    (*player).velY = -SDL_cosf((*player).rot * PI / 180); /* Due to Y coordinates of SDL, a minus is required */
  }

  (*player).posX += (*player).currentMoveSpeed * (delta * 10) * (*player).velX;
  (*player).posY += (*player).currentMoveSpeed * (delta * 10) * (*player).velY;

  (*player).rect.x = (*player).posX;
  (*player).rect.y = (*player).posY;
}

void renderPlayer(Player player)
{
  SDL_FRect destRect;
  destRect.x = player.rect.x;
  destRect.y = player.rect.y;
  destRect.w = 100;
  destRect.h = 100;
  SDL_RenderTextureRotated(gRenderer, player.icon, NULL, &destRect, player.rot, NULL, SDL_FLIP_NONE);
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

  (*player).icon = SDL_CreateTextureFromSurface(gRenderer, IMG_Load("Crystal.png"));
  if ((*player).icon == NULL)
  {
    printf("'Crystal.png' could not be loaded! SDL_image Error: %s\n", IMG_GetError());
  }

  return success;
}

void draw(Player player)
{
  SDL_SetRenderDrawColor(gRenderer, 0x22, 0x22, 0x11, 0xFF);
  SDL_RenderClear(gRenderer);

  SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

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

  player.width = 25;
  player.height = 25;

  player.posX = 10;
  player.posY = 10;
  player.rot = 90;

  player.maxMoveSpeed = 17.5;
  player.minMoveSpeed = 0.1;
  player.maxRotSpeed = 0.15;
  player.minRotSpeed = 0.01;

  player.currentMoveSpeed = 0;
  player.currentRotSpeed = 0;
  player.vel = 0;
  player.rotVel = 0;
  player.moving = false;

  player.velMulti = 0.009;
  player.velDecay = 0.999;

  player.rotVelMulti = 0.005;
  player.rotVelDecay = 0.999;

  player.velX = 0;
  player.velY = 0;

  player.rect.w = player.width;
  player.rect.h = player.height;
  player.rect.x = player.posX;
  player.rect.y = player.posY;

  player.icon = NULL;

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

    draw(player); /* Draw, Blit and Render */
  }

  quit(); // Quit
  return 0;
}
