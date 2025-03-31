#ifndef BULLET_H_
#define BULLET_H_
#include "includes.h"
#include "timer.h"

typedef struct Bullet {
  int width;  /* 10 */
  int height; /* 10 */

  float posX;
  float posY;
  float rot;

  int damage; /* 1 */

  float speed; /* 1000 */

  float velX;
  float velY;

  SDL_FRect rect;
  SDL_FPoint center;
  uint32 bulletNum;

  uint32 lifeTime;
  Timer lifeTimer;

} Bullet;

/* Linked List */
typedef struct BulletNode {
  Bullet bullet;
  struct BulletNode *nextBullet;

} BulletNode;

void bulletDestroy(Bullet *bullet, BulletNode *bullets); 

#endif //BULLET_H_
