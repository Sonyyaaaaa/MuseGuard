#include "wokwi-api.h"
#include <stdint.h>
static pin_t pir, light;
static timer_t timer;
static uint32_t seconds;
static void tick(void *data) {
  (void)data; ++seconds;
  pin_write(pir, seconds >= 20 && (seconds - 20) % 300 == 0 ? HIGH : LOW);
  if (seconds == 120 || seconds == 1920) pin_dac_write(light, 4.96f);
  if (seconds == 180 || seconds == 1980) pin_dac_write(light, 1.22f);
}
void chip_init(void) {
  pir = pin_init("PIR", OUTPUT_LOW);
  light = pin_init("LIGHT", ANALOG);
  pin_dac_write(light, 1.22f);
  const timer_config_t t = {.callback = tick};
  timer = timer_init(&t);
  timer_start(timer, 1000000, true);
}
