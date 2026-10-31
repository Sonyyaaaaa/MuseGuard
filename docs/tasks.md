# Задачі MuseGuard, ЛР3

Один foreground цикл, один MSP, без ОС і перемикання контекстів. Periodic release за SysTick, пропущені періоди рахуються, catch-up burst немає. Періоди нижчі за 2^31 мкс; різниця unsigned витримує один wrap.

| Задача | FR | Період або подія | Дедлайн / бюджет | Доказ |
|---|---|---|---|---|
| Sample ADC/PIR/IMU і detector | 02–08, 11, 13 | 20 000 мкс | завершення ≤20 000 мкс від release | PA6, STAT sample, CADENCE |
| Recovery | 14 | 1 с, якщо датчик недоступний | ціль початку ≤100 мс | STAT recovery; I²C timeout 15 мс на transfer |
| Команда | 10–14 | ISR RX, ≤1 byte/poll | ≤100 мс за 1 command/s | STAT command_latency, PA8 |
| Journal | 06, 12 | ≤1 record/poll, queue 16 | ціль ≤100 мс до постановки; запис не hard-real-time | PA7, EVENT flash_us, CRC dump |
| Stats або dump | 01, 12 | ≤1 row/poll | background, UART 115200 8N1 | STAT output |
| Heartbeat | 01, 09 | 1 с | release ≤100 мс | HEART t_us |

Порядок: ACCESS timeout, Sample, Recovery, одна команда, один запис, один рядок stats/dump, heartbeat. EXTI user button з ЛР1 лишається, застосунок його не використовує. SysTick IRQ підтримує HAL tick; для статистики береться тільки HAL tick з кроком 1 мс. USART2 IRQ ставить байт з часом у SPSC ring. ISR не викликає HAL із timeout або printf.

`late` рахує запуски пізніше release+deadline; `finish_miss` завершення sample пізніше release+deadline; `skipped` точну кількість пропущених номінальних release. Jitter = поточний start мінус попередній start мінус period, зі знаком.

PA4 ADC, PA6 sample, PA7 flash, PA8 command. У diagram.json D0=SCL, D1=SDA, D2=UART TX, D3=PIR, D4=ADC, D5=sample, D6=flash, D7=command. PA6/7/8 вільні GPIO, SPI і TIM3 channels не використовуються.

Команди: `a/g/k/c/n/f/v/s/d/z`. `f/v` є програмною ін'єкцією недоступності IMU й її зняттям, не фізичним від'єднанням SDA. `n` адресний NACK 0x69. `z` скидає часову статистику й release clocks; RX/drop, event/drop, flash/fail і memory high-water лишаються cumulative. Неповні UART рядки і невідомі символи не змінюють режим.
