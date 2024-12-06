#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

// Timer functions
void timer_init(uint32_t frequency);
void timer_wait(uint32_t ticks);
uint32_t get_timer_ticks(void);
void sleep(uint32_t ms);

#endif /* TIMER_H */
