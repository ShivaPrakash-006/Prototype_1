#ifndef TIMER_H_
#define TIMER_H_

#include "includes.h"

typedef struct Timer {
  uint32 startTicks;
  uint32 pausedTicks;

  uint32 ticks;

  _Bool started;
  _Bool paused;

} Timer;

typedef struct DeltaTimer {
  uint32 frameStart;
  uint32 frameEnd;

  double delta;
} DeltaTimer;


Timer timerInit(void); 
void timerStart(Timer *timer);
void timerStop(Timer *timer);
void timerReset(Timer *timer);
void timerPause(Timer *timer);
void timerCalcTicks(Timer *timer);

void deltaCalc(DeltaTimer *dTimer);

#endif //TIMER_H_
