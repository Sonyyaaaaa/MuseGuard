#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "runner.c"
static uint8_t media[MG_STORE_BYTES];
static uint32_t now;
static bool rd(void*c,uint32_t off,void*out,uint32_t n){(void)c;memcpy(out,media+off,n);return true;}
static bool wr(void*c,uint32_t off,const void*in){(void)c;const uint8_t*p=in;for(unsigned i=0;i<8;i++){assert((media[off+i]&p[i])==p[i]);media[off+i]&=p[i];}return true;}
static bool er(void*c,uint32_t off){(void)c;memset(media+off,255,MG_STORE_PAGE);return true;}
mg_storage_t lab3_storage(void){return (mg_storage_t){0,rd,wr,er};}
void lab3_io_init(void){}
uint32_t lab3_us(void){return now*1000;}
uint32_t lab3_clock_ambiguous(void){return 0;}
uint32_t lab3_stack_used(void){return 0;}
uint32_t lab3_heap_peak(void){return 0;}
bool lab3_rx(char*c,uint32_t*t){(void)c;(void)t;return false;}
void lab3_rx_stats(uint32_t*d,uint32_t*o){*d=*o=0;}
void lab3_probe(unsigned pin,bool high){(void)pin;(void)high;}
mg_bus_t platform_i2c(void){return (mg_bus_t){0};}
mg_status_t platform_adc(uint16_t*v){*v=1001;return MG_OK;}
bool platform_pir(void){return false;}
void platform_led(bool b){(void)b;}
uint32_t platform_now(void){return now;}
mg_status_t platform_start(void){return MG_OK;}
int main(void){
 for(unsigned trial=0;trial<30;trial++){
  memset(media,255,sizeof media);assert(mg_store_open(&store,lab3_storage()));store_ready=true;
  qhead=qtail=qcount=events_lost=flash_failures=0;last_adc=1001;
  for(unsigned i=0;i<24;i++)remember(1000+i,MG_PRESENCE);
  assert(qcount==16 && events_lost==8);
  for(unsigned i=0;i<4;i++)journal_task();
  for(unsigned i=0;i<4;i++)remember(2000+i,MG_LIGHT);
  while(qcount)journal_task();
  assert(store.sequence==20 && flash_failures==0 && events_lost==8);
  for(unsigned i=0;i<20;i++){
   mg_record_t r;assert(mg_store_read(&store,i,&r)&&mg_record_valid(&r));
   assert(r.milliseconds==(i<16?1000+i:2000+i-16));
   assert(r.events==(i<16?MG_PRESENCE:MG_LIGHT));
  }
 }
 puts("PASS product runner queue: 30 repetitions, capacity 16, 8/24 dropped, FIFO preserved through drain/refill, 600 CRC-valid records; host fake storage, no RTOS timing claim");
}
