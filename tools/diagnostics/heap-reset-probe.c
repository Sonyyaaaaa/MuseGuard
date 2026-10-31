#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "main.h"

extern void __real_runner_start(void);
extern uint32_t __real_lab3_heap_peak(void);
extern bool __real_lab3_rx(char *, uint32_t *);

void __wrap_runner_start(void)
{
  __real_runner_start();
  printf("DIAG kind=heap-reset-probe-v1 base_source=f8a557c75edecf2461b712bfe0667b9c0069caccdc3f814856416e294387d3e4 extra=heap-reset-probe.c command_r=NVIC_SystemReset\r\n");
}

uint32_t __wrap_lab3_heap_peak(void)
{
  struct mallinfo m=mallinfo();
  uint32_t arena=__real_lab3_heap_peak();
  printf("HEAPDIAG arena=%lu allocated_chunks=%lu free_chunks=%lu ordblks_raw=%lu sbrk_arena=%lu\r\n",
    (unsigned long)m.arena,(unsigned long)m.uordblks,(unsigned long)m.fordblks,
    (unsigned long)m.ordblks,(unsigned long)arena);
  return arena;
}

bool __wrap_lab3_rx(char *c,uint32_t *received)
{
  bool ready=__real_lab3_rx(c,received);
  if(ready && *c=='r') {
    printf("RESETDIAG request=NVIC_SystemReset rx_us=%lu\r\n",(unsigned long)*received);
    NVIC_SystemReset();
  }
  return ready;
}
