#ifndef MUSEGUARD_BUS_H
#define MUSEGUARD_BUS_H
#include <stddef.h>
#include <stdint.h>

typedef enum {
  MG_OK, MG_INVALID, MG_BUS_ERROR, MG_TIMEOUT, MG_BAD_ID
} mg_status_t;

typedef struct {
  void *context;
  mg_status_t (*read)(void *, uint8_t, uint8_t, uint8_t *, size_t);
  mg_status_t (*write)(void *, uint8_t, uint8_t, const uint8_t *, size_t);
} mg_bus_t;
#endif
