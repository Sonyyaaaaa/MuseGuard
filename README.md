# MuseGuard

Розпутня Софія Михайлівна, ІО-31. Середовище: Ubuntu 24.04, Docker, STM32 HAL, Wokwi. Проєкт спостерігає за музейною вітриною через PIR, фоторезистор та MPU6050. Присутність і світло є інформаційними подіями; механічна подія фіксує alarm. Намір людини чи факт крадіжки система не визначає.

ЛР1 збережена в історії main. ЛР2 продовжує той самий репозиторій на STM32C031C6. [Вимоги та стан перевірки](docs/PRD.md), [терміни](CONTEXT.md), [звіт ЛР2](docs/lab2-report.md).

## Збірка й запуск

```sh
make build
make test
make coverage
```

Збірка потребує Docker, тести C compiler та Python 3. ELF/HEX/BIN зберігаються у firmware/build. `make clean` видаляє лише цю папку. На macOS для coverage використовуйте `make coverage GCOV='xcrun llvm-cov gcov'`. [Залежності](docs/dependencies.md), [тести](docs/testing.md), [справжні журнали](docs/validation/).

Відкрийте цей каталог у VS Code з Wokwi extension, виконайте `Wokwi: Start Simulator`. Для запуску потрібен інтернет. Після зміни ELF закрийте старий simulator tab і запустіть знову. UART має показати BOOT, три SELFTEST, READY та DATA. Команди в terminal: `c` читає ID і 14 raw bytes, `n` створює адресний NACK, `r` скидає alarm і baseline. PIR запускається через Simulate motion, світло та IMU змінюються контролами сенсорів.

Після Stop аналізатор зберігає wokwi.vcd; збережіть його до наступного запуску. [Записи і декодування](docs/captures/analysis.md) містять параметри імпорту та виміри.

## Структура

- firmware/Core і Drivers містять startup, HAL і CMSIS.
- firmware/Platform містить pin/handle knowledge, адаптери та runner.
- firmware/Devices містить переносимі драйвери, firmware/App правила подій.
- tests містить fake bus, error scenarios та кадри зі справжніх VCD.

[Component diagram](docs/architecture.md), [підключення](docs/pinout.md), [сенсори](docs/hardware-components.md). Перед CubeMX regeneration звірити .ioc і Makefile, зберегти власні каталоги. Перед перенесенням до VM прочитайте Study/VM-transfer.md і порівняйте файли.

Перевірено simulator bring-up, обмін та host logic. Фізичні струми, шум, короткий удар і всі комбінації впливів ще не перевірені. Погодження моделі МК та незастосовності SPI-flash потребує підтвердження.

Основа походить із [Wokwi Nucleo-C031C6 example](https://github.com/wokwi/stm32-hello-wokwi). Ліцензії збережено у LICENSE і firmware/Drivers.
