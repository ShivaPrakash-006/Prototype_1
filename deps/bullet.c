#include "bullet.h"
#include "includes.h"
#include "objects.h"

void bulletDestroy(Bullet *bullet, BulletNode *bullets) {
  BulletNode *bulletPtr, *prevPtr;
  prevPtr = bullets;
  bulletPtr = bullets->nextBullet;
  while (bulletPtr != NULL) {
    if (bulletPtr->bullet.bulletNum == bullet->bulletNum) {
      prevPtr->nextBullet = bulletPtr->nextBullet;
      free(bulletPtr);
      break;
    }
    prevPtr = bulletPtr;
    bulletPtr = bulletPtr->nextBullet;
  }
}
