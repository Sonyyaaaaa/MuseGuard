#include "museguard.h"
#include <string.h>
static int64_t square(int32_t value) { return (int64_t)value * value; }

void museguard_reset(museguard_t *app) { memset(app, 0, sizeof(*app)); }

uint32_t museguard_update(museguard_t *app, uint32_t now, bool presence,
                         bool adc_valid, uint16_t adc,
                         bool imu_valid, const mpu6050_sample_t *sample)
{
  imu_valid = imu_valid && sample != NULL;
  uint32_t events = pir_rising(&app->pir, presence) ? MG_PRESENCE : 0;
  bool fault = !adc_valid || !imu_valid;
  if (fault && !app->fault) { events |= MG_FAULT; }
  if (!fault && app->fault) { events |= MG_RECOVERED; }
  app->fault = fault;
  if (adc_valid) {
    if (!app->light_ready) {
      app->light_base = adc;
      app->light_ready = true;
    }
    int32_t delta = (int32_t)adc - app->light_base;
    if (delta < 0) { delta = -delta; }
    if (delta >= 600 && !app->light_active) {
      app->light_active = true;
      events |= MG_LIGHT;
    }
    if (delta < 400) { app->light_active = false; }
  }
  if (!imu_valid || sample == NULL) {
    app->tilt_pending = false;
    return events;
  }
  if (!app->imu_ready) {
    memcpy(app->accel_base, sample->accel_mg, sizeof(app->accel_base));
    app->imu_ready = true;
  }
  int64_t norm2 = 0, delta2 = 0;
  for (size_t i = 0; i < 3; ++i) {
    norm2 += square(sample->accel_mg[i]);
    delta2 += square(sample->accel_mg[i] - app->accel_base[i]);
  }
  bool shock = norm2 > square(1400) || norm2 < square(600);
  if (shock && !app->shock_active) {
    events |= MG_SHOCK;
    app->alarm = true;
    app->shock_active = true;
  }
  if (norm2 >= square(900) && norm2 <= square(1100)) {
    app->shock_active = false;
  }
  bool stable_gravity = norm2 >= square(800) && norm2 <= square(1200);
  if (stable_gravity && delta2 >= square(400)) {
    if (!app->tilt_pending) {
      app->tilt_since = now;
      app->tilt_pending = true;
    }
    if (!app->tilt_active && (uint32_t)(now - app->tilt_since) >= 500) {
      app->tilt_active = true;
      app->alarm = true;
      events |= MG_TILT;
    }
  } else {
    app->tilt_pending = false;
    if (delta2 < square(250)) { app->tilt_active = false; }
  }
  return events;
}
