#include "museguard.h"
#include "exhibit.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 for(unsigned n=0;n<30;n++){
  museguard_t d;exhibit_t e={0};museguard_reset(&d);
  uint32_t start=0xffff0000U+n*7919U;
  mpu6050_sample_t base={.accel_mg={0,0,1000}},tilt={.accel_mg={600,0,800}};
  museguard_update(&d,start,false,true,1001,true,&base);
  assert(exhibit_command(&e,&d,'a',start)==MG_ACCESS_BEGIN);
  for(unsigned ms=20;ms<=520;ms+=20){bool old=d.alarm;uint32_t ev=museguard_update(&d,start+ms,false,true,1001,true,&tilt);exhibit_apply(&e,&d,old,ev);}
  assert(d.tilt_active&&!d.alarm);
  d.alarm=true;int32_t reference[3];memcpy(reference,d.accel_base,sizeof reference);
  assert(exhibit_command(&e,&d,'k',start+540)==MG_ACK&&!d.alarm);
  assert(!memcmp(reference,d.accel_base,sizeof reference)&&d.tilt_active);
  for(unsigned ms=540;ms<=2540;ms+=20){bool old=d.alarm;uint32_t ev=museguard_update(&d,start+ms,false,true,1001,true,&tilt);exhibit_apply(&e,&d,old,ev);assert(!(ev&MG_TILT)&&!d.alarm);}
  assert(!exhibit_command(&e,&d,'a',start+59000));
  assert(!exhibit_tick(&e,start+59999));assert(exhibit_tick(&e,start+60000)==MG_ACCESS_END);
 }
 puts("PASS policy: 30 wrap-shifted trials; ACCESS TILT, prior alarm, ACK preserves tilt latch/reference, no retrigger during unchanged tilt; first-command 60000ms expiry/no renewal; 3840 periodic host updates");
}
