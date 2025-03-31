#include "timer.h"

// Timer Functions
Timer timerInit(void) {
  Timer timer;

  timer.startTicks = 0;
  timer.pausedTicks = 0;
  timer.started = false;
  timer.paused = false;
  timer.ticks = 0;

  return timer;
}

void timerStart(Timer *timer) {
  timer->started = true;
  timer->paused = false;

  timer->startTicks = SDL_GetTicks();
  timer->pausedTicks = 0;
}

void timerStop(Timer *timer) {
  timer->started = false;
  timer->paused = false;

  timer->ticks = 0;
  timer->startTicks = 0;
  timer->pausedTicks = 0;
}

void timerReset(Timer *timer) {
  timerStop(timer);
  timerStart(timer);
}

void timerPause(Timer *timer) {
  if (timer->started && !timer->paused) {
    timer->paused = true;

    timer->pausedTicks = SDL_GetTicks() - timer->startTicks;
    timer->startTicks = 0;
  }
}

void timerCalcTicks(Timer *timer) {
  if (timer->started) {
    if (timer->paused) {
      timer->ticks = timer->pausedTicks;
    } else {
      timer->ticks = SDL_GetTicks() - timer->startTicks;
    }
  }
}

// Delta Timer Functions
void deltaCalc(DeltaTimer *dTimer) {
  dTimer->frameEnd = dTimer->frameStart;
  dTimer->frameStart = SDL_GetPerformanceCounter();

  dTimer->delta = (dTimer->frameStart - dTimer->frameEnd) /
                  (double)SDL_GetPerformanceFrequency();
}
