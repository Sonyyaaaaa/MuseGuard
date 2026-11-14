#include "wokwi-api.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* External stimulus only: reproduce 30 baseline packets and PIR pulses.
   Every timing must also be checked against the captured input signals. */
static pin_t pir;
static uart_dev_t uart;
static timer_t timer;
static uint32_t ms, lcg=0x4d475333, pulse_at, pulse_off;
static unsigned trial;
static uint8_t packet[68], z[]="z", s[]="s";
static void tick(void *unused) {
  (void)unused;
  ++ms;
  if (ms==2000) uart_write(uart,z,1);
  if (trial<30 && ms==4000+trial*8000) {
    unsigned n=0;
    if (trial%10==0) packet[n++]='d';
    packet[n++]='a'; packet[n++]='g';
    for (unsigned i=0;i<64;i++) packet[n++]='n';
    packet[n++]='\n';
    uart_write(uart,packet,n);
    lcg=1664525u*lcg+1013904223u;
    pulse_at=ms+30+lcg%270;
    pulse_off=pulse_at+1000;
    ++trial;
  }
  if (ms==pulse_at && pulse_at) pin_write(pir,HIGH);
  if (ms==pulse_off && pulse_off) pin_write(pir,LOW);
  if (ms==250000) uart_write(uart,s,1);
}
void chip_init(void) {
  pir=pin_init("PIR",OUTPUT_LOW);
  pin_init("SDA",INPUT);
  const uart_config_t u={.tx=pin_init("TX",INPUT_PULLUP),.rx=pin_init("RX",INPUT),.baud_rate=115200};
  uart=uart_init(&u);
  const timer_config_t t={.callback=tick,.user_data=NULL};
  timer=timer_init(&t); timer_start(timer,1000,true);
}
