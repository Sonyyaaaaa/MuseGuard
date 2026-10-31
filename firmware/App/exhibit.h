#ifndef MG_EXHIBIT_H
#define MG_EXHIBIT_H
#include "museguard.h"
enum { MG_ACCESS_BEGIN=64U, MG_ACCESS_END=128U, MG_ACK=256U, MG_BOOT=512U };
typedef struct { bool access; uint32_t opened_at; } exhibit_t;
uint32_t exhibit_command(exhibit_t *state, museguard_t *detector, char command, uint32_t now);
uint32_t exhibit_tick(exhibit_t *state, uint32_t now);
void exhibit_apply(const exhibit_t *state, museguard_t *detector, bool previous_alarm, uint32_t events);
#endif
