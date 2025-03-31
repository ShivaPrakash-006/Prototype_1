#ifndef PLAYER_H_
#define PLAYER_H_

#include "includes.h"
#include "timer.h"
#include "powerup.h"
#include "bullet.h"

typedef struct Player {
  short armor; /* 3 */
  TTF_Text *armorText;

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

  BulletNode bullets;

  Timer bulletTimer;
  uint32 shootDelay; /* 200 */

  uint64 score;
  TTF_Text *scoreText;

  bool shield;
  bool repair;
  bool infFuel;
  bool multiBullet;

  Timer shieldTimer;
  uint32 shieldTime;

  Timer shieldBlinker;
  bool shieldBlink;

  Timer infFuelTimer;
  uint32 infFuelTime;

  Timer multiBullTimer;
  uint32 multiBullTime;

  Timer gameTimer;
} Player;

void playerInit(Player *player); 
void playerShoot(Player *player, int num); 
void playerBulletHander(Player *player, double delta, Mix_Chunk *shootSfx); 
void playerEventHandler(SDL_Event e, Player *player, enum State *gameState); 
void playerMovementHandler(Player *player, double delta); 
void playerTextHandler(Player *player, TTF_TextEngine *gTextEngine,
                       TTF_Font *kenVectorFont);
void playerPowerUpHandler(Player *player, PowerUpNode *powerUps, SDL_Texture *spriteSheet, Sprite *spriteList, Mix_Chunk *shieldUpSfx, Mix_Chunk *shieldDownSfx); 
void playerRender(Player *player, SDL_Renderer *gRenderer, SDL_Texture *spriteSheet, Sprite *spriteList); 
void playerDestroy(Player *player); 

#endif //PLAYER_H_
