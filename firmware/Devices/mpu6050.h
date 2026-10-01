#ifndef MUSEGUARD_MPU6050_H
#define MUSEGUARD_MPU6050_H
#include "bus.h"
#include <stdbool.h>
#define MPU6050_ADDRESS 0x68U
#define MPU6050_FRAME_SIZE 14U
typedef struct {
  int16_t accel_raw[3], gyro_raw[3], temperature_raw;
  int32_t accel_mg[3], gyro_mdps[3], temperature_mc;
} mpu6050_sample_t;
typedef struct {
  mg_bus_t bus;
  bool ready;
} mpu6050_t;
mg_status_t mpu6050_init(mpu6050_t *device, mg_bus_t bus);
mg_status_t mpu6050_read(mpu6050_t *device, mpu6050_sample_t *sample);
mg_status_t mpu6050_decode(const uint8_t *frame, size_t length, mpu6050_sample_t *sample);
#endif
