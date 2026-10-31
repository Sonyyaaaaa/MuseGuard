#include "runner.h"
#include "platform.h"
#include "lab3_io.h"
#include "exhibit.h"
#include "cadence.h"
#include "build_id.h"
#include <stdio.h>
#include <string.h>
static museguard_t detector;
static exhibit_t exhibit;
static mpu6050_t imu;
static mg_bus_t bus;
static mg_store_t store;
static bool adc_ready,store_ready,inject_fault;
static uint16_t last_adc;
static uint32_t events_lost,flash_failures,report_row,dump_slot,window_start,last_loop;
static uint64_t busy_us;
static bool reporting,dumping,reset_metrics;
static cadence_t sample_clock={.period=20000,.deadline=20000};
static cadence_t recovery_clock={.period=1000000,.deadline=100000};
static cadence_t heartbeat_clock={.period=1000000,.deadline=100000};
enum {SAMPLE,RECOVERY,COMMAND,JOURNAL,OUTPUT,LOOP,LATENCY,PERIOD,NSTATS};
static duration_t timing[NSTATS];
static duration_t report_timing[NSTATS];
static cadence_t report_clock;
static uint32_t report_load;
static const char *const task_names[]={"sample","recovery","command","journal","output","loop","command_latency","loop_period"};
typedef struct {uint32_t time,events,observation;} pending_t;
static pending_t queue[16];static unsigned qhead,qtail,qcount;
static void remember(uint32_t time,uint32_t events)
{
  if(!events)return;
  if(events&MG_ACCESS_END) {detector.tilt_active=false;detector.tilt_pending=false;}
  if(qcount==16){++events_lost;return;}
  queue[qhead]=(pending_t){time,events,(uint32_t)last_adc|((uint32_t)exhibit.access<<16)|((uint32_t)detector.alarm<<17)};
  qhead=(qhead+1)%16;++qcount;
}
static void account(unsigned index,uint32_t start)
{
  uint32_t elapsed=lab3_us()-start;duration_add(&timing[index],elapsed);busy_us+=elapsed;
}
void runner_start(void)
{
  lab3_io_init(); bus=platform_i2c();adc_ready=platform_start()==MG_OK;
  museguard_reset(&detector);platform_led(false);
  store_ready=mg_store_open(&store,lab3_storage());
  window_start=lab3_us();last_loop=window_start;
  sample_clock.due=heartbeat_clock.due=recovery_clock.due=window_start;
  printf("BOOT MuseGuard lab=3 core=C031 source=%s git=%s clock=SysTick_1ms store=%u seq=%lu corrupt=%lu\r\n",MG_BUILD_ID,MG_GIT_ID,(unsigned)store_ready,(unsigned long)store.sequence,(unsigned long)store.corrupt);
  printf("READY mode=GUARD commands=a:access60s,g:guard,k:ack,c:read,n:nack,f:fault,v:recover,s:stats,d:dump,z:metrics\r\n");
  remember(platform_now(),MG_BOOT);
}
static void sample_task(void)
{
  mpu6050_sample_t sample={0};
  mg_status_t adc=adc_ready?platform_adc(&last_adc):MG_BUS_ERROR;
  mg_status_t motion=inject_fault?MG_BUS_ERROR:mpu6050_read(&imu,&sample);
  if(motion!=MG_OK)imu.ready=false;
  bool old_alarm=detector.alarm;
  uint32_t events=museguard_update(&detector,platform_now(),platform_pir(),adc==MG_OK,last_adc,motion==MG_OK,&sample);
  exhibit_apply(&exhibit,&detector,old_alarm,events);
  remember(platform_now(),events);platform_led(detector.alarm);
}
static void command_task(char c,uint32_t received)
{
  lab3_probe(8,true);
  duration_add(&timing[LATENCY],lab3_us()-received);
  remember(platform_now(),exhibit_command(&exhibit,&detector,c,platform_now()));
  if(c=='s'){
    memcpy(report_timing,timing,sizeof(timing));report_clock=sample_clock;
    uint32_t span=lab3_us()-window_start;
    report_load=(uint32_t)(span?busy_us*1000/span:0);
    reporting=true;report_row=0;
  }
  else if(c=='d'){dumping=true;dump_slot=0;}
  else if(c=='f'){inject_fault=true;}
  else if(c=='v'){inject_fault=false;}
  else if(c=='z') {reset_metrics=true;}
  else if(c=='c'||c=='n') {
    uint8_t id=0;mg_status_t result=bus.read(bus.context,c=='n'?0x69:0x68,0x75,&id,1);
    printf("READ t_us=%lu status=%u id=%u\r\n",(unsigned long)lab3_us(),(unsigned)result,(unsigned)id);
  }
  if(c!='\r' && c!='\n')printf("CMD t_us=%lu rx_us=%lu char=%c access=%u alarm=%u\r\n",(unsigned long)lab3_us(),(unsigned long)received,c,(unsigned)exhibit.access,(unsigned)detector.alarm);
  platform_led(detector.alarm);lab3_probe(8,false);
}
static void journal_task(void)
{
  pending_t entry=queue[qtail];qtail=(qtail+1)%16;--qcount;
  uint32_t started=lab3_us();lab3_probe(7,true);
  bool saved=store_ready&&mg_store_append(&store,entry.time,entry.events,entry.observation);
  lab3_probe(7,false);uint32_t elapsed=lab3_us()-started;
  if(!saved){++flash_failures;store_ready=false;}
  printf("EVENT t_ms=%lu bits=%lu observation=%lu saved=%u seq=%lu flash_us=%lu\r\n",(unsigned long)entry.time,(unsigned long)entry.events,(unsigned long)entry.observation,(unsigned)saved,(unsigned long)store.sequence,(unsigned long)elapsed);
}
static void output_task(void)
{
  if(reporting){
    if(report_row<NSTATS){
      duration_t s=report_timing[report_row];
      printf("STAT name=%s n=%lu min_us=%lu avg_us=%lu max_us=%lu\r\n",task_names[report_row],(unsigned long)s.count,(unsigned long)s.min,(unsigned long)(s.count?s.sum/s.count:0),(unsigned long)s.max);++report_row;
    }else{
      uint32_t drop,ore;lab3_rx_stats(&drop,&ore);
      printf("RESOURCE stack=%lu heap_peak=%lu rx_drop=%lu ore=%lu event_drop=%lu flash_fail=%lu clock_ambiguous=%lu\r\n",(unsigned long)lab3_stack_used(),(unsigned long)lab3_heap_peak(),(unsigned long)drop,(unsigned long)ore,(unsigned long)events_lost,(unsigned long)flash_failures,(unsigned long)lab3_clock_ambiguous());
      printf("CADENCE n=%lu skipped=%lu late=%lu finish_miss=%lu release_max_us=%lu jitter_min_us=%ld jitter_max_us=%ld busy_permille=%lu\r\n",(unsigned long)report_clock.count,(unsigned long)report_clock.skipped,(unsigned long)report_clock.late,(unsigned long)report_clock.completion_misses,(unsigned long)report_clock.max_lateness,(long)report_clock.jitter_min,(long)report_clock.jitter_max,(unsigned long)report_load);
      reporting=false;
    }
  }else if(dumping){
    mg_record_t r;
    if(mg_store_read(&store,dump_slot,&r)&&mg_record_valid(&r))printf("RECORD slot=%lu seq=%lu t_ms=%lu bits=%lu observation=%lu crc=%08lX\r\n",(unsigned long)dump_slot,(unsigned long)r.sequence,(unsigned long)r.milliseconds,(unsigned long)r.events,(unsigned long)r.observation,(unsigned long)r.crc);
    if(++dump_slot==MG_STORE_BYTES/MG_STORE_RECORD)dumping=false;
  }
}
void runner_poll(void)
{
  if(reset_metrics) {
    uint32_t now=lab3_us();
    memset(timing,0,sizeof(timing));busy_us=0;window_start=now;last_loop=now;
    sample_clock=(cadence_t){.period=20000,.deadline=20000,.due=now};
    recovery_clock=(cadence_t){.period=1000000,.deadline=100000,.due=now};
    heartbeat_clock=(cadence_t){.period=1000000,.deadline=100000,.due=now};
    reporting=dumping=reset_metrics=false;
  }
  uint32_t loop_start=lab3_us(),start=loop_start;
  duration_add(&timing[PERIOD],loop_start-last_loop);last_loop=loop_start;
  remember(platform_now(),exhibit_tick(&exhibit,platform_now()));
  /* Sample first, then at most one command and one journal record. No catch-up burst. */
  if(cadence_release(&sample_clock,start)){
    lab3_probe(6,true);sample_task();lab3_probe(6,false);account(SAMPLE,start);cadence_complete(&sample_clock,lab3_us());
  }
  start=lab3_us();
  if(cadence_release(&recovery_clock,start) && (!imu.ready || !adc_ready)){
    if(!inject_fault && !imu.ready)mpu6050_init(&imu,bus);
    if(!adc_ready)adc_ready=platform_start()==MG_OK;
    account(RECOVERY,start);
  }
  char command;uint32_t received;start=lab3_us();
  if(lab3_rx(&command,&received)){command_task(command,received);account(COMMAND,start);}
  start=lab3_us();if(qcount){journal_task();account(JOURNAL,start);}
  start=lab3_us();
  if(reporting||dumping){output_task();account(OUTPUT,start);}
  start=lab3_us();
  if(cadence_release(&heartbeat_clock,start)){
    printf("HEART t_us=%lu adc=%u fault=%u alarm=%u access=%u\r\n",(unsigned long)start,(unsigned)last_adc,(unsigned)detector.fault,(unsigned)detector.alarm,(unsigned)exhibit.access);account(OUTPUT,start);
  }
  duration_add(&timing[LOOP],lab3_us()-loop_start);
}
