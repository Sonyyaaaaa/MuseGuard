#include "event_store.h"
#include "exhibit.h"
#include "cadence.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct { uint8_t bytes[MG_STORE_BYTES]; int remaining,erase_remaining; bool fail_erase; } flash_t;
static bool read_bytes(void *ctx,uint32_t offset,void *out,uint32_t size)
{flash_t *f=ctx;assert(offset+size<=sizeof(f->bytes));memcpy(out,f->bytes+offset,size);return true;}
static bool program(void *ctx,uint32_t offset,const void *data)
{
  flash_t *f=ctx;const uint8_t *p=data;
  for(unsigned i=0;i<8;++i){
    if(f->remaining==0)return false;
    if(f->remaining>0)--f->remaining;
    assert((f->bytes[offset+i]&p[i])==p[i]);f->bytes[offset+i]&=p[i];
  }
  return true;
}
static bool erase(void *ctx,uint32_t offset)
{
  flash_t *f=ctx;if(f->fail_erase)return false;
  for(unsigned i=0;i<MG_STORE_PAGE;++i){
    if(f->erase_remaining==0)return false;
    if(f->erase_remaining>0)--f->erase_remaining;
    f->bytes[offset+i]=255;
  }
  return true;
}
static mg_storage_t io(flash_t *f){return (mg_storage_t){f,read_bytes,program,erase};}
static flash_t empty(void){flash_t f;memset(&f,255,sizeof(f));f.remaining=-1;f.erase_remaining=-1;f.fail_erase=false;return f;}
static void power_cuts(void)
{
  for(int cut=0;cut<=32;++cut){
    flash_t f=empty();mg_store_t s;assert(mg_store_open(&s,io(&f)));
    assert(mg_store_append(&s,20,MG_PRESENCE,1000));
    f.remaining=cut;bool saved=mg_store_append(&s,40,MG_SHOCK,132072);
    assert(saved==(cut==32));f.remaining=-1;
    mg_store_t boot;assert(mg_store_open(&boot,io(&f)));
    assert(boot.sequence==(cut==32?2U:1U));
    assert(mg_store_append(&boot,60,MG_BOOT,0));
    mg_record_t record;assert(mg_store_read(&boot,0,&record));assert(mg_record_valid(&record));
  }
  puts("PASS store: 33 byte-boundary power-cut positions, preceding record preserved");
}
static void rollover(void)
{
  flash_t f=empty();mg_store_t s;assert(mg_store_open(&s,io(&f)));
  for(unsigned i=1;i<=320;++i){assert(mg_store_append(&s,i*20,MG_LIGHT,i));assert(s.sequence==i);}
  mg_store_t boot;assert(mg_store_open(&boot,io(&f)));assert(boot.sequence==320 && boot.valid==128);
  f.fail_erase=true;assert(!mg_store_append(&boot,0,MG_BOOT,0));
  assert(mg_store_open(&boot,io(&f)) && boot.sequence==320);
  f.fail_erase=false;assert(mg_store_append(&boot,0,MG_BOOT,0));
  f.bytes[MG_STORE_PAGE]^=1;assert(mg_store_open(&boot,io(&f)));assert(boot.sequence==320 && boot.corrupt==1);
  assert(mg_crc("123456789",9)==0xCBF43926U);
  puts("PASS store: 320 appends, page rotation, erase failure, bit corruption, CRC vector");
}
static void museum_policy(void)
{
  museguard_t detector={0};exhibit_t e={0};
  assert(exhibit_command(&e,&detector,'a',0xFFFF8000U)==MG_ACCESS_BEGIN);
  assert(exhibit_command(&e,&detector,'a',0)==0); /* no renewal by a command flood */
  detector.alarm=true;exhibit_apply(&e,&detector,false,MG_TILT);assert(!detector.alarm);
  detector.alarm=true;exhibit_apply(&e,&detector,false,MG_SHOCK|MG_TILT);assert(detector.alarm);
  assert(exhibit_command(&e,&detector,'k',1)==MG_ACK && !detector.alarm);
  assert(!exhibit_tick(&e,0xFFFF8000U+59999U));
  assert(exhibit_tick(&e,0xFFFF8000U+60000U)==MG_ACCESS_END && !e.access);
  detector.alarm=true;exhibit_apply(&e,&detector,false,MG_TILT);assert(detector.alarm);
  assert(exhibit_command(&e,&detector,'a',100)==MG_ACCESS_BEGIN);
  exhibit_apply(&e,&detector,true,MG_TILT);assert(detector.alarm);
  assert(exhibit_command(&e,&detector,'g',101)==MG_ACCESS_END && !e.access);
  assert(!exhibit_command(&e,&detector,'?',102));
  puts("PASS exhibit: 60-second window across tick wrap, no flood renewal, shock always alarm");
}
static void interrupted_rotation(void)
{
  flash_t seed=empty();mg_store_t s;
  assert(mg_store_open(&s,io(&seed)));
  for(unsigned i=1;i<=320;++i)assert(mg_store_append(&s,i,MG_PRESENCE,1000));
  for(int cut=0;cut<=2048;++cut){
    flash_t trial=seed;trial.erase_remaining=cut;
    assert(mg_store_open(&s,io(&trial)) && s.sequence==320);
    bool saved=mg_store_append(&s,321,MG_LIGHT,2000);
    assert(saved==(cut==2048));trial.erase_remaining=-1;
    assert(mg_store_open(&s,io(&trial)) && s.sequence==(cut==2048?321U:320U));
    assert(mg_store_append(&s,322,MG_BOOT,0));
  }
  for(int cut=0;cut<=32;++cut){
    flash_t trial=seed;trial.remaining=cut;
    assert(mg_store_open(&s,io(&trial)));
    bool saved=mg_store_append(&s,321,MG_SHOCK,132072);
    assert(saved==(cut==32));trial.remaining=-1;
    assert(mg_store_open(&s,io(&trial)) && s.sequence==(cut==32?321U:320U));
    assert(mg_store_append(&s,322,MG_BOOT,0));
  }
  puts("PASS rotation: 2049 erase-cut boundaries and 33 rollover program-cut boundaries (host model)");
}
static void deadlines(void)
{
  cadence_t t={.period=20000,.deadline=20000,.due=0xFFFF0000U};
  assert(!cadence_release(&t,t.due-1));assert(cadence_release(&t,t.due));
  uint32_t nominal=t.due;
  assert(cadence_release(&t,nominal+45000));assert(t.skipped==2 && t.late==1);
  assert(t.max_lateness==45000 && t.jitter_max==45000);
  cadence_complete(&t,nominal+46000);assert(t.completion_misses==1);
  assert(!cadence_release(&t,nominal+45000));
  for(unsigned i=0;i<180000;++i)assert(cadence_release(&t,t.due));
  assert(t.count==180002);
  duration_t s={0};duration_add(&s,9);duration_add(&s,2);duration_add(&s,10);
  assert(s.count==3 && s.min==2 && s.max==10 && s.sum==21);
  puts("PASS cadence: skip accounting and wrap; 180000 logical releases (not a one-hour MCU run)");
}
int main(void){power_cuts();rollover();museum_policy();deadlines();interrupted_rotation();return 0;}
