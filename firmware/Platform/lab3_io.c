#include "lab3_io.h"
#include "main.h"
#include <string.h>
#define STORE_ADDRESS 0x08007000U
#define STACK_PATTERN 0xA52CC25AU
static uint32_t previous_us, ambiguous;
static volatile uint32_t received_at[128];
static volatile uint8_t received_byte[128],write_index,read_index;
static volatile uint32_t dropped_bytes,uart_overruns;
extern uint32_t _estack, _Min_Stack_Size;
static uint32_t *stack_floor(void) {return (uint32_t *)((uintptr_t)&_estack-(uintptr_t)&_Min_Stack_Size);}
void lab3_paint_stack(void)
{
  uint32_t *end=(uint32_t *)__get_MSP()-16;
  for(uint32_t *p=stack_floor();p<end;++p)*p=STACK_PATTERN;
}
uint32_t lab3_stack_used(void)
{
  uint32_t *p=stack_floor();while(p<&_estack && *p==STACK_PATTERN)++p;
  return (uint32_t)((uintptr_t)&_estack-(uintptr_t)p);
}
void lab3_io_init(void)
{
  GPIO_InitTypeDef pin={0};pin.Pin=GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8;
  pin.Mode=GPIO_MODE_OUTPUT_PP;pin.Speed=GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA,&pin); GPIOA->BRR=pin.Pin;
  USART2->ICR=USART_ICR_ORECF;
  USART2->CR1|=USART_CR1_RXNEIE_RXFNEIE;
  HAL_NVIC_SetPriority(USART2_IRQn,1,0);HAL_NVIC_EnableIRQ(USART2_IRQn);
}
uint32_t lab3_us(void)
{
  uint32_t mask=__get_PRIMASK();__disable_irq();
  /* Wokwi C031 sub-tick/register snapshots disagreed with independent VCD.
     Use the monotonic HAL millisecond tick and report its real resolution. */
  uint32_t result=HAL_GetTick()*1000U;
  if((int32_t)(result-previous_us)<0)++ambiguous;
  previous_us=result;
  __set_PRIMASK(mask);return result;
}
uint32_t lab3_clock_ambiguous(void){return ambiguous;}
void USART2_IRQHandler(void)
{
  uint32_t flags=USART2->ISR;
  if(flags&USART_ISR_ORE) {++uart_overruns; USART2->ICR=USART_ICR_ORECF;}
  if(flags&USART_ISR_RXNE_RXFNE) {
    uint8_t value=(uint8_t)USART2->RDR, next=(write_index+1U)&127U;
    if(next==read_index)++dropped_bytes;
    else {received_byte[write_index]=value;received_at[write_index]=lab3_us();write_index=next;}
  }
}
bool lab3_rx(char *c,uint32_t *time)
{
  if(read_index==write_index)return false;
  *c=(char)received_byte[read_index];*time=received_at[read_index];
  read_index=(read_index+1U)&127U;return true;
}
void lab3_rx_stats(uint32_t *dropped,uint32_t *overruns) {*dropped=dropped_bytes;*overruns=uart_overruns;}
void lab3_probe(unsigned pin,bool high) {GPIOA->BSRR=1U<<(pin+(high?0U:16U));}
static bool storage_read(void *ctx,uint32_t offset,void *out,uint32_t length)
{
  (void)ctx;if(offset>MG_STORE_BYTES || length>MG_STORE_BYTES-offset)return false;
  memcpy(out,(const void *)(STORE_ADDRESS+offset),length);return true;
}
static bool storage_program(void *ctx,uint32_t offset,const void *data)
{
  (void)ctx;if(offset>MG_STORE_BYTES-8 || (offset&7U))return false;
  uint64_t word;memcpy(&word,data,8);
  if(HAL_FLASH_Unlock()!=HAL_OK)return false;
  HAL_StatusTypeDef result=HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,STORE_ADDRESS+offset,word);
  HAL_FLASH_Lock();return result==HAL_OK;
}
static bool storage_erase(void *ctx,uint32_t offset)
{
  (void)ctx;if(offset!=0 && offset!=MG_STORE_PAGE)return false;
  FLASH_EraseInitTypeDef erase={0};uint32_t error=0;
  erase.TypeErase=FLASH_TYPEERASE_PAGES;erase.Page=14U+offset/MG_STORE_PAGE;erase.NbPages=1;
  if(HAL_FLASH_Unlock()!=HAL_OK)return false;
  HAL_StatusTypeDef result=HAL_FLASHEx_Erase(&erase,&error);
  HAL_FLASH_Lock();return result==HAL_OK;
}
mg_storage_t lab3_storage(void) {return (mg_storage_t){0,storage_read,storage_program,storage_erase};}
