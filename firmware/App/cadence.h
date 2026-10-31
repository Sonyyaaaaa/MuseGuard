#ifndef MG_CADENCE_H
#define MG_CADENCE_H
#include <stdint.h>
#include <stdbool.h>
typedef struct {
  uint32_t period, deadline, due, count, skipped, late, max_lateness;
  uint32_t previous, released_due, completion_misses; int32_t jitter_min, jitter_max;
} cadence_t;
bool cadence_release(cadence_t *task,uint32_t now);
void cadence_complete(cadence_t *task,uint32_t now);
typedef struct { uint32_t count,min,max; uint64_t sum; } duration_t;
void duration_add(duration_t *stat,uint32_t elapsed);
#endif
