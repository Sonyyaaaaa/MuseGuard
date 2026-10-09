#include "museguard.h"
#include "fixtures/mpu6050_capture.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

typedef struct {
  mg_status_t fail;
  size_t calls, fail_at, writes;
  uint8_t id, frame[14], registers[5], values[5];
} fake_t;

static mg_status_t fake_read(void *ctx, uint8_t address, uint8_t reg,
                             uint8_t *data, size_t size)
{
  fake_t *fake = ctx;
  assert(address == 0x68);
  ++fake->calls;
  if (fake->fail_at == fake->calls) { return fake->fail; }
  if (reg == 0x75) { assert(size == 1); *data = fake->id; }
  else { assert(reg == 0x3B && size == 14); memcpy(data, fake->frame, 14); }
  return MG_OK;
}
static mg_status_t fake_write(void *ctx, uint8_t address, uint8_t reg,
                              const uint8_t *data, size_t size)
{
  fake_t *fake = ctx;
  assert(address == 0x68 && size == 1);
  ++fake->calls;
  if (fake->fail_at == fake->calls) { return fake->fail; }
  assert(fake->writes < 5);
  fake->registers[fake->writes] = reg;
  fake->values[fake->writes++] = *data;
  return MG_OK;
}
static mg_bus_t fake_bus(fake_t *fake)
{
  mg_bus_t bus = {fake, fake_read, fake_write};
  return bus;
}

static void test_initialization(void)
{
  fake_t fake = {.id=0x68};
  mpu6050_t imu = {0};
  assert(mpu6050_init(&imu, fake_bus(&fake)) == MG_OK && imu.ready);
  const uint8_t regs[] = {0x6B,0x1A,0x1B,0x1C,0x19}, values[] = {0,3,0,0,9};
  assert(fake.writes == 5);
  assert(memcmp(regs, fake.registers, 5) == 0);
  assert(memcmp(values, fake.values, 5) == 0);
  for (size_t call = 1; call <= 6; ++call) {
    for (mg_status_t error = MG_BUS_ERROR; error <= MG_TIMEOUT; ++error) {
      fake = (fake_t){.id=0x68,.fail_at=call,.fail=error};
      assert(mpu6050_init(&imu, fake_bus(&fake)) == error);
      assert(!imu.ready && fake.calls == call);
    }
  }
  fake = (fake_t){.id=0x69};
  assert(mpu6050_init(&imu, fake_bus(&fake)) == MG_BAD_ID);
  assert(fake.writes == 0 && !imu.ready);
  mg_bus_t empty = {0};
  assert(mpu6050_init(&imu, empty) == MG_INVALID);
  assert(mpu6050_init(NULL, empty) == MG_INVALID);
}

static void test_conversion(void)
{
  uint8_t frame[14] = {0x40,0,0xC0,0,0,0,0,0,0,0x83,0xFF,0x7D,0,0};
  mpu6050_sample_t sample = {0};
  assert(mpu6050_decode(frame,14,&sample) == MG_OK);
  assert(sample.accel_mg[0] == 1000 && sample.accel_mg[1] == -1000);
  assert(sample.gyro_mdps[0] == 1000 && sample.gyro_mdps[1] == -1000);
  assert(sample.temperature_mc == 36530);
  frame[0]=0x80; frame[1]=0; frame[2]=0x7F; frame[3]=0xFF;
  assert(mpu6050_decode(frame,14,&sample) == MG_OK);
  assert(sample.accel_raw[0] == INT16_MIN && sample.accel_raw[1] == INT16_MAX);
  assert(sample.accel_mg[0] == -2000 && sample.accel_mg[1] == 1999);
  assert(mpu6050_decode(NULL,14,&sample) == MG_INVALID);
  assert(mpu6050_decode(frame,13,&sample) == MG_INVALID);
  assert(mpu6050_decode(frame,14,NULL) == MG_INVALID);
}

static void test_read_error(void)
{
  fake_t fake = {.id=0x68,.frame={0,0,0,0,0x40,0}};
  mpu6050_t imu = {0};
  mpu6050_sample_t sample = {0};
  assert(mpu6050_read(&imu,&sample) == MG_INVALID);
  assert(mpu6050_read(NULL,&sample) == MG_INVALID);
  assert(mpu6050_init(&imu,fake_bus(&fake)) == MG_OK);
  assert(mpu6050_read(&imu,NULL) == MG_INVALID);
  assert(mpu6050_read(&imu,&sample) == MG_OK && sample.accel_mg[2] == 1000);
  for (mg_status_t error=MG_BUS_ERROR; error<=MG_TIMEOUT; ++error) {
    fake.fail_at=fake.calls+1;fake.fail=error;
    mpu6050_sample_t before=sample;
    assert(mpu6050_read(&imu,&sample)==error);
    assert(memcmp(&before,&sample,sizeof(sample))==0);
  }
}

static void test_simple_sensors(void)
{
  pir_t pir={0};
  assert(!pir_rising(&pir,false));
  assert(pir_rising(&pir,true));
  assert(!pir_rising(&pir,true));
  assert(!pir_rising(&pir,false));
  assert(pir_rising(&pir,true));
  uint16_t mv=99;
  assert(light_millivolts(0,&mv)==MG_OK && mv==0);
  assert(light_millivolts(4095,&mv)==MG_OK && mv==3300);
  assert(light_millivolts(2048,&mv)==MG_OK && mv==1650);
  assert(light_millivolts(4096,&mv)==MG_INVALID && mv==1650);
  assert(light_millivolts(0,NULL)==MG_INVALID);
}

static mpu6050_sample_t rest(void)
{
  mpu6050_sample_t sample={0};
  sample.accel_mg[2]=1000;
  return sample;
}
static uint32_t update(museguard_t *app,uint32_t time,bool pir,uint16_t adc,
                       const mpu6050_sample_t *sample)
{
  return museguard_update(app,time,pir,true,adc,true,sample);
}

static void test_events(void)
{
  museguard_t app;
  museguard_reset(&app);
  mpu6050_sample_t sample=rest();
  assert(update(&app,0,false,1000,&sample)==0);
  assert(update(&app,20,true,1000,&sample)==MG_PRESENCE && !app.alarm);
  assert(update(&app,40,true,1600,&sample)==MG_LIGHT && !app.alarm);
  assert(update(&app,60,true,1600,&sample)==0);
  assert(update(&app,80,false,1400,&sample)==0);
  assert(update(&app,100,false,1399,&sample)==0);
  assert(update(&app,120,false,400,&sample)==MG_LIGHT);
  sample.accel_mg[2]=1600;
  assert((update(&app,140,false,400,&sample)&MG_SHOCK)!=0 && app.alarm);
  assert(update(&app,160,false,400,&sample)==0);
  sample=rest();update(&app,180,false,400,&sample);
  sample.accel_mg[2]=0;
  assert((update(&app,200,false,400,&sample)&MG_SHOCK)!=0);
  museguard_reset(&app);
  assert(!app.alarm);
}

static void test_tilt_and_time(void)
{
  museguard_t app;
  museguard_reset(&app);
  mpu6050_sample_t sample=rest();
  update(&app,0,false,1000,&sample);
  sample.accel_mg[0]=600;sample.accel_mg[2]=800;
  assert(update(&app,20,false,1000,&sample)==0);
  assert(update(&app,519,false,1000,&sample)==0);
  assert(update(&app,520,false,1000,&sample)==MG_TILT && app.alarm);
  assert(update(&app,540,false,1000,&sample)==0);
  sample=rest();update(&app,560,false,1000,&sample);
  sample.accel_mg[0]=600;sample.accel_mg[2]=800;
  update(&app,580,false,1000,&sample);
  assert(update(&app,1080,false,1000,&sample)==MG_TILT);
  museguard_reset(&app);sample=rest();update(&app,UINT32_MAX-300,false,1000,&sample);
  sample.accel_mg[0]=600;sample.accel_mg[2]=800;
  update(&app,UINT32_MAX-200,false,1000,&sample);
  assert(update(&app,299,false,1000,&sample)==MG_TILT);
  museguard_reset(&app);sample=rest();update(&app,0,false,1000,&sample);
  sample.accel_mg[0]=600;sample.accel_mg[2]=800;update(&app,20,false,1000,&sample);
  sample=rest();update(&app,400,false,1000,&sample);
  sample.accel_mg[0]=600;sample.accel_mg[2]=800;update(&app,420,false,1000,&sample);
  assert(update(&app,520,false,1000,&sample)==0);
}

static void test_fault(void)
{
  museguard_t app;
  museguard_reset(&app);
  mpu6050_sample_t sample=rest();
  assert(museguard_update(&app,0,false,false,0,false,NULL)==MG_FAULT);
  assert(museguard_update(&app,20,true,false,0,false,NULL)==MG_PRESENCE);
  assert(museguard_update(&app,30,true,true,1000,true,NULL)==0 && app.fault);
  assert(update(&app,40,true,1000,&sample)==MG_RECOVERED);
  sample.accel_mg[0]=600;sample.accel_mg[2]=800;update(&app,60,true,1000,&sample);
  assert(museguard_update(&app,300,true,true,1000,false,NULL)==MG_FAULT);
  assert(!app.tilt_pending);
  assert(update(&app,600,true,1000,&sample)==MG_RECOVERED);
  assert(update(&app,1099,true,1000,&sample)==0);
  assert(update(&app,1100,true,1000,&sample)==MG_TILT);
}

static void test_captured_samples(void)
{
  mpu6050_sample_t sample;
  museguard_t app;
  museguard_reset(&app);
  assert(mpu6050_decode(captured_rest,14,&sample)==MG_OK);
  assert(sample.accel_mg[0]==0 && sample.accel_mg[1]==0 && sample.accel_mg[2]==1000);
  assert(sample.temperature_mc==24001);
  assert(sample.gyro_mdps[0]==0 && sample.gyro_mdps[1]==0 && sample.gyro_mdps[2]==0);
  assert(update(&app,74151,false,1001,&sample)==0);
  assert(mpu6050_decode(captured_tilt,14,&sample)==MG_OK);
  assert(sample.accel_raw[0]==9830 && sample.accel_raw[2]==13107);
  assert(sample.accel_mg[0]==599 && sample.accel_mg[2]==799);
  assert(update(&app,74171,false,1001,&sample)==0);
  assert(update(&app,74670,false,1001,&sample)==0);
  assert(update(&app,74671,false,1001,&sample)==MG_TILT && app.alarm);
}

int main(void)
{
  test_initialization();test_conversion();test_read_error();test_simple_sensors();
  test_events();test_tilt_and_time();test_fault();test_captured_samples();
  puts("PASS: 8 groups: initialization, units, errors, PIR/ADC, events, timing, faults, captured samples");
  return 0;
}
