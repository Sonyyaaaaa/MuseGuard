#include "wokwi-api.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
static pin_t pir,sda; static uart_dev_t uart; static timer_t timer; static unsigned step; static uint8_t burst[32],z[]="z",dump[]="d",stats[]="s",readid[]="c";
static void pulse(void *u){(void)u; ++step; if(step>=60 && step<460) pin_write(pir,((step-60)%8)<2?HIGH:LOW); else pin_write(pir,LOW); if(step==40) uart_write(uart,z,1); if(step>=80 && step<=400 && step%20==0) uart_write(uart,burst,sizeof burst); if(step==420 || step==680) uart_write(uart,dump,1); if(step==460 || step==860) uart_write(uart,stats,1); if(step==500) pin_mode(sda,OUTPUT_LOW); if(step==600) pin_mode(sda,INPUT); if(step==660) uart_write(uart,readid,1);}
void chip_init(void){for(unsigned i=0;i<sizeof burst;++i)burst[i]='n'; pir=pin_init("PIR",OUTPUT_LOW); sda=pin_init("SDA",INPUT); const uart_config_t u={.tx=pin_init("TX",INPUT_PULLUP),.rx=pin_init("RX",INPUT),.baud_rate=115200}; uart=uart_init(&u); const timer_config_t t={.callback=pulse,.user_data=NULL}; timer=timer_init(&t); timer_start(timer,50000,true);}
