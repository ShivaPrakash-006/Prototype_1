#ifndef INIT_H_
#define INIT_H_

#include "player.h"

bool init(SDL_Window **gWindow, SDL_Renderer **gRenderer,
          TTF_TextEngine **gTextEngine, TTF_Font **kenVectorFont); 
bool load(SDL_Renderer *gRenderer, Sprite *spriteList,
          SDL_Texture **menuBack1, SDL_Texture **menuBack2, SDL_Texture **gameBack,
          SDL_Texture **pauseBack, SDL_Texture **overBack, SDL_Texture **scoreBack,
          SDL_Texture **spriteSheet, Mix_Music **bgMusic, Mix_Music **battleMusic,
          Mix_Chunk **shootSfx, Mix_Chunk **shieldUpSfx, Mix_Chunk **shieldDownSfx,
          Mix_Chunk **astDestroySfx, Mix_Chunk **loseSfx,
          Mix_Chunk **selectSfx);
bool loadPlayer(SDL_Renderer *gRenderer, Player *player);

#endif //INIT_H_
