#include "powerup.h"
#include "includes.h"
#include "objects.h"

PowerUpNode powerUpInit(void) {
  PowerUpNode powerUps;

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

void powerUpDestroy(PowerUp *powerUp, PowerUpNode *powerUps) {
  PowerUpNode *powerPtr, *prevPtr;
  powerPtr = powerUps->nextPowerUp;
  prevPtr = powerUps;

  while (powerPtr != NULL) {
    if (powerUp->powerNum == powerPtr->power.powerNum) {
      prevPtr->nextPowerUp = powerPtr->nextPowerUp;
      free(powerPtr);
      break;
    }
    prevPtr = powerPtr;
    powerPtr = powerPtr->nextPowerUp;
  }
}

void powerUpSpawn(Asteroid *asteroid, PowerUpNode *powerUps) {
  PowerUp power;
  power.width = 32;
  power.height = 32;
  power.posX = asteroid->posX + asteroid->width / 2.f;
  power.posY = asteroid->posY + asteroid->height / 2.f;
  power.rect.w = power.width;
  power.rect.h = power.height;
  power.rect.x = power.posX;
  power.rect.y = power.posY;
  power.powerUp = (enum Power)SDL_rand(4);

  PowerUpNode *powerPtr, *prevPtr, *newPower;
  powerPtr = powerUps->nextPowerUp;
  prevPtr = powerUps;
  newPower = (PowerUpNode *)malloc(sizeof(PowerUpNode));
  uint32 powerNum = 1;

  while (powerPtr != NULL) {
    if (powerPtr->power.powerNum == powerNum) {
      prevPtr = powerPtr;
      powerPtr = powerPtr->nextPowerUp;
      powerNum++;
    } else {
      break;
    }
  }

  newPower->power = power;
  newPower->power.powerNum = powerNum;
  newPower->nextPowerUp = powerPtr;
  prevPtr->nextPowerUp = newPower;
}
