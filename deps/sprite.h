#ifndef SPRITE_H_
#define SPRITE_H_

#include "includes.h"

typedef struct Sprite {
  char name[50];
  int width, height, x, y;
} Sprite;

SDL_FRect getSpriteRect(Sprite *spriteList, const char *spriteName); 
bool parseXML(const char *fileName, Sprite *spriteList); 

#endif //SPRITE_H_
