#ifndef APP_KERNAL
#define APP_KERNAL

#include <stdint.h>

#define TIMER_MAX	16

typedef struct System_Timer
{
	uint8_t  timer_id;
	uint8_t  repeat;
  uint8_t  suspend;
	uint32_t repeattime;
	uint32_t start_tick;
	uint32_t stop_tick;
	void (*fun)(void);
	struct System_Timer *next;
} Timer;



void systemTickInc(void);
uint32_t getSystemTick(void);


int8_t startTimer(uint32_t time,void (*fun)(void),uint8_t repeat);
void stopTimer(uint8_t timer_id);
void stopTimerRepeat(uint8_t timer_id);


int8_t createSystemTask(void(*fun)(void),uint32_t cycletime);
void systemTaskSuspend(uint8_t taskid);
void systemTaskResume(uint8_t taskid);

void kernalRun(void);

#endif
