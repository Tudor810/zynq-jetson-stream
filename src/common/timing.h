#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>

/* Current time in microseconds, from a monotonic clock.
   Only differences between two calls are meaningful. */
uint64_t time_now_us(void);

#endif /* TIMING_H */