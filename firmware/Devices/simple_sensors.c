#include "simple_sensors.h"
bool pir_rising(pir_t *device, bool high)
{
  bool rising = high && !device->high;
  device->high = high;
  return rising;
}

mg_status_t light_millivolts(uint16_t raw, uint16_t *millivolts)
{
  if (millivolts == NULL || raw > 4095) { return MG_INVALID; }
  *millivolts = (uint16_t)((uint32_t)raw * 3300 / 4095);
  return MG_OK;
}
