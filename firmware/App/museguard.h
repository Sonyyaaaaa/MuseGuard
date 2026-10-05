#ifndef MUSEGUARD_APP_H
#define MUSEGUARD_APP_H
#include "mpu6050.h"
#include "simple_sensors.h"
enum {
  MG_PRESENCE = 1U, MG_LIGHT = 2U, MG_SHOCK = 4U, MG_TILT = 8U,
  MG_FAULT = 16U, MG_RECOVERED = 32U
};
typedef struct {
  pir_t pir;
  bool light_ready, imu_ready, light_active, shock_active;
  bool tilt_pending, tilt_active, alarm, fault;
  uint16_t light_base;
  int32_t accel_base[3];
  uint32_t tilt_since;
} museguard_t;
void museguard_reset(museguard_t *app);
uint32_t museguard_update(museguard_t *app, uint32_t now, bool presence,
                         bool adc_valid, uint16_t adc,
                         bool imu_valid, const mpu6050_sample_t *sample);
#endif
