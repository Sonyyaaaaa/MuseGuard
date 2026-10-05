#include "runner.h"
#include "platform.h"
#include "museguard.h"
#include <stdio.h>

static mg_bus_t bus;
static mpu6050_t imu;
static museguard_t app;
static uint32_t last_sample, last_telemetry, last_retry;
static bool adc_calibrated;

static const char *status_name(mg_status_t result)
{
  switch (result) {
    case MG_OK: return "OK";
    case MG_INVALID: return "INVALID";
    case MG_BUS_ERROR: return "BUS_ERROR";
    case MG_TIMEOUT: return "TIMEOUT";
    case MG_BAD_ID: return "BAD_ID";
    default: return "UNKNOWN";
  }
}

static void print_sample(uint32_t now, uint16_t adc, mg_status_t adc_status,
                         mg_status_t imu_status, const mpu6050_sample_t *sample)
{
  uint16_t mv = 0;
  light_millivolts(adc, &mv);
  printf("DATA t=%lu pir=%u adc_status=%s adc=%u mv=%u imu_status=%s "
         "ax_mg=%ld ay_mg=%ld az_mg=%ld gx_mdps=%ld gy_mdps=%ld gz_mdps=%ld "
         "temp_mc=%ld alarm=%u\r\n", (unsigned long)now,
         (unsigned)platform_pir(), status_name(adc_status), (unsigned)adc, (unsigned)mv,
         status_name(imu_status), (long)sample->accel_mg[0], (long)sample->accel_mg[1],
         (long)sample->accel_mg[2], (long)sample->gyro_mdps[0], (long)sample->gyro_mdps[1],
         (long)sample->gyro_mdps[2], (long)sample->temperature_mc, (unsigned)app.alarm);
}

static void self_test(void)
{
  mg_status_t result = mpu6050_init(&imu, bus);
  uint16_t adc = 0;
  mg_status_t adc_result = adc_calibrated ? platform_adc(&adc) : MG_BUS_ERROR;
  printf("SELFTEST device=MPU6050 status=%s addr=0x68 expected_id=0x68\r\n",
         status_name(result));
  printf("SELFTEST device=LDR status=%s raw=%u\r\n",
         status_name(adc_result), (unsigned)adc);
  printf("SELFTEST device=PIR status=READABLE level=%u\r\n",
         (unsigned)platform_pir());
  printf("READY commands=c:read,n:nack,r:reset\r\n");
}

static void capture(void)
{
  uint8_t id = 0, frame[MPU6050_FRAME_SIZE] = {0};
  mg_status_t result = bus.read(bus.context, MPU6050_ADDRESS, 0x75, &id, 1);
  printf("CAPTURE op=id status=%s id=0x%02X\r\n", status_name(result), (unsigned)id);
  result = bus.read(bus.context, MPU6050_ADDRESS, 0x3B, frame, sizeof(frame));
  printf("CAPTURE op=frame status=%s bytes=", status_name(result));
  if (result == MG_OK) {
    for (size_t i = 0; i < sizeof(frame); ++i) { printf("%02X", (unsigned)frame[i]); }
  }
  printf("\r\n");
  if (result == MG_OK) {
    mpu6050_sample_t sample = {0};
    uint16_t adc = 0;
    mpu6050_decode(frame, sizeof(frame), &sample);
    mg_status_t adc_result = adc_calibrated ? platform_adc(&adc) : MG_BUS_ERROR;
    print_sample(platform_now(), adc, adc_result, MG_OK, &sample);
  }
}

void runner_start(void)
{
  printf("BOOT MuseGuard lab=2 bus=register-poll\r\n");
  bus = platform_i2c();
  adc_calibrated = platform_start() == MG_OK;
  museguard_reset(&app);
  platform_led(false);
  self_test();
  last_sample = last_telemetry = last_retry = platform_now();
}

void runner_poll(void)
{
  char command;
  uint32_t now = platform_now();
  if (platform_command(&command)) {
    if (command == 'c') { capture(); }
    if (command == 'n') {
      uint8_t id = 0;
      mg_status_t result = bus.read(bus.context, 0x69, 0x75, &id, 1);
      printf("ANOMALY t=%lu addr=0x69 status=%s\r\n",
             (unsigned long)platform_now(), status_name(result));
    }
    if (command == 'r') { runner_start(); }
  }
  now = platform_now();
  if ((uint32_t)(now - last_sample) < 20) { return; }
  last_sample = now;
  if (!imu.ready && (uint32_t)(now - last_retry) >= 1000) {
    last_retry = now;
    mpu6050_init(&imu, bus);
  }
  uint16_t adc = 0;
  mpu6050_sample_t sample = {0};
  mg_status_t adc_result = adc_calibrated ? platform_adc(&adc) : MG_BUS_ERROR;
  mg_status_t imu_result = mpu6050_read(&imu, &sample);
  if (imu_result != MG_OK) { imu.ready = false; }
  uint32_t events = museguard_update(&app, now, platform_pir(),
                     adc_result == MG_OK, adc, imu_result == MG_OK, &sample);
  static const struct { uint32_t flag; const char *name; } names[] = {
    {MG_PRESENCE, "PRESENCE"}, {MG_LIGHT, "LIGHT_CHANGE"}, {MG_SHOCK, "SHOCK"},
    {MG_TILT, "TILT"}, {MG_FAULT, "SENSOR_FAULT"}, {MG_RECOVERED, "SENSOR_RECOVERED"}
  };
  for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
    if ((events & names[i].flag) != 0) {
      printf("EVENT t=%lu type=%s alarm=%u\r\n", (unsigned long)now,
             names[i].name, (unsigned)app.alarm);
    }
  }
  platform_led(app.alarm);
  if ((uint32_t)(now - last_telemetry) >= 500) {
    last_telemetry = now;
    print_sample(now, adc, adc_result, imu_result, &sample);
  }
}
