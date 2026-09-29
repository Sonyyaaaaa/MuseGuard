# Залежності

- ST STM32C0 HAL 1.0.1, vendored. ADC/ADCEx/LL_ADC додано з STMicroelectronics/stm32c0xx-hal-driver, tag v1.0.1, commit 24999b605d0b9da3e4675c4c884036ae724bb59f. Решта HAL з основи ЛР1 має HAL_GetHalVersion=0x01000100.
- CMSIS і STM32C0 device headers з незміненого дерева ЛР1, базовий commit 89d4f8e. Ліцензії збережено в Drivers.
- Власні драйвери не потребують сторонніх runtime бібліотек.
- Тулчейн Docker використовує Ubuntu 22.04 і ARM GCC 10.3.1. Хостове середовище демонстрації: Ubuntu 24.04. Версії образу і пакетів фіксуються у Dockerfile; склад можна перевірити make toolchain.
- Хостові тести: C11, gcc або clang, assert.h; Python 3 для перевірки залежностей. assert простий, але не дає ізоляції кожного тесту, структурованих mock diagnostics і продовження після першого failure.
