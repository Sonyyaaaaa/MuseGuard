#include "wokwi-api.h"
#include <stdint.h>
#include <stddef.h>
static uart_dev_t uart;
static pin_t pir, mark, out;
static unsigned ms;
static void send(char c) { uint8_t b=(uint8_t)c; uart_write(uart,&b,1); }
static void tick(void *u) {
  (void)u; ++ms;
  unsigned mask=0, active=0;
  if(ms>=5000 && ms<35000) {
    unsigned n=(ms-5000)/1000, p=(ms-5000)%1000;
    active=p>=n%20 && p<n%20+20; mask=4;
    if(p==0) { uint8_t burst[16]; for(unsigned i=0;i<16;i++)burst[i]='n';uart_write(uart,burst,16); }
    if(p==500)send('k');
  } else if(ms>=35000 && ms<95000) {
    unsigned n=(ms-35000)/1000, p=(ms-35000)%1000;
    mask=n%7+1;active=p<120;
    if(p==500)send('k');
  } else if(ms>=95000 && ms<155000) {
    unsigned n=(ms-95000)/1000,p=(ms-95000)%1000;
    mask=(n%4);active=p<700;
    if(p==800)send('k');
  }
  pin_write(pir,active && (mask&1));
  pin_dac_write(out,active && (mask&2)?1.9f:1.0f);
  pin_write(mark,active);
  if(ms==3000)send('z');
  if(ms==64000)send('a');
  if(ms==125000)send('g');
  if(ms==157000)send('d');
  if(ms==162000)send('s');
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
