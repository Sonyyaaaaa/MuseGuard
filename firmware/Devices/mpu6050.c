#include "mpu6050.h"

static int16_t signed_word(const uint8_t *bytes)
{
  int32_t value = ((uint16_t)bytes[0] << 8) | bytes[1];
  if (value >= 32768) { value -= 65536; }
  return (int16_t)value;
}

mg_status_t mpu6050_decode(const uint8_t *frame, size_t length, mpu6050_sample_t *sample)
{
  if (frame == NULL || sample == NULL || length != MPU6050_FRAME_SIZE) {
    return MG_INVALID;
  }
  for (size_t axis = 0; axis < 3; ++axis) {
    sample->accel_raw[axis] = signed_word(frame + 2 * axis);
    sample->gyro_raw[axis] = signed_word(frame + 8 + 2 * axis);
    sample->accel_mg[axis] = (int32_t)sample->accel_raw[axis] * 1000 / 16384;
    sample->gyro_mdps[axis] = (int32_t)sample->gyro_raw[axis] * 1000 / 131;
  }
  sample->temperature_raw = signed_word(frame + 6);
  sample->temperature_mc = (int32_t)sample->temperature_raw * 1000 / 340 + 36530;
  return MG_OK;
}

mg_status_t mpu6050_init(mpu6050_t *device, mg_bus_t bus)
{
  if (device == NULL) { return MG_INVALID; }
  device->ready = false;
  if (bus.read == NULL || bus.write == NULL) { return MG_INVALID; }
  device->bus = bus;
  uint8_t id = 0;
  mg_status_t result = bus.read(bus.context, MPU6050_ADDRESS, 0x75, &id, 1);
  if (result != MG_OK) { return result; }
  if (id != 0x68) { return MG_BAD_ID; }
  static const uint8_t settings[][2] = {
    {0x6B, 0x00}, {0x1A, 0x03}, {0x1B, 0x00}, {0x1C, 0x00}, {0x19, 0x09}
  };
  for (size_t i = 0; i < sizeof(settings) / sizeof(settings[0]); ++i) {
    result = bus.write(bus.context, MPU6050_ADDRESS, settings[i][0], &settings[i][1], 1);
    if (result != MG_OK) { return result; }
  }
  device->ready = true;
  return MG_OK;
}

mg_status_t mpu6050_read(mpu6050_t *device, mpu6050_sample_t *sample)
{
  if (device == NULL || sample == NULL || !device->ready) { return MG_INVALID; }
  uint8_t frame[MPU6050_FRAME_SIZE];
  mg_status_t result = device->bus.read(device->bus.context, MPU6050_ADDRESS,
                                       0x3B, frame, sizeof(frame));
  if (result != MG_OK) { return result; }
  return mpu6050_decode(frame, sizeof(frame), sample);
}
