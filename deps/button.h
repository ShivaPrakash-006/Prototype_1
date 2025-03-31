#ifndef BUTTON_H_
#define BUTTON_H_

#include "includes.h"

typedef struct Button {
  float width;
  float height;
  float posX;
  float posY;

  bool hovered;
  bool clicked;

  SDL_FRect rect;
  TTF_Text *text;

} Button;

void buttonStateUpdater(Button *button, Mix_Chunk *selectSfx); 

#endif //BUTTON_H_
