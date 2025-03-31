#include "draw.h"
#include "includes.h"
#include "objects.h"

void drawMenu(SDL_Renderer *gRenderer, TTF_TextEngine *gTextEngine,
              Button *buttons, float f1PosX, float f2PosX,
              SDL_Texture *menuBack1, SDL_Texture *menuBack2,
              TTF_Font *kenVectorFont) {
  char buttonsText[3][10] = {"Play", "Scores", "Quit"};
  SDL_SetRenderDrawColor(gRenderer, 0x06, 0x12, 0x21, 0xFF);
  SDL_RenderClear(gRenderer);

  SDL_FRect tempRect = {f1PosX, 0.f, WIDTH,
                        HEIGHT}; // Every Image has Same Dimension
  SDL_RenderTexture(gRenderer, menuBack1, NULL, &tempRect);
  tempRect.x = f1PosX - WIDTH;
  SDL_RenderTexture(gRenderer, menuBack1, NULL, &tempRect);
  tempRect.x = f2PosX;
  SDL_RenderTexture(gRenderer, menuBack2, NULL, &tempRect);
  tempRect.x = f2PosX - WIDTH;
  SDL_RenderTexture(gRenderer, menuBack2, NULL, &tempRect);

  for (uint8 i = 0; i < 3; i++) // Number of Buttons
  {
    TTF_Text *buttonText = TTF_CreateText(
        gTextEngine, kenVectorFont, buttonsText[i], strlen(buttonsText[i]));
    int textHeight, textWidth;
    TTF_GetTextSize(buttonText, &textWidth, &textHeight);
    if (buttons[i].hovered) {
      SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
      TTF_SetTextColor(buttonText, 255, 255, 255, 255);
    } else {
      SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255);
      TTF_SetTextColor(buttonText, 0, 0, 0, 255);
    }

    SDL_RenderFillRect(gRenderer, &buttons[i].rect);
    TTF_DrawRendererText(
        buttonText, buttons[i].posX + buttons[i].width / 2.f - textWidth / 2.f,
        buttons[i].posY + buttons[i].height / 2.f - textHeight / 2.f);
  }

  SDL_RenderPresent(gRenderer);
}

void drawGame(SDL_Renderer *gRenderer, TTF_TextEngine *gTextEngine,
              Player *player, AsteroidNode *asteroids, PowerUpNode *powerUps,
              TTF_Text *fpsText, SDL_Texture *gameBack,
              SDL_Texture *spriteSheet, Sprite *spriteList,
              TTF_Font *kenVectorFont) {
  SDL_SetRenderDrawColor(gRenderer, 0x22, 0x22, 0x11, 0xFF);
  SDL_RenderClear(gRenderer);

  float backWidth, backHeight;
  SDL_GetTextureSize(gameBack, &backWidth, &backHeight);
  SDL_FRect backRect = {0, 0, backWidth, backHeight};
  for (int i = 0; i < WIDTH; i += backWidth)
    for (int j = 0; j < HEIGHT; j += backHeight) {
      backRect.x = i;
      backRect.y = j;
      SDL_RenderTexture(gRenderer, gameBack, NULL, &backRect);
    }

  SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

  SDL_FRect bulletSpriteRect = getSpriteRect(spriteList, "laserRed16.png");
  BulletNode *bullPtr = &player->bullets;
  while (bullPtr != NULL) {
    if (!SDL_HasRectIntersectionFloat(&player->rect, &bullPtr->bullet.rect) &&
        bullPtr->bullet.damage != -1) {
      SDL_RenderTextureRotated(gRenderer, spriteSheet, &bulletSpriteRect,
                               &bullPtr->bullet.rect, bullPtr->bullet.rot, NULL,
                               SDL_FLIP_NONE);
      // SDL_RenderRect(gRenderer, &bullPtr->bullet.rect);
    }

    bullPtr = bullPtr->nextBullet;
  }

  AsteroidNode *astPtr = asteroids;
  while (astPtr != NULL) /* Render Asteroids */
  {
    SDL_RenderTextureRotated(
        gRenderer, spriteSheet, &astPtr->asteroid.spriteRect,
        &astPtr->asteroid.rect, astPtr->asteroid.rot, NULL, SDL_FLIP_NONE);
    astPtr = astPtr->nextAsteroid;
  }

  PowerUpNode *powerPtr = powerUps;
  while (powerPtr != NULL) {
    SDL_FRect spriteRect;
    if (powerPtr->power.powerUp == SHIELD) {
      spriteRect = getSpriteRect(spriteList, "powerupBlue_shield.png");
    } else if (powerPtr->power.powerUp == ARMOR) {
      spriteRect = getSpriteRect(spriteList, "powerupBlue_star.png");
    } else if (powerPtr->power.powerUp == MULTIBULLET) {
      spriteRect = getSpriteRect(spriteList, "powerupBlue_bolt.png");
    }

    SDL_RenderTexture(gRenderer, spriteSheet, &spriteRect,
                      &powerPtr->power.rect);

    powerPtr = powerPtr->nextPowerUp;
  }

  TTF_DrawRendererText(fpsText, WIDTH - 70, HEIGHT - 20);

  playerTextHandler(player, gTextEngine, kenVectorFont);
  playerRender(player, gRenderer, spriteSheet, spriteList);

  SDL_RenderPresent(gRenderer);
}

void drawPaused(SDL_Renderer *gRenderer, TTF_TextEngine *gTextEngine,
                Button *buttons, SDL_Texture *pauseBack,
                TTF_Font *kenVectorFont) {
  char buttonsText[2][10] = {"Resume", "Menu"};
  SDL_SetRenderDrawColor(gRenderer, 0x22, 0x22, 0x11, 0x77);
  SDL_RenderFillRect(gRenderer, NULL);

  float backWidth, backHeight;
  SDL_GetTextureSize(pauseBack, &backWidth, &backHeight);
  SDL_FRect backRect = {0, 0, backWidth, backHeight};
  for (int i = 0; i < WIDTH; i += backWidth)
    for (int j = 0; j < HEIGHT; j += backHeight) {
      backRect.x = i;
      backRect.y = j;
      SDL_RenderTexture(gRenderer, pauseBack, NULL, &backRect);
    }

  for (uint8 i = 0; i < 2; i++) // Number of Buttons
  {
    TTF_Text *buttonText = TTF_CreateText(
        gTextEngine, kenVectorFont, buttonsText[i], strlen(buttonsText[i]));
    int textHeight, textWidth;
    TTF_GetTextSize(buttonText, &textWidth, &textHeight);
    if (buttons[i].hovered) {
      SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
      TTF_SetTextColor(buttonText, 255, 255, 255, 255);
    } else {
      SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255);
      TTF_SetTextColor(buttonText, 0, 0, 0, 255);
    }

    SDL_RenderFillRect(gRenderer, &buttons[i].rect);
    TTF_DrawRendererText(
        buttonText, buttons[i].posX + buttons[i].width / 2 - textWidth / 2.f,
        buttons[i].posY + buttons[i].height / 2 - textHeight / 2.f);
  }

  SDL_RenderPresent(gRenderer);
}

void drawOver(SDL_Renderer *gRenderer, TTF_TextEngine *gTextEngine,
              Button *buttons, TTF_Text **texts, SDL_Texture *overBack,
              TTF_Font *kenVectorFont) {
  SDL_SetRenderDrawColor(gRenderer, 0x22, 0x22, 0x11, 0xFF);
  SDL_RenderClear(gRenderer);

  float backWidth, backHeight;
  SDL_GetTextureSize(overBack, &backWidth, &backHeight);
  SDL_FRect backRect = {0, 0, backWidth, backHeight};
  for (int i = 0; i < WIDTH; i += backWidth)
    for (int j = 0; j < HEIGHT; j += backHeight) {
      backRect.x = i;
      backRect.y = j;
      SDL_RenderTexture(gRenderer, overBack, NULL, &backRect);
    }

  SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
  int textHeight, textWidth;
  TTF_GetTextSize(texts[0], &textWidth, &textHeight);
  TTF_DrawRendererText(texts[0], WIDTH / 2.f - textWidth / 2.f,
                       HEIGHT / 4.f - textHeight / 2.f);
  TTF_GetTextSize(texts[1], &textWidth, &textHeight);
  TTF_DrawRendererText(texts[1], WIDTH / 2.f - textWidth / 2.f,
                       3 * HEIGHT / 4.f - textHeight / 2.f);
  TTF_SetFontSize(kenVectorFont, HEIGHT / 50.f);
  TTF_GetTextSize(texts[2], &textWidth, &textHeight);
  TTF_DrawRendererText(texts[2], WIDTH / 2.f - textWidth / 2.f,
                       7 * HEIGHT / 8.f - textHeight / 2.f);
  TTF_SetFontSize(kenVectorFont, 3 * HEIGHT / 50.f);

  char buttonsText[2][10] = {"Menu", "Replay"};
  for (uint8 i = 0; i < 2; i++) // Number of Buttons
  {
    TTF_Text *buttonText = TTF_CreateText(
        gTextEngine, kenVectorFont, buttonsText[i], strlen(buttonsText[i]));
    TTF_GetTextSize(buttonText, &textWidth, &textHeight);
    if (buttons[i].hovered) {
      SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
      TTF_SetTextColor(buttonText, 255, 255, 255, 255);
    } else {
      SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255);
      TTF_SetTextColor(buttonText, 0, 0, 0, 255);
    }

    SDL_RenderFillRect(gRenderer, &buttons[i].rect);
    TTF_DrawRendererText(
        buttonText, buttons[i].posX + buttons[i].width / 2 - textWidth / 2.f,
        buttons[i].posY + buttons[i].height / 2 - textHeight / 2.f);
  }

  SDL_RenderPresent(gRenderer);
}

void drawScores(SDL_Renderer *gRenderer, TTF_TextEngine *gTextEngine,
                Button *buttons, TTF_Text **texts, ScoreObj *scores,
                SDL_Texture *scoreBack, TTF_Font *kenVectorFont) {
  SDL_SetRenderDrawColor(gRenderer, 0x22, 0x22, 0x11, 0xFF);
  SDL_RenderClear(gRenderer);

  float backWidth, backHeight;
  SDL_GetTextureSize(scoreBack, &backWidth, &backHeight);
  SDL_FRect backRect = {0, 0, backWidth, backHeight};
  for (int i = 0; i < WIDTH; i += backWidth)
    for (int j = 0; j < HEIGHT; j += backHeight) {
      backRect.x = i;
      backRect.y = j;
      SDL_RenderTexture(gRenderer, scoreBack, NULL, &backRect);
    }

  SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
  if (texts[3] != NULL) {
    int textHeight, textWidth;
    TTF_SetFontSize(kenVectorFont, HEIGHT / 50.f);
    TTF_GetTextSize(texts[0], &textWidth, &textHeight);
    TTF_DrawRendererText(texts[0], WIDTH / 2.f - textWidth / 2.f,
                         3 * HEIGHT / 4.f - textHeight / 2.f);
    TTF_SetFontSize(kenVectorFont, 3 * HEIGHT / 50.f);
    TTF_GetTextSize(texts[1], &textWidth, &textHeight);
    TTF_DrawRendererText(texts[1], WIDTH / 2.f - textWidth / 2.f,
                         2 * HEIGHT / 4.f - textHeight / 2.f);
  } else {
    TTF_SetFontSize(kenVectorFont, HEIGHT / 50.f);
    for (uint8 i = 0; i < 2; i++) // Number of Buttons
    {
      int textHeight, textWidth;
      TTF_GetTextSize(buttons[i].text, &textWidth, &textHeight);
      if (buttons[i].hovered) {
        SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
        TTF_SetTextColor(buttons[i].text, 255, 255, 255, 255);
      } else {
        SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255);
        TTF_SetTextColor(buttons[i].text, 0, 0, 0, 255);
      }

      SDL_RenderFillRect(gRenderer, &buttons[i].rect);
      TTF_DrawRendererText(
          buttons[i].text,
          buttons[i].posX + buttons[i].width / 2 - textWidth / 2.f,
          buttons[i].posY + buttons[i].height / 2 - textHeight / 2.f);
    }

    int textWidth, textHeight;
    TTF_GetTextSize(texts[2], &textWidth, &textHeight);
    TTF_DrawRendererText(texts[2], WIDTH / 2.f - textWidth / 2.f,
                         50 - textHeight / 2.f);
    TTF_SetFontSize(kenVectorFont, 3 * HEIGHT / 50.f);

    char *tempStr;
    TTF_Text *tempText = NULL;

    SDL_asprintf(&tempStr, "Username");
    tempText = TTF_CreateText(gTextEngine, kenVectorFont, tempStr, 0);
    TTF_GetTextSize(tempText, &textWidth, &textHeight);
    TTF_DrawRendererText(tempText, 10,
                         (2 * HEIGHT / 11.f) - (textHeight / 2.f));
    SDL_asprintf(&tempStr, "Asteroids Destroyed");
    tempText = TTF_CreateText(gTextEngine, kenVectorFont, tempStr, 0);
    TTF_GetTextSize(tempText, &textWidth, &textHeight);
    TTF_DrawRendererText(tempText, WIDTH / 2.f - textWidth / 2.f,
                         (2 * HEIGHT / 11.f) - (textHeight / 2.f));
    SDL_asprintf(&tempStr, "Time Survived");
    tempText = TTF_CreateText(gTextEngine, kenVectorFont, tempStr, 0);
    TTF_GetTextSize(tempText, &textWidth, &textHeight);
    TTF_DrawRendererText(tempText, WIDTH - (10 + textWidth),
                         (2 * HEIGHT / 11.f) - (textHeight / 2.f));

    for (int i = 0; i < 8; i++) {
      if (scores[i].username[0] != '\0') {
        SDL_asprintf(&tempStr, "%s", scores[i].username);
        tempText = TTF_CreateText(gTextEngine, kenVectorFont, tempStr, 0);
        TTF_GetTextSize(tempText, &textWidth, &textHeight);
        TTF_DrawRendererText(tempText, 10,
                             ((i + 3) * HEIGHT / 11.f) - (textHeight / 2.f));
        SDL_asprintf(&tempStr, "%i", scores[i].score);
        tempText = TTF_CreateText(gTextEngine, kenVectorFont, tempStr, 0);
        TTF_GetTextSize(tempText, &textWidth, &textHeight);
        TTF_DrawRendererText(tempText, WIDTH / 2.f - textWidth / 2.f,
                             ((i + 3) * HEIGHT / 11.f) - (textHeight / 2.f));
        SDL_asprintf(&tempStr, "%i", scores[i].time);
        tempText = TTF_CreateText(gTextEngine, kenVectorFont, tempStr, 0);
        TTF_GetTextSize(tempText, &textWidth, &textHeight);
        TTF_DrawRendererText(tempText, WIDTH - (10 + textWidth),
                             ((i + 3) * HEIGHT / 11.f) - (textHeight / 2.f));
      }
    }
  }

  SDL_RenderPresent(gRenderer);
}
