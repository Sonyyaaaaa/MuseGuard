#include "wokwi-api.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Test fixture, not a model of MPU6050 power-up or physical timing. */
typedef struct {
  uint8_t reg;
  bool expect_reg;
  bool first_ready;
} state_t;

static bool connect_cb(void *user_data, uint32_t address, bool read) {
  state_t *s = user_data;
  bool ready = get_sim_nanos() >= 5000000000ULL;
  if (!ready) {
    printf("NOT_READY t_ns=%llu addr=%lu NACK\n",
           (unsigned long long)get_sim_nanos(), (unsigned long)address);
    return false;
  }
  if (!s->first_ready) {
    printf("READY t_ns=%llu addr=%lu ACK\n",
           (unsigned long long)get_sim_nanos(), (unsigned long)address);
    s->first_ready = true;
  }
  if (!read) s->expect_reg = true;
  return true;
}

static bool write_cb(void *user_data, uint8_t data) {
  state_t *s = user_data;
  if (s->expect_reg) {
    s->reg = data;
    s->expect_reg = false;
  } else {
    s->reg++;
  }
  return true;
}

static uint8_t read_cb(void *user_data) {
  state_t *s = user_data;
  uint8_t reg = s->reg++;
  if (reg == 0x75) return 0x68;
  if (reg == 0x3f) return 0x40; /* +1 g on Z, big endian */
  return 0;
}

void chip_init(void) {
  state_t *s = calloc(1, sizeof(*s));
  const i2c_config_t config = {
    .address = 0x68,
    .scl = pin_init("SCL", INPUT_PULLUP),
    .sda = pin_init("SDA", INPUT_PULLUP),
    .connect = connect_cb,
    .read = read_cb,
    .write = write_cb,
    .user_data = s,
  };
  i2c_init(&config);
}
