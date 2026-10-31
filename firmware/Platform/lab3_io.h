#ifndef MG_LAB3_IO_H
#define MG_LAB3_IO_H
#include "event_store.h"
void lab3_io_init(void);
uint32_t lab3_us(void);
uint32_t lab3_clock_ambiguous(void);
void lab3_paint_stack(void);
uint32_t lab3_stack_used(void);
uint32_t lab3_heap_peak(void);
bool lab3_rx(char *command,uint32_t *received);
void lab3_rx_stats(uint32_t *dropped,uint32_t *overruns);
void lab3_probe(unsigned pin,bool high);
mg_storage_t lab3_storage(void);
#endif
