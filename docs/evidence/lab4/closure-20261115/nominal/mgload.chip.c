#include "wokwi-api.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
static uart_dev_t uart;
static pin_t pir, mark, out;
static unsigned ms;
static void send(char c) { uint8_t b=(uint8_t)c; uart_write(uart,&b,1); }
static void tick(void *u) {
 (void)u;++ms;
 unsigned phase=ms>=5000?(ms-5000)%1000:1000;
 bool active=ms>=5000 && ms<65000 && phase<200;
 pin_write(pir,active);pin_write(mark,active);
 pin_dac_write(out,active?1.9f:1.0f);
 if(ms==3000)send('z');
 if(ms>=5000 && ms<65000 && phase==0)send(((ms-5000)/1000)%2 ? 'k' : 'n');

 if(ms==68000)send('d');if(ms==73000)send('s');
}
void chip_init(void) {
  pir=pin_init("PIR",OUTPUT_LOW);mark=pin_init("MARK",OUTPUT_LOW);
  out=pin_init("OUT",ANALOG);pin_dac_write(out,1.0f);
  pin_init("SDA",INPUT);
  const uart_config_t config={.tx=pin_init("TX",INPUT_PULLUP),.rx=pin_init("RX",INPUT),.baud_rate=115200};
  uart=uart_init(&config);
  const timer_config_t t={.callback=tick,.user_data=NULL};
  timer_start(timer_init(&t),1000,true);
}
