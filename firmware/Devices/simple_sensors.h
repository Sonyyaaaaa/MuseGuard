#ifndef MUSEGUARD_SIMPLE_SENSORS_H
#define MUSEGUARD_SIMPLE_SENSORS_H
#include "bus.h"
#include <stdbool.h>
typedef struct { bool high; } pir_t;
bool pir_rising(pir_t *device, bool high);
mg_status_t light_millivolts(uint16_t raw, uint16_t *millivolts);
#endif
