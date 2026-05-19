#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>

#define TARGET_HZ 2000000
#define FPS 60
#define CYCLES_PER_FRAME (TARGET_HZ / FPS)   /* ~33,333 cycles */

typedef struct CPU CPU;

void init_timer(void);
void cpu_throttle(const CPU* cpu, uint32_t target_hz);

#endif
