#include "platform.h"
#include "main.h"
#include <stdio.h>
extern I2C_HandleTypeDef hi2c1;
extern ADC_HandleTypeDef hadc1;
extern UART_HandleTypeDef huart2;

static mg_status_t status(HAL_StatusTypeDef result)
{
  if (result == HAL_OK) { return MG_OK; }
  return result == HAL_TIMEOUT ? MG_TIMEOUT : MG_BUS_ERROR;
}

/* Poll the peripheral with complete CR2 assignments. This also avoids a
   retained START bit producing an extra address phase in the simulator. */
static mg_status_t wait_i2c(I2C_TypeDef *i2c, uint32_t flag, uint32_t started)
{
  while ((i2c->ISR & flag) == 0) {
    if ((i2c->ISR & (I2C_ISR_NACKF | I2C_ISR_BERR | I2C_ISR_ARLO)) != 0) {
      return MG_BUS_ERROR;
    }
    if ((uint32_t)(HAL_GetTick() - started) >= 15) {
      printf("I2C timeout flag=0x%lX isr=0x%lX cr2=0x%lX\r\n",
             (unsigned long)flag, (unsigned long)i2c->ISR, (unsigned long)i2c->CR2);
      return MG_TIMEOUT;
    }
  }
  return MG_OK;
}

static void settle_byte(void)
{
  /* Wokwi updates TXIS asynchronously. Allow the address/data phase to
     finish before writing TXDR again. RXDR must still be read promptly. */
  uint32_t start = SysTick->VAL;
  uint32_t period = SysTick->LOAD + 1;
  uint32_t cycles = SystemCoreClock / 1000000U * 250U;
  for (;;) {
    uint32_t current = SysTick->VAL;
    uint32_t elapsed = start >= current ? start - current : start + period - current;
    if (elapsed >= cycles) { break; }
  }
}

static void finish_i2c(I2C_TypeDef *i2c, mg_status_t result)
{
  if (result != MG_OK) {
    i2c->CR2 = I2C_CR2_STOP;
    uint32_t started = HAL_GetTick();
    while ((i2c->ISR & I2C_ISR_STOPF) == 0 &&
           (uint32_t)(HAL_GetTick() - started) < 2) {}
  }
  i2c->ICR = I2C_ICR_STOPCF | I2C_ICR_NACKCF | I2C_ICR_BERRCF | I2C_ICR_ARLOCF;
  i2c->CR2 = 0;
}

static mg_status_t read_register(void *context, uint8_t address, uint8_t reg,
                                uint8_t *data, size_t length)
{
  if (data == NULL || length == 0 || length > 32 || address > 127) {
    return MG_INVALID;
  }
  I2C_TypeDef *i2c = ((I2C_HandleTypeDef *)context)->Instance;
  uint32_t started = HAL_GetTick();
  i2c->ICR = I2C_ICR_STOPCF | I2C_ICR_NACKCF;
  i2c->CR2 = ((uint32_t)address << 1) | (1U << I2C_CR2_NBYTES_Pos) |
             I2C_CR2_START | I2C_CR2_AUTOEND;
  settle_byte();
  mg_status_t result = wait_i2c(i2c, I2C_ISR_TXIS, started);
  if (result == MG_OK) {
    i2c->TXDR = reg;
    settle_byte();
    result = wait_i2c(i2c, I2C_ISR_STOPF, started);
  }
  if (result == MG_OK) {
    i2c->ICR = I2C_ICR_STOPCF;
    i2c->CR2 = 0;
    i2c->CR2 = ((uint32_t)address << 1) |
               ((uint32_t)length << I2C_CR2_NBYTES_Pos) |
               I2C_CR2_START | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND;
    settle_byte();
    for (size_t i = 0; i < length; ++i) {
      result = wait_i2c(i2c, I2C_ISR_RXNE, started);
      if (result != MG_OK) { break; }
      data[i] = (uint8_t)i2c->RXDR;
    }
    if (result == MG_OK) { result = wait_i2c(i2c, I2C_ISR_STOPF, started); }
  }
  finish_i2c(i2c, result);
  return result;
}

static mg_status_t write_register(void *context, uint8_t address, uint8_t reg,
                                 const uint8_t *data, size_t length)
{
  if (data == NULL || length == 0 || length > 32 || address > 127) {
    return MG_INVALID;
  }
  I2C_TypeDef *i2c = ((I2C_HandleTypeDef *)context)->Instance;
  uint32_t started = HAL_GetTick();
  i2c->ICR = I2C_ICR_STOPCF | I2C_ICR_NACKCF;
  i2c->CR2 = ((uint32_t)address << 1) |
             ((uint32_t)(length + 1) << I2C_CR2_NBYTES_Pos) |
             I2C_CR2_START | I2C_CR2_AUTOEND;
  settle_byte();
  mg_status_t result = MG_OK;
  for (size_t i = 0; i <= length; ++i) {
    result = wait_i2c(i2c, I2C_ISR_TXIS, started);
    if (result != MG_OK) { break; }
    i2c->TXDR = i == 0 ? reg : data[i - 1];
    settle_byte();
  }
  if (result == MG_OK) { result = wait_i2c(i2c, I2C_ISR_STOPF, started); }
  finish_i2c(i2c, result);
  return result;
}

mg_bus_t platform_i2c(void)
{
  mg_bus_t bus = {&hi2c1, read_register, write_register};
  return bus;
}

mg_status_t platform_start(void)
{
  /* Calibration occurs before regular measurements. */
  return status(HAL_ADCEx_Calibration_Start(&hadc1));
}

mg_status_t platform_adc(uint16_t *raw)
{
  if (raw == NULL) { return MG_INVALID; }
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
  HAL_StatusTypeDef result = HAL_ADC_Start(&hadc1);
  if (result == HAL_OK) { result = HAL_ADC_PollForConversion(&hadc1, 2); }
  if (result == HAL_OK) { *raw = (uint16_t)HAL_ADC_GetValue(&hadc1); }
  HAL_ADC_Stop(&hadc1);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
  return status(result);
}

bool platform_pir(void)
{
  return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_10) == GPIO_PIN_SET;
}

void platform_led(bool on)
{
  HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

uint32_t platform_now(void) { return HAL_GetTick(); }

bool platform_command(char *command)
{
  uint8_t value;
  if (HAL_UART_Receive(&huart2, &value, 1, 0) != HAL_OK) { return false; }
  *command = (char)value;
  return true;
}
