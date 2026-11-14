#include "wokwi-api.h"
#include <stdbool.h>
#include <stdint.h>
typedef struct { uint8_t reg; bool first; } state_t;
static bool connect_cb(void *u,uint32_t addr,bool read) { (void)addr;if(!read)((state_t*)u)->first=true;return true; }
static bool write_cb(void *u,uint8_t b) { state_t*s=u;if(s->first){s->reg=b;s->first=false;}else ++s->reg;return true; }
static uint8_t read_cb(void *u) {
  uint8_t r=((state_t*)u)->reg++;
  uint64_t ms=get_sim_nanos()/1000000ULL;
  bool shock=false,tilt=false;
  if(ms>=5000 && ms<35000) { unsigned n=(ms-5000)/1000,p=(ms-5000)%1000;shock=p>=n%20 && p<n%20+20; }
  if(ms>=35000 && ms<95000) { unsigned n=(ms-35000)/1000,p=(ms-35000)%1000;shock=(n%7+1)&4;shock=shock && p<120; }
  if(ms>=94500 && ms<94620)shock=true;
  if(ms>=95000 && ms<155000)tilt=(ms-95000)%1000<700;
  int16_t x=tilt?9830:0,z=tilt?13107:(shock?32767:16384);
  if(r==0x75)return 0x68;
  if(r==0x3b)return (uint16_t)x>>8;
  if(r==0x3c)return x&255;
  if(r==0x3f)return (uint16_t)z>>8;
  if(r==0x40)return z&255;
  return 0;
}
void chip_init(void) {
  static state_t state;
  const i2c_config_t c={.address=0x68,.scl=pin_init("SCL",INPUT_PULLUP),.sda=pin_init("SDA",INPUT_PULLUP),.connect=connect_cb,.read=read_cb,.write=write_cb,.user_data=&state};
  i2c_init(&c);
}
