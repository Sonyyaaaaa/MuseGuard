ROOT := ../firmware
K := freertos
BUILD := ../firmware/build/pilot
CC := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
CFLAGS := -mcpu=cortex-m0plus -mthumb -Og -g -Wall -Wextra -ffunction-sections -fdata-sections -MMD -MP -DUSE_HAL_DRIVER -DSTM32C031xx -Isrc -I$(K)/include -I$(K)/portable/GCC/ARM_CM0 -I$(ROOT)/Core/Inc -I$(ROOT)/Drivers/STM32C0xx_HAL_Driver/Inc -I$(ROOT)/Drivers/CMSIS/Device/ST/STM32C0xx/Include -I$(ROOT)/Drivers/CMSIS/Include
SOURCES := src/pilot_main.c $(K)/tasks.c $(K)/list.c $(K)/queue.c $(K)/portable/GCC/ARM_CM0/port.c $(K)/portable/MemMang/heap_4.c $(ROOT)/Core/Src/system_stm32c0xx.c $(ROOT)/Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal.c $(ROOT)/Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_cortex.c $(ROOT)/Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_gpio.c $(ROOT)/Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_rcc.c $(ROOT)/Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_rcc_ex.c $(ROOT)/Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_uart.c $(ROOT)/Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_hal_uart_ex.c
OBJECTS := $(addprefix $(BUILD)/,$(notdir $(SOURCES:.c=.o))) $(BUILD)/startup.o
vpath %.c $(sort $(dir $(SOURCES)))
.PHONY: all
all: $(BUILD)/pilot.elf $(BUILD)/pilot.hex
$(BUILD):
	mkdir -p $@
$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/startup.o: $(ROOT)/startup_stm32c031xx.s | $(BUILD)
	$(CC) $(CFLAGS) -x assembler-with-cpp -c $< -o $@
$(BUILD)/pilot.elf: $(OBJECTS)
	$(CC) $(CFLAGS) $^ -specs=nano.specs -T$(ROOT)/STM32C031C6Tx_FLASH.ld -Wl,-Map=$(BUILD)/pilot.map,--gc-sections,--print-memory-usage -lc -lm -lnosys -o $@
	arm-none-eabi-size $@
$(BUILD)/pilot.hex: $(BUILD)/pilot.elf
	$(OBJCOPY) -O ihex $< $@

-include $(OBJECTS:.o=.d)
