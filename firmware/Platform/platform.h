#ifndef MUSEGUARD_PLATFORM_H
#define MUSEGUARD_PLATFORM_H
#include "bus.h"
#include <stdbool.h>
mg_bus_t platform_i2c(void);
mg_status_t platform_adc(uint16_t *raw);
bool platform_pir(void);
void platform_led(bool on);
uint32_t platform_now(void);
bool platform_command(char *command);
void platform_start(void);
#endif
