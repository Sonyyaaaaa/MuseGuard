#include "platform.h"
#include "main.h"
extern I2C_HandleTypeDef hi2c1;
extern ADC_HandleTypeDef hadc1;
extern UART_HandleTypeDef huart2;

static mg_status_t status(HAL_StatusTypeDef result)
{
  if (result == HAL_OK) { return MG_OK; }
  return result == HAL_TIMEOUT ? MG_TIMEOUT : MG_BUS_ERROR;
}

static mg_status_t read_register(void *context, uint8_t address, uint8_t reg,
                                uint8_t *data, size_t length)
{
  if (data == NULL || length == 0 || length > UINT16_MAX || address > 127) {
    return MG_INVALID;
  }
  return status(HAL_I2C_Mem_Read(context, (uint16_t)address << 1, reg,
                I2C_MEMADD_SIZE_8BIT, data, (uint16_t)length, 5));
}

static mg_status_t write_register(void *context, uint8_t address, uint8_t reg,
                                 const uint8_t *data, size_t length)
{
  if (data == NULL || length == 0 || length > UINT16_MAX || address > 127) {
    return MG_INVALID;
  }
  return status(HAL_I2C_Mem_Write(context, (uint16_t)address << 1, reg,
                I2C_MEMADD_SIZE_8BIT, (uint8_t *)data, (uint16_t)length, 5));
}

mg_bus_t platform_i2c(void)
{
  mg_bus_t bus = {&hi2c1, read_register, write_register};
  return bus;
}

void platform_start(void)
{
  /* Calibration occurs before regular measurements. */
  if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK) { Error_Handler(); }
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
