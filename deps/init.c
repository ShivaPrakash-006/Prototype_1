#include "init.h"
#include "includes.h"
#include "objects.h"

bool init(SDL_Window **gWindow, SDL_Renderer **gRenderer,
          TTF_TextEngine **gTextEngine, TTF_Font **kenVectorFont) {
  printf("=== Start Of Program ===\n");
  bool success = true;

  /* Initializing SDL3 */
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    printf("SDL could not be initialized! SDL Error: %s\n", SDL_GetError());
    success = false;
  } else {
    if (IMG_Init(IMG_INIT_PNG - IMG_INIT_JPG) == 0) {
      printf("SDL_Image could not be initialized! SDL_Image Error %s\n",
             SDL_GetError());
      success = false;
    } else {
      if (!TTF_Init()) {
        printf("SDL_ttf could not be initialized! SDL_ttf Error: %s\n",
               SDL_GetError());
        success = false;
      } else {
        *gWindow = SDL_CreateWindow("Astroids-P1", WIDTH, HEIGHT,
                                    SDL_WINDOW_BORDERLESS);
        if (*gWindow == NULL) {
          printf("Window could not be created! SDL Error: %s\n",
                 SDL_GetError());
          success = false;
        } else {
          *gRenderer = SDL_CreateRenderer(*gWindow, NULL);
          if (*gRenderer == NULL) {
            printf("Renderer could not be created! SDL Error: %s\n",
                   SDL_GetError());
            success = false;
          } else {
            SDL_SetRenderVSync(*gRenderer, 1);
            SDL_SetRenderDrawColor(*gRenderer, 0x22, 0x22, 0x11, 0xFF);
            *gTextEngine = TTF_CreateRendererTextEngine(*gRenderer);
            if (*gTextEngine == NULL) {
              printf("TextEngine could not be created! SDL_ttf Error: %s\n",
                     SDL_GetError());
              success = false;
            } else {
              *kenVectorFont = TTF_OpenFont(
                  "Assets/Fonts/kenvector_future_thin.ttf", HEIGHT / 50.f);
              if (*kenVectorFont == NULL) {
                printf("'JetBrainsMono-Medium.ttf' could not be loaded! "
                       "SDL_ttf Error: %s\n",
                       SDL_GetError());
                success = false;
              } else {
                SDL_AudioSpec audioSpec;
                SDL_zero(audioSpec);
                audioSpec.format = MIX_DEFAULT_FORMAT;
                audioSpec.channels = 2;
                audioSpec.freq = 44100;
                if (Mix_OpenAudio(0, &audioSpec) == 0 ||
                    Mix_Init(MIX_INIT_MP3) == 0 ||
                    Mix_Init(MIX_INIT_OGG) == 0) {
                  printf("SDL_mixer could not be initialized! SDL_mixer Error: "
                         "%s\n",
                         SDL_GetError());
                  success = false;
                } else {
                  Mix_VolumeMusic(80);
                }
              }
            }
          }
        }
      }
    }
  }

  return success;
}

bool load(SDL_Renderer *gRenderer, Sprite *spriteList, SDL_Texture **menuBack1,
          SDL_Texture **menuBack2, SDL_Texture **gameBack,
          SDL_Texture **pauseBack, SDL_Texture **overBack,
          SDL_Texture **scoreBack, SDL_Texture **spriteSheet,
          Mix_Music **bgMusic, Mix_Music **battleMusic, Mix_Chunk **shootSfx,
          Mix_Chunk **shieldUpSfx, Mix_Chunk **shieldDownSfx,
          Mix_Chunk **astDestroySfx, Mix_Chunk **loseSfx,
          Mix_Chunk **selectSfx) /* Get Game Objects and
load their respective assets */
{
  bool success = true;

  *menuBack1 = SDL_CreateTextureFromSurface(
      gRenderer, IMG_Load("Assets/Backgrounds/menu1.png"));
  if (*menuBack1 == NULL) {
    printf("'Assets/Backgrounds/menu1.png' could not be loaded! SDL_image "
           "Error: %s\n",
           SDL_GetError());
    success = false;
  }
  *menuBack2 = SDL_CreateTextureFromSurface(
      gRenderer, IMG_Load("Assets/Backgrounds/menu2.png"));
  if (*menuBack2 == NULL) {
    printf("'Assets/Backgrounds/menu2.png' could not be loaded! SDL_image "
           "Error: %s\n",
           SDL_GetError());
    success = false;
  }

  *scoreBack = SDL_CreateTextureFromSurface(
      gRenderer, IMG_Load("Assets/Backgrounds/scores.png"));
  if (*scoreBack == NULL) {
    printf("'Assets/Backgrounds/scores.png' could not be loaded! SDL_image "
           "Error: %s\n",
           SDL_GetError());
    success = false;
  }

  *gameBack = SDL_CreateTextureFromSurface(
      gRenderer, IMG_Load("Assets/Backgrounds/game.png"));
  if (*gameBack == NULL) {
    printf("'Assets/Backgrounds/game.png' could not be loaded! SDL_image "
           "Error: %s\n",
           SDL_GetError());
    success = false;
  }

  *pauseBack = SDL_CreateTextureFromSurface(
      gRenderer, IMG_Load("Assets/Backgrounds/paused.png"));
  if (*pauseBack == NULL) {
    printf("'Assets/Backgrounds/paused.png' could not be loaded! SDL_image "
           "Error: %s\n",
           SDL_GetError());
    success = false;
  }

  *overBack = SDL_CreateTextureFromSurface(
      gRenderer, IMG_Load("Assets/Backgrounds/over.png"));
  if (*overBack == NULL) {
    printf("'Assets/Backgrounds/over.png' could not be loaded! SDL_image "
           "Error: %s\n",
           SDL_GetError());
    success = false;
  }

  *spriteSheet =
      SDL_CreateTextureFromSurface(gRenderer, IMG_Load("Assets/sheet.png"));
  if (*spriteSheet == NULL) {
    printf("'Assets/sheet.png' could not be loaded! SDL_image Error: %s\n",
           SDL_GetError());
    success = false;
  }

  parseXML("Assets/sheet.xml", spriteList);

  // Sound and Music
  *bgMusic = Mix_LoadMUS("Assets/Music/background.mp3");
  if (*bgMusic == NULL) {
    printf("'Assets/Music/background.mp3' could not be loaded! SDL_mixer "
           "Error: %s\n",
           SDL_GetError());
    success = false;
  }

  *battleMusic = Mix_LoadMUS("Assets/Music/battle.mp3");
  if (*battleMusic == NULL) {
    printf("'Assets/Music/battle.mp3' could not be loaded! SDL_mixer "
           "Error: %s\n",
           SDL_GetError());
    success = false;
  }

  *selectSfx = Mix_LoadWAV("Assets/SoundEffects/sfx_twoTone.ogg");
  if (*selectSfx == NULL) {
    printf(
        "'Assets/SoundEffects/sfx_twoTone.ogg' could not be loaded! SDL_mixer "
        "Error: %s\n",
        SDL_GetError());
    success = false;
  }

  *shootSfx = Mix_LoadWAV("Assets/SoundEffects/sfx_laser2.ogg");
  if (*shootSfx == NULL) {
    printf(
        "'Assets/SoundEffects/sfx_laser2.ogg' could not be loaded! SDL_mixer "
        "Error: %s\n",
        SDL_GetError());
    success = false;
  }

  *shieldUpSfx = Mix_LoadWAV("Assets/SoundEffects/sfx_shieldUp.ogg");
  if (*shieldUpSfx == NULL) {
    printf(
        "'Assets/SoundEffects/sfx_shieldUp.ogg' could not be loaded! SDL_mixer "
        "Error: %s\n",
        SDL_GetError());
    success = false;
  }

  *shieldDownSfx = Mix_LoadWAV("Assets/SoundEffects/sfx_shieldDown.ogg");
  if (*shieldDownSfx == NULL) {
    printf("'Assets/SoundEffects/sfx_shieldDown.ogg' could not be loaded! "
           "SDL_mixer "
           "Error: %s\n",
           SDL_GetError());
    success = false;
  }

  *astDestroySfx = Mix_LoadWAV("Assets/SoundEffects/sfx_zap.ogg");
  if (*astDestroySfx == NULL) {
    printf("'Assets/SoundEffects/sfx_zap.ogg' could not be loaded! "
           "SDL_mixer "
           "Error: %s\n",
           SDL_GetError());
    success = false;
  }

  *loseSfx = Mix_LoadWAV("Assets/SoundEffects/sfx_lose.ogg");
  if (*loseSfx == NULL) {
    printf("'Assets/SoundEffects/sfx_lose.ogg' could not be loaded! "
           "SDL_mixer "
           "Error: %s\n",
           SDL_GetError());
    success = false;
  }

  return success;
}

bool loadPlayer(SDL_Renderer *gRenderer, Player *player) {
  bool success = true;
  player->icon =
      SDL_CreateTextureFromSurface(gRenderer, IMG_Load("Assets/Crystal.png"));
  if (player->icon == NULL) {
    printf("'Crystal.png' could not be loaded! SDL_image Error: %s\n",
           SDL_GetError());
    success = false;
  }

  return success;
}
