#ifndef ASTEROID_H_
#define ASTEROID_H_

#include "includes.h"

typedef struct Sprite Sprite;
typedef struct PowerUpNode PowerUpNode;
typedef struct Player Player;
typedef struct Timer Timer;

typedef struct Asteroid {
  enum Size size;
  int color;

  short width;  /* 20 */
  short height; /* 20 */

  float posX;
  float posY;
  float rot;

  float velX;
  float velY;
  float rotVel;

  float speed;

  uint32 astNum;

  SDL_FRect rect;
  SDL_FRect spriteRect;
} Asteroid;

typedef struct AsteroidNode {
  Asteroid asteroid;
  struct AsteroidNode *nextAsteroid;
} AsteroidNode;

AsteroidNode asteroidInit(void);
void asteroidSpawn(AsteroidNode *asteroids, Asteroid *refAsteroid, Sprite *spriteList); 
void asteroidDestroy(Asteroid *asteroid, AsteroidNode *asteroids, Sprite *spriteList, Mix_Chunk *astDestroySfx); 
void asteroidHandler(AsteroidNode *asteroids, PowerUpNode *powerUps,
                     Player *player, Timer *spawnTimer, int *spawnCount,
                     int spawnTime, double delta, Sprite *spriteList, Mix_Chunk *astDestroySfx); 

#endif //ASTEROID_H_
