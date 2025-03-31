#include "button.h"
#include "includes.h"
#include "objects.h"

void buttonStateUpdater(Button *button, Mix_Chunk *selectSfx) {
  float mouseX;
  float mouseY;
  SDL_MouseButtonFlags mouseState =
      SDL_GetMouseState(&mouseX, &mouseY); // Getting The State of the mouse

  if (mouseX > button->posX &&
      mouseX < button->posX + button->width && // Checking X Collision
      mouseY > button->posY &&
      mouseY < button->posY + button->height) // Checking Y Collision
  {
    button->hovered = true;
    if (mouseState == SDL_BUTTON_LMASK) {
      button->clicked = true;
      Mix_PlayChannel(1, selectSfx, 0);
    } else {
      button->clicked = false;
    }
  } else {
    button->hovered = false;
    button->clicked = false;
  }
}
