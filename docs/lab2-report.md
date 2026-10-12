# ЛР2. Інтеграція сенсорної підсистеми MuseGuard

Розпутня Софія Михайлівна, ІО-31. Середовище: Ubuntu 24.04, Docker, STM32 HAL, Wokwi.

Прошивка об'єднує MPU6050 на I²C1, PIR на GPIO та LDR на ADC. На старті вона видає self-test, далі опитує сенсори кожні 20 мс і реєструє PRESENCE, LIGHT_CHANGE, SHOCK та TILT. App і Devices незалежні від HAL. Результат перевірено у Wokwi, host tests та ARM build.

Звіт містить результати ЛР2. Фінальний PDF збережено в папці звіту. Цей файл надає навігацію до первинних матеріалів у репозиторії.

| Частина звіту | Матеріал |
| --- | --- |
| Вибір сенсорів і межі моделі | [hardware-components.md](hardware-components.md) |
| Pinout, монтаж, clocks, бюджет живлення | [pinout.md](pinout.md) |
| Component diagram і bus callbacks | [architecture.md](architecture.md) |
| HAL/CMSIS/toolchain, модулі, CI | [dependencies.md](dependencies.md) |
| VCD, manual decode, timings, NACK | [captures/analysis.md](captures/analysis.md) |
| Host tests і coverage | [testing.md](testing.md) |
| Справжні build/test/toolchain logs | [validation](validation/) |
| Вимоги та перевірені сценарії | [PRD.md](PRD.md) |

## Результати

- WHO_AM_I=0x68; burst з 0x3B дає 0, 0, 1000 mg і 24001 m°C у спокої. Нахил дає 599, 0, 799 mg. Обидва raw frames входять до test fixtures.
- SCL=100 кГц; ID read 0.712775 мс, pointer+14 bytes 1.882775 мс. Адресний NACK на 0x69 викликає BUS_ERROR, потім 0x68 знову читається.
- PIR high приблизно 1 с, затримка до UART EVENT 6.176850 мс. ADC_MARK period 20.017325 мс, width 71.175 мкс.
- Освітлення дало ADC 1001→4063 і LIGHT_CHANGE. Прискорення 1.6 g дало SHOCK, окремий нахил TILT після 500 мс без SHOCK.
- 8 груп tests пройшли на macOS clang і Linux GCC. ASan/UBSan без діагностик. Portable GCC line coverage 100%, branch taken 96.88–100%.
- Чиста ARM build без warnings: text 20316, data 120, bss 1976 байтів. GitHub Actions [run 38263190985](https://github.com/Sonyyaaaaa/MuseGuard/actions/runs/38263190985) успішно виконав build, test і coverage для 7f0fec9.

## Журнал проблем

| Симптом | Причина | Виправлення |
| --- | --- | --- |
| Немає ADC headers/sources | Основа ЛР1 не потребувала ADC | HAL ADC/ADCEx/LL_ADC тієї самої версії, [cd44da8](https://github.com/Sonyyaaaaa/MuseGuard/commit/cd44da8) |
| Wokwi сенсори не живляться | Неточні назви power pins | 3V3.1/5V.1/GND.1, [5b87f89](https://github.com/Sonyyaaaaa/MuseGuard/commit/5b87f89) |
| ACK є, HAL timeout або corrupt writes | TXIS update у моделі запізнюється | Platform register adapter, CR2 assignments, TX pause, [5b87f89](https://github.com/Sonyyaaaaa/MuseGuard/commit/5b87f89) |
| Burst втрачає bytes | TX pause помилково застосована до RXDR | RX без паузи, wire/UART verification, [5b87f89](https://github.com/Sonyyaaaaa/MuseGuard/commit/5b87f89) |
| Linux виконує macOS binary | Спільна папка test build | Папки OS-arch, [bf04f1d](https://github.com/Sonyyaaaaa/MuseGuard/commit/bf04f1d) |
| GCC не знаходить host C headers | Docker мав лише ARM середовище | gcc/libc6-dev, [7f0fec9](https://github.com/Sonyyaaaaa/MuseGuard/commit/7f0fec9) |
| sigrok не експортує clipped VCD | Crop metadata у body не підтримуються importer | $comment перенесено в header, [d9e6a7d](https://github.com/Sonyyaaaaa/MuseGuard/commit/d9e6a7d) |

Host tests не виявили нового функціонального дефекту в portable logic. Проблеми знайдено під час bring-up і налаштування test environment.

## Скриншоти для вставлення

1. CubeMX Pinout і ADC/I²C settings. PA0, PA4, PA10, PB8/PB9, PA2/PA3, PA5; ADC 12-bit, 160.5 cycles, I²C timing.
2. Загальна схема Wokwi, усі сенсори й аналізатор одночасно, два pull-up, спільна земля.
3. UART fresh startup з BOOT, SELFTEST MPU/LDR/PIR, READY та DATA у спокої.
4. PulseView i2c-uart-bringup.vcd при 20 МГц; I²C decoder SCL/SDA, адреса 0x68, регістр 0x75, ID=0x68, ACK/NACK. Виділити біти 0xD0 і курсорами період SCL.
5. PulseView i2c-command-read.vcd і i2c-nack.vcd: burst від 0x3B, 14 bytes, duration; окремо адреса 0x69 та NACK.
6. PulseView pir-presence.vcd і adc-light-change.vcd: PIR pulse, UART decoder 115200 8N1, ADC_MARK period/width. Captures мають локальний час, original offsets у header.
7. UART SHOCK і окремий TILT із ax/az, t та alarm. Для TILT змінити X/Z при паузі, щоб уникнути проміжного SHOCK.
8. GitHub Actions успішний run та test/coverage terminal. Видимі команди, PASS і відсотки.

PulseView GUI тут ще не використано. Декодування sigrok-cli та ручний розбір виконано, анотації збережено. Скриншот CubeMX підтвердить відкриття оновленого .ioc; код і .ioc вже звірено автоматично.

## Відповідальність за напрями

| Напрям | Відповідальна |
| --- | --- |
| Звіт і документація | Розпутня Софія |
| Datasheet та pinout | Розпутня Софія |
| Стенд Wokwi і captures | Розпутня Софія |
| Декодування та аналіз | Розпутня Софія |
| Архітектура, build, CI | Розпутня Софія |
| Тестування і coverage | Розпутня Софія |

## Межі перевірки

Фізична плата, струми, шум ADC, реальні фронти, короткий удар і всі комбінації впливів не перевірені. Споживання і PIR OUT треба перевірити перед реальним монтажем. Класифікація SHOCK поки порогова. Погодження STM32C031C6 і незастосовності SPI-flash потребує підтвердження викладача.


## Pull request

ЛР1 уже в main, тому base main, compare feature/lab-2. Відкрити [форму PR](https://github.com/Sonyyaaaaa/MuseGuard/compare/main...feature/lab-2?expand=1), назва «ЛР2: інтеграція сенсорної підсистеми MuseGuard». В описі зазначити сенсори, шари, captures і перевірки, додати посилання на цей звіт. Перед merge перевірити зелений CI і вкладені скриншоти. Фінальний стан ЛР2 позначається tag lab2.
