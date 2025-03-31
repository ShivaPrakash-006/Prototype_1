#ifndef ENUMS_H_
#define ENUMS_H_

enum State { MENU = 0, GAME = 1, PAUSED = 2, OVER = 3, SCORES = 4 };
enum Power {
  SHIELD = 0,
  ARMOR = 1,
  INFFUEL = 2,
  MULTIBULLET = 3,
};
enum Size { SMALL = 0, NORMAL = 1, LARGE = 2 };
enum Sort { SCORE = 0, TIME = 1, NAME = 2 };

#endif // ENUMS_H_
