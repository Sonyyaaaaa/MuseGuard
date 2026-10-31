#ifndef MG_EVENT_STORE_H
#define MG_EVENT_STORE_H
#include <stdint.h>
#include <stdbool.h>
#define MG_STORE_BYTES 4096U
#define MG_STORE_PAGE 2048U
#define MG_STORE_RECORD 32U
typedef struct {
  uint32_t magic, sequence, milliseconds, events, observation, reserved, crc, commit;
} mg_record_t;
typedef struct {
  void *context;
  bool (*read)(void *, uint32_t, void *, uint32_t);
  bool (*program8)(void *, uint32_t, const void *);
  bool (*erase)(void *, uint32_t);
} mg_storage_t;
typedef struct { mg_storage_t io; uint32_t sequence, next, valid, corrupt; } mg_store_t;
uint32_t mg_crc(const void *bytes, uint32_t length);
bool mg_record_valid(const mg_record_t *record);
bool mg_store_open(mg_store_t *store, mg_storage_t io);
bool mg_store_append(mg_store_t *store, uint32_t time, uint32_t events, uint32_t observation);
bool mg_store_read(const mg_store_t *store, uint32_t slot, mg_record_t *record);
#endif
