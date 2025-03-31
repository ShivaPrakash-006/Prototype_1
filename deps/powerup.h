#ifndef POWERUP_H_
#define POWERUP_H_

#include "includes.h"
#include "asteroid.h"

typedef struct PowerUp {
  float width;
  float height;
  float posX;
  float posY;

  uint32 powerNum;

  SDL_FRect rect;

  enum Power powerUp;
} PowerUp;

typedef struct PowerUpNode {

  PowerUp power;
  struct PowerUpNode *nextPowerUp;

} PowerUpNode;

PowerUpNode powerUpInit(void);
void powerUpDestroy(PowerUp *powerUp, PowerUpNode *powerUps); 
void powerUpSpawn(Asteroid *asteroid, PowerUpNode *powerUps); 

#endif //POWERUP_H_
