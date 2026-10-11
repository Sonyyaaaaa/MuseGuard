# Залежності

- ST STM32C0 HAL 1.0.1, vendored. ADC/ADCEx/LL_ADC додано з STMicroelectronics/stm32c0xx-hal-driver, tag v1.0.1, commit 24999b605d0b9da3e4675c4c884036ae724bb59f. Решта HAL з основи ЛР1 має HAL_GetHalVersion=0x01000100.
- CMSIS і STM32C0 device headers з незміненого дерева ЛР1, базовий commit 89d4f8e. Ліцензії збережено в Drivers.
- Власні драйвери не потребують сторонніх runtime бібліотек.
- Тулчейн Docker використовує Ubuntu 22.04 і ARM GCC 10.3.1. Базовий multi-arch image зафіксовано digest, ARM GCC, binutils і newlib зафіксовано версіями пакетів у Dockerfile. Допоміжні gcc, make, Python і системні залежності беруться з репозиторію Ubuntu; повний apt snapshot не фіксується. Фактичний склад можна перевірити `make toolchain`. CI працює на Ubuntu 24.04; локально перевірено macOS arm64 і Linux aarch64 у контейнері.
- Хостові тести: C11, gcc або clang, assert.h; Python 3 для перевірки залежностей. assert простий, але не дає ізоляції кожного тесту, структурованих mock diagnostics і продовження після першого failure.

## Чому ввімкнено модулі HAL

| Модулі | Призначення |
| --- | --- |
| ADC, ADCEx | Калібрування і читання PA0 |
| I2C, I2CEx | Початкове налаштування I2C1; transfer adapter у Platform працює з регістрами |
| UART, UARTEx | USART2 журнал і команди |
| GPIO | PIR, LED, ADC_MARK, alternate functions |
| RCC, RCCEx, FLASH, FLASHEx, PWR, PWREx, CORTEX | Початкові тактування, живлення, flash latency і SysTick |
| EXTI | Успадкований user button ISR основи ЛР1 |
| DMA, DMAEx | Типи й залежності ST HAL I2C/UART; DMA transfers у застосунку не використовуються |

SPI, TIM, RTC та інші непотрібні модулі вимкнено. HAL/CMSIS зберігаються в репозиторії з ліцензіями, мережеве завантаження під час ARM compilation не потрібне. `-Wall -Wextra -Werror` застосовано до власного C-коду, `-Wall` до vendor sources. Посилання на версію HAL: [ST tag v1.0.1](https://github.com/STMicroelectronics/stm32c0xx-hal-driver/tree/24999b605d0b9da3e4675c4c884036ae724bb59f).

CI виконує `make build`, `make test`, `make coverage` на push та pull request до main. Checkout action зафіксовано commit SHA. Архітектурна перевірка забороняє hardware headers і символи у App/Devices; компіляція host tests перевіряє їхню незалежність на практиці. Перевірка wiring звіряє ключові з'єднання і призначення .ioc, але не замінює перевірку electrical constraints для фізичної плати.
