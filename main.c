#include "deps/includes.h"
#include "deps/objects.h"

int main(int argc, char *args[]) {
  enum State gameState = MENU;

  SDL_Window *gWindow = NULL;
  SDL_Renderer *gRenderer = NULL;

  SDL_Texture *menuBack1 = NULL;
  SDL_Texture *menuBack2 = NULL;
  SDL_Texture *scoreBack = NULL;
  SDL_Texture *gameBack = NULL;
  SDL_Texture *pauseBack = NULL;
  SDL_Texture *overBack = NULL;

  SDL_Texture *spriteSheet = NULL;

  TTF_Font *kenVectorFont = NULL;
  TTF_TextEngine *gTextEngine = NULL;

  Mix_Music *bgMusic = NULL;
  Mix_Music *battleMusic = NULL;

  Mix_Chunk *selectSfx = NULL;
  Mix_Chunk *shootSfx = NULL;
  Mix_Chunk *loseSfx = NULL;
  Mix_Chunk *shieldUpSfx = NULL;
  Mix_Chunk *shieldDownSfx = NULL;
  Mix_Chunk *astDestroySfx = NULL;

  Sprite spriteList[SPRITEMAX] = {};
  bool run = false;

  if (init(&gWindow, &gRenderer, &gTextEngine, &kenVectorFont)) /* Initialize */
    if (load(gRenderer, spriteList, &menuBack1, &menuBack2, &gameBack,
             &pauseBack, &overBack, &scoreBack, &spriteSheet, &bgMusic,
             &battleMusic, &shootSfx, &shieldUpSfx, &shieldDownSfx,
             &astDestroySfx, &loseSfx, &selectSfx)) /* Load Assets */
      run = true;

  while (run) {
    if (gameState == MENU) /* Main Menu */
    {
      /* Music */
      if (!Mix_PlayingMusic())
        Mix_PlayMusic(bgMusic, -1);

      /* Buttons */
      Button buttons[3];

      Button playButton; /* Play */
      playButton.clicked = 0;
      playButton.hovered = 0;
      playButton.width = 200;
      playButton.height = 50;
      playButton.posX = (WIDTH - playButton.width) / 2;
      playButton.posY = 1 * HEIGHT / 4.f - playButton.height / 2.f;
      playButton.rect.w = playButton.width;
      playButton.rect.h = playButton.height;
      playButton.rect.x = playButton.posX;
      playButton.rect.y = playButton.posY;

      Button scoreButton; /* Scores */
      scoreButton.clicked = 0;
      scoreButton.hovered = 0;
      scoreButton.width = 200;
      scoreButton.height = 50;
      scoreButton.posX = (WIDTH - scoreButton.width) / 2;
      scoreButton.posY = 2 * HEIGHT / 4.f - scoreButton.height / 2.f;
      scoreButton.rect.w = scoreButton.width;
      scoreButton.rect.h = scoreButton.height;
      scoreButton.rect.x = scoreButton.posX;
      scoreButton.rect.y = scoreButton.posY;

      Button quitButton; /* Quit */
      quitButton.clicked = 0;
      quitButton.hovered = 0;
      quitButton.width = 200;
      quitButton.height = 50;
      quitButton.posX = (WIDTH - quitButton.width) / 2;
      quitButton.posY = 3 * HEIGHT / 4.f - quitButton.height / 2.f;
      quitButton.rect.w = quitButton.width;
      quitButton.rect.h = quitButton.height;
      quitButton.rect.x = quitButton.posX;
      quitButton.rect.y = quitButton.posY;

      buttons[0] = playButton;
      buttons[1] = scoreButton;
      buttons[2] = quitButton;

      float f1PosX = 0;
      float f1Speed = 10;
      float f2PosX = 0;
      float f2Speed = 8;

      SDL_Event e;
      bool exited = false;

      while (!exited) {
        while (SDL_PollEvent(&e) != 0) {
          if (e.type == SDL_EVENT_QUIT) {
            exited = true;
            run = false;
          }
        }

        for (uint8 i = 0; i < 3; i++) {
          buttonStateUpdater(&buttons[i], selectSfx);
        }

        if (buttons[0].clicked == true) {
          gameState = GAME;
        }

        else if (buttons[1].clicked == true) {
          gameState = SCORES;
        }

        else if (buttons[2].clicked == true) {
          exited = true;
          run = false;
        }

        if (gameState != MENU) {
          break;
        }

        f1PosX += f1Speed;
        if (f1PosX > WIDTH)
          f1PosX = 0;
        f2PosX += f2Speed;
        if (f2PosX > WIDTH)
          f2PosX = 0;

        drawMenu(gRenderer, gTextEngine, buttons, f1PosX, f2PosX, menuBack1,
                 menuBack2, kenVectorFont);
      }
    }

    else if (gameState == GAME) /* Gameplay */
    {
      /* Initializing Game Objects */
      /* Delta Timer */
      DeltaTimer dTimer;

      dTimer.frameStart = SDL_GetPerformanceCounter();
      dTimer.frameEnd = 0;
      dTimer.delta = 0;

      uint64 frameStart;
      uint64 frameEnd;
      float fps;

      /* Player */
      Player player;
      playerInit(&player);

      /* Asteroids */
      AsteroidNode asteroids = asteroidInit();

      Timer astSpawnTimer;
      int astSpawnCount = 0;
      int astSpawnTime = 5000;
      timerStart(&astSpawnTimer);

      PowerUpNode powerUps = powerUpInit();

      loadPlayer(gRenderer, &player);

      /* Play Music */
      Mix_HaltMusic();
      Mix_PlayMusic(battleMusic, -1);

      SDL_Event e;
      bool exited = false;
      bool replay = false;

      while (!exited) {
        while (SDL_PollEvent(&e) != 0) {
          if (e.type == SDL_EVENT_QUIT) {
            exited = true;
            run = false;
          }

          playerEventHandler(e, &player, &gameState);
        }

        frameStart = SDL_GetPerformanceCounter();

        deltaCalc(&dTimer);
        timerCalcTicks(&player.gameTimer);

        playerMovementHandler(&player, dTimer.delta);
        playerBulletHander(&player, dTimer.delta, shootSfx);
        playerPowerUpHandler(&player, &powerUps, spriteSheet, spriteList,
                             shieldUpSfx, shieldDownSfx);
        asteroidHandler(&asteroids, &powerUps, &player, &astSpawnTimer,
                        &astSpawnCount, astSpawnTime, dTimer.delta, spriteList,
                        astDestroySfx);
        if (astSpawnCount > 10 && astSpawnCount > 500)
          astSpawnTime -= 100;

        if (player.armor < 0) {
          Mix_PlayChannel(1, loseSfx, 0);
          Mix_HaltMusic();
          gameState = OVER;
        }

        if (gameState == OVER) {
          AsteroidNode *astTemp = asteroids.nextAsteroid;
          while (astTemp != NULL) {
            AsteroidNode *toFree = astTemp;
            astTemp = astTemp->nextAsteroid;
            free(toFree);
          }
          asteroids.nextAsteroid = NULL;

          BulletNode *bullTemp = player.bullets.nextBullet;
          while (bullTemp != NULL) {
            BulletNode *toFree = bullTemp;
            bullTemp = bullTemp->nextBullet;
            free(toFree);
          }
          player.bullets.nextBullet = NULL;

          PowerUpNode *powerTemp = powerUps.nextPowerUp;
          while (powerTemp != NULL) {
            PowerUpNode *toFree = powerTemp;
            powerTemp = powerTemp->nextPowerUp;
            free(toFree);
          }
          powerUps.nextPowerUp = NULL;

          SDL_DestroyTexture(player.icon);

          TTF_SetFontSize(kenVectorFont, 3 * HEIGHT / 50.f);
          char *tempText;
          SDL_asprintf(
              &tempText,
              "You destroyed %li asteroids \nin %i minutes and %i seconds",
              player.score, (player.gameTimer.ticks / 1000) / 60,
              (player.gameTimer.ticks / 1000) % 60);
          free(tempText);
          TTF_Text *texts[3] = {};
          texts[0] = TTF_CreateText(gTextEngine, kenVectorFont, tempText, 0);
          tempText =
              "Type the username. Press Escape to not save the score. Press "
              "Enter to save the score.\n(Empty Username won't be stored!)";
          texts[2] = TTF_CreateText(gTextEngine, kenVectorFont, tempText, 0);

          Button buttons[2];

          buttons[0].clicked = 0;
          buttons[0].width = 250;
          buttons[0].height = 100;
          buttons[0].hovered = 0;
          buttons[0].posX = WIDTH / 3.f - buttons[0].width / 2.f;
          buttons[0].posY = 2 * HEIGHT / 4.f;
          buttons[0].rect.x = buttons[0].posX;
          buttons[0].rect.y = buttons[0].posY;
          buttons[0].rect.w = buttons[0].width;
          buttons[0].rect.h = buttons[0].height;

          buttons[1].clicked = 0;
          buttons[1].width = 250;
          buttons[1].height = 100;
          buttons[1].hovered = 0;
          buttons[1].posX = 2 * WIDTH / 3.f - buttons[1].width / 2.f;
          buttons[1].posY = 2 * HEIGHT / 4.f;
          buttons[1].rect.x = buttons[1].posX;
          buttons[1].rect.y = buttons[1].posY;
          buttons[1].rect.w = buttons[1].width;
          buttons[1].rect.h = buttons[1].height;

          bool textInput = true;
          SDL_Rect textArea = {WIDTH / 2, 3 * HEIGHT / 4, 200, 60};
          char playerName[50] = {};
          int nameCursor = 0;
          SDL_SetTextInputArea(gWindow, NULL, 0);
          SDL_StartTextInput(gWindow);

          SDL_Event e;
          bool next = false;
          while (!next) {
            while (SDL_PollEvent(&e) != 0) {
              if (e.type == SDL_EVENT_QUIT) {
                next = true;
                run = false;
              }

              else if (e.type == SDL_EVENT_TEXT_INPUT &&
                       nameCursor < sizeof(playerName) - 1) {
                SDL_strlcat(playerName, e.text.text, sizeof(playerName));
                nameCursor = strlen(playerName);
              }

              else if (e.type == SDL_EVENT_KEY_DOWN && textInput) {
                if (e.key.key == SDLK_RETURN) {
                  SDL_StopTextInput(gWindow);
                  textInput = false;
                } else if (e.key.key == SDLK_ESCAPE) {
                  strcpy(playerName, "");
                  SDL_StopTextInput(gWindow);
                  textInput = false;
                } else if (e.key.key == SDLK_BACKSPACE && nameCursor > 0) {
                  nameCursor--;
                  playerName[nameCursor] = '\0';
                }
              }
            }
            texts[1] =
                TTF_CreateText(gTextEngine, kenVectorFont, playerName, 0);

            for (int i = 0; i < 2; i++) {
              buttonStateUpdater(&buttons[i], selectSfx);
            }

            if (buttons[0].clicked) {
              Mix_HaltMusic();
              gameState = MENU;
            }

            if (buttons[1].clicked) {
              replay = true;
              gameState = GAME;
            }

            if (gameState != OVER) {
              TTF_DestroyText(texts[0]);
              TTF_DestroyText(texts[1]);
              TTF_DestroyText(texts[2]);

              if (strcmp(playerName, "") != 0) {
                char *jsonData = extractScores("scores.json");
                jsonData = updateScores(jsonData, playerName, player.score,
                                        player.gameTimer.ticks / 1000);
                saveScores(jsonData, "scores.json");
              }

              TTF_SetFontSize(kenVectorFont, HEIGHT / 50.f);
              break;
            }

            drawOver(gRenderer, gTextEngine, buttons, texts, overBack,
                     kenVectorFont);
          }
        } else if (gameState == PAUSED) {
          Mix_PauseMusic();
          Button buttons[2];

          Button resumeButton;
          resumeButton.clicked = 0;
          resumeButton.hovered = 0;
          resumeButton.width = 200;
          resumeButton.height = 50;
          resumeButton.posX = (WIDTH - resumeButton.width) / 2;
          resumeButton.posY = 1 * HEIGHT / 3.f - resumeButton.height / 2;
          resumeButton.rect.w = resumeButton.width;
          resumeButton.rect.h = resumeButton.height;
          resumeButton.rect.x = resumeButton.posX;
          resumeButton.rect.y = resumeButton.posY;

          Button menuButton;
          menuButton.clicked = 0;
          menuButton.hovered = 0;
          menuButton.width = 200;
          menuButton.height = 50;
          menuButton.posX = (WIDTH - menuButton.width) / 2;
          menuButton.posY = 2 * HEIGHT / 3.f - menuButton.height / 2;
          menuButton.rect.w = menuButton.width;
          menuButton.rect.h = menuButton.height;
          menuButton.rect.x = menuButton.posX;
          menuButton.rect.y = menuButton.posY;

          buttons[0] = resumeButton;
          buttons[1] = menuButton;

          SDL_Event e;
          bool paused = true;

          while (paused) {
            while (SDL_PollEvent(&e) != 0) {
              if (e.type == SDL_EVENT_QUIT) {
                paused = false;
                run = false;
              }

              if (e.type == SDL_EVENT_KEY_DOWN && e.key.repeat == false) {
                if (e.key.key == SDLK_ESCAPE) {
                  gameState = GAME;
                  Mix_ResumeMusic();
                  paused = false;
                }
              }
            }
            for (uint8 i = 0; i < 2; i++) {
              buttonStateUpdater(&buttons[i], selectSfx);
            }

            if (buttons[0].clicked) {
              gameState = GAME;
              Mix_ResumeMusic();
              paused = false;
            }

            if (buttons[1].clicked) {
              gameState = MENU;
              Mix_HaltMusic();
              paused = false;
              exited = true;
            }

            if (gameState != PAUSED) {
              break;
            }

            drawPaused(gRenderer, gTextEngine, buttons, pauseBack,
                       kenVectorFont);
          }
        }

        if (gameState != GAME || replay) {
          break;
        }

        char *fpsStr;
        SDL_asprintf(&fpsStr, "%f", 1 / fps);
        TTF_Text *fpsText =
            TTF_CreateText(gTextEngine, kenVectorFont, fpsStr, 0);
        free(fpsStr);

        drawGame(gRenderer, gTextEngine, &player, &asteroids, &powerUps,
                 fpsText, gameBack, spriteSheet, spriteList,
                 kenVectorFont); /* Draw, Blit and Render */

        TTF_DestroyText(fpsText);
        frameEnd = SDL_GetPerformanceCounter();
        fps = (frameEnd - frameStart) / (float)SDL_GetPerformanceFrequency();
      }
    }

    else if (gameState == SCORES) {
      Button buttons[2];

      buttons[0].text =
          TTF_CreateText(gTextEngine, kenVectorFont, "Sort By Score", 0);
      buttons[0].clicked = 0;
      buttons[0].hovered = 0;
      buttons[0].width = 200;
      buttons[0].height = 50;
      buttons[0].posX = 10;
      buttons[0].posY = 1 * HEIGHT / 11.f - buttons[0].height / 2.f;
      buttons[0].rect.w = buttons[0].width;
      buttons[0].rect.h = buttons[0].height;
      buttons[0].rect.x = buttons[0].posX;
      buttons[0].rect.y = buttons[0].posY;
      Timer buttonTimer = timerInit();
      timerStart(&buttonTimer);

      buttons[1].text = TTF_CreateText(gTextEngine, kenVectorFont, "Search", 0);
      buttons[1].clicked = 0;
      buttons[1].hovered = 0;
      buttons[1].width = 200;
      buttons[1].height = 50;
      buttons[1].posX = WIDTH - buttons[1].width - 10;
      buttons[1].posY = 1 * HEIGHT / 11.f - buttons[1].height / 2.f;
      buttons[1].rect.w = buttons[1].width;
      buttons[1].rect.h = buttons[1].height;
      buttons[1].rect.x = buttons[1].posX;
      buttons[1].rect.y = buttons[1].posY;

      char *jsonData = extractScores("scores.json"); /* Grab Data */
      enum Sort sortType = SCORE;
      cJSON *root = NULL;
      cJSON *scoreArr = NULL;
      if (jsonData == NULL) /* Doesn't Exist -> Create New */
      {
        root = cJSON_CreateObject();
        scoreArr = cJSON_CreateArray();
        cJSON_AddItemToObject(root, "Scores", scoreArr);
      } else /* Exist -> Use Existing*/
      {
        root = cJSON_Parse(jsonData);
        sortScores(root, sortType);
        scoreArr = cJSON_GetObjectItem(root, "Scores");
      }

      free(jsonData);

      int arrSize = cJSON_GetArraySize(scoreArr);
      ScoreObj scores[8]; /* 8 Scores At A Time */
      int scoreCursor = 0;
      TTF_Text *texts[4] = {};

      char tempText[] = "Enter Username to get Scores. Leave empty to get all.";
      texts[0] = TTF_CreateText(gTextEngine, kenVectorFont, tempText, 0);
      texts[2] = TTF_CreateText(gTextEngine, kenVectorFont,
                                "Press Esc to go back to menu", 0);
      texts[3] = NULL;

      char username[50] = {};
      int nameCursor = 0;
      bool textInput = false;

      SDL_Event e;
      bool exited = false;

      while (!exited) {
        while (SDL_PollEvent(&e) != 0) {
          if (e.type == SDL_EVENT_QUIT) {
            exited = true;
            run = false;
          }

          else if (e.type == SDL_EVENT_TEXT_INPUT &&
                   nameCursor < sizeof(username) - 1) {
            SDL_strlcat(username, e.text.text, sizeof(username));
            nameCursor = strlen(username);
          }

          else if (e.type == SDL_EVENT_KEY_DOWN && textInput) {
            if (e.key.key == SDLK_RETURN || e.key.key == SDLK_ESCAPE) {
              if (e.key.key == SDLK_ESCAPE)
                strcpy(username, "");
              SDL_StopTextInput(gWindow);
              textInput = false;
              texts[1] =
                  TTF_CreateText(gTextEngine, kenVectorFont, username, 0);
              texts[3] = NULL;
            } else if (e.key.key == SDLK_BACKSPACE && nameCursor > 0) {
              nameCursor--;
              username[nameCursor] = '\0';
            }
          }

          else if (e.type == SDL_EVENT_KEY_DOWN && e.key.repeat == 0) {
            if (e.key.key == SDLK_ESCAPE)
              gameState = MENU;
          }

          else if (e.type == SDL_EVENT_MOUSE_WHEEL && !textInput) {
            if (!(scoreCursor - (int)e.wheel.y < 0) &&
                !(scoreCursor - (int)e.wheel.y >
                  arrSize - 8 /* Number of Scores on Screen*/))
              scoreCursor -= (int)e.wheel.y;
          }
        }
        if (textInput)
          texts[1] = TTF_CreateText(gTextEngine, kenVectorFont, username, 0);
        else {
          for (int i = 0; i < 2; i++) {
            buttonStateUpdater(&buttons[i], selectSfx);
          }

          if (buttons[0].clicked && buttonTimer.ticks > 200) {
            if (sortType == SCORE) /* Cycle through sort types */
            {
              sortType = TIME;
              buttons[0].text =
                  TTF_CreateText(gTextEngine, kenVectorFont, "Sort By Time", 0);
            } else if (sortType == TIME) {
              sortType = NAME;
              buttons[0].text =
                  TTF_CreateText(gTextEngine, kenVectorFont, "Sort By Name", 0);
            } else if (sortType == NAME) {
              sortType = SCORE;
              buttons[0].text = TTF_CreateText(gTextEngine, kenVectorFont,
                                               "Sort By Score", 0);
            }
            sortScores(root, sortType);
            scoreArr = cJSON_GetObjectItem(root, "Scores"); /* Regrab array */
            timerReset(&buttonTimer);
          }

          if (buttons[1].clicked) {
            texts[3] = TTF_CreateText(gTextEngine, kenVectorFont, "!", 0);
            textInput = true;
            SDL_StartTextInput(gWindow);
          }

          if (username[0] == '\0') {
            for (int i = 0; i < 8; i++) {
              cJSON *jScoreObj = cJSON_GetArrayItem(scoreArr, scoreCursor + i);
              if (jScoreObj != NULL) {
                cJSON *jUsername = cJSON_GetObjectItem(jScoreObj, "Username");
                char *userName = jUsername->valuestring;
                cJSON *jScore = cJSON_GetObjectItem(jScoreObj, "Score");
                int score = jScore->valueint;
                cJSON *jTime = cJSON_GetObjectItem(jScoreObj, "Time");
                int time = jTime->valueint;
                strcpy(scores[i].username, userName);
                scores[i].score = score;
                scores[i].time = time;
              } else {
                strcpy(scores[i].username, "");
                scores[i].score = 0;
                scores[i].time = 0;
              }
            }
          } else {
            cJSON *userArr = cJSON_CreateArray();
            cJSON *userScore = NULL;
            int arrSize = cJSON_GetArraySize(scoreArr);
            for (int i = 0; i < arrSize; i++) {
              userScore = cJSON_GetArrayItem(scoreArr, i);
              if (strncmp(
                      cJSON_GetObjectItem(userScore, "Username")->valuestring,
                      username, 50) == 0)
                cJSON_AddItemReferenceToArray(userArr, userScore);
            }

            for (int i = 0; i < 8; i++) {
              cJSON *jScoreObj = cJSON_GetArrayItem(userArr, scoreCursor + i);
              if (jScoreObj != NULL) {
                cJSON *jUsername = cJSON_GetObjectItem(jScoreObj, "Username");
                char *userName = jUsername->valuestring;
                cJSON *jScore = cJSON_GetObjectItem(jScoreObj, "Score");
                int score = jScore->valueint;
                cJSON *jTime = cJSON_GetObjectItem(jScoreObj, "Time");
                int time = jTime->valueint;
                strcpy(scores[i].username, userName);
                scores[i].score = score;
                scores[i].time = time;
              } else {
                strcpy(scores[i].username, "");
                scores[i].score = 0;
                scores[i].time = 0;
              }
            }
          }
        }

        if (gameState != SCORES) {
          TTF_DestroyText(texts[0]);
          TTF_DestroyText(texts[1]);
          TTF_DestroyText(texts[2]);
          TTF_DestroyText(texts[3]);
          TTF_SetFontSize(kenVectorFont, HEIGHT / 50.f);
          break;
        }

        timerCalcTicks(&buttonTimer);
        drawScores(gRenderer, gTextEngine, buttons, texts, scores, scoreBack,
                   kenVectorFont);
      }
    }
  }

  // Quit Protocols
  Mix_FreeChunk(loseSfx);
  Mix_FreeChunk(shieldDownSfx);
  Mix_FreeChunk(shieldUpSfx);
  Mix_FreeChunk(astDestroySfx);
  Mix_FreeChunk(shootSfx);
  Mix_FreeChunk(selectSfx);
  Mix_FreeMusic(battleMusic);
  Mix_FreeMusic(bgMusic);
  Mix_Quit();

  TTF_DestroyRendererTextEngine(gTextEngine);
  TTF_CloseFont(kenVectorFont);
  TTF_Quit();

  SDL_DestroyTexture(spriteSheet);
  SDL_DestroyTexture(scoreBack);
  SDL_DestroyTexture(overBack);
  SDL_DestroyTexture(pauseBack);
  SDL_DestroyTexture(gameBack);
  SDL_DestroyTexture(menuBack2);
  SDL_DestroyTexture(menuBack1);
  IMG_Quit();

  SDL_DestroyRenderer(gRenderer);
  SDL_DestroyWindow(gWindow);
  SDL_Quit();

  printf("=== End Of Program ===\n");
  return 0;
}
