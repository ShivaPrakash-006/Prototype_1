#ifndef DRAW_H_
#define DRAW_H_

#include "includes.h"
#include "player.h"
#include "button.h"
#include "asteroid.h"
#include "powerup.h"
#include "score.h"

void drawMenu(SDL_Renderer *gRenderer, TTF_TextEngine *gTextEngine, Button *buttons, float f1PosX, float f2PosX, SDL_Texture *menuBack1, SDL_Texture *menuBack2, TTF_Font *kenVectorFont); 
void drawGame(SDL_Renderer *gRenderer, TTF_TextEngine *gTextEngine, Player *player, AsteroidNode *asteroids, PowerUpNode *powerUp, TTF_Text *fpsText, SDL_Texture *gameBack, SDL_Texture *spriteSheet, Sprite *spriteList, TTF_Font *kenVectorFont);
void drawPaused(SDL_Renderer *gRenderer, TTF_TextEngine *gTextEngine, Button *buttons, SDL_Texture *pauseBack, TTF_Font *kenVectorFont); 
void drawOver(SDL_Renderer *gRenderer, TTF_TextEngine *gTextEngine, Button *buttons, TTF_Text **texts, SDL_Texture *overBack, TTF_Font *kenVectorFont); 
void drawScores(SDL_Renderer *gRenderer, TTF_TextEngine *gTextEngine, Button *buttons, TTF_Text **texts, ScoreObj *scores, SDL_Texture *scoreBack, TTF_Font *kenVectorFont); 

#endif //DRAW_H_
