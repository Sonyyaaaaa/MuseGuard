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

ЛР3 продовжує ту саму прошивку в гілці feature/lab-3. [Звіт і стан вимог](docs/lab3-report.md), [задачі](docs/tasks.md), [виміри](docs/metrics/scenarios.csv), [обмеження](docs/limitations.md), [база ЛР4](docs/baseline-lab4.md). ЛР2 лишається історичною редакцією в Git.

Для web Wokwi C031 вставте diagram.json, через F1 оберіть Upload Firmware and Start Simulation та firmware/build/firmware.elf. BOOT містить lab=3, source SHA-256, git revision і clock=SysTick_1ms. Команди: a відкриває ACCESS на 60 с без продовження повторною a; g повертає GUARD; k підтверджує alarm; c читає ID, n перевіряє NACK, f/v програмний fault/recovery; s видає метрики, d читає CRC-valid записи, z починає нове вікно метрик. Після символів у web terminal натисніть Enter.

Після Stop аналізатор зберігає wokwi.vcd; збережіть його до наступного запуску. [Записи і декодування](docs/captures/analysis.md) містять параметри імпорту та виміри.

## Структура

- firmware/Core і Drivers містять startup, HAL і CMSIS.
- firmware/Platform містить pin/handle knowledge, адаптери та runner.
- firmware/Devices містить переносимі драйвери, firmware/App правила подій.
- tests містить fake bus, error scenarios та кадри зі справжніх VCD.

[Component diagram](docs/architecture.md), [підключення](docs/pinout.md), [сенсори](docs/hardware-components.md). Перед CubeMX regeneration звірити .ioc і Makefile, зберегти власні каталоги. Перед перенесенням до VM прочитайте Study/VM-transfer.md і порівняйте файли.

Перевірено simulator bring-up, обмін та host logic. Фізичні струми, шум, короткий удар і всі комбінації впливів ще не перевірені. Погодження моделі МК та незастосовності SPI-flash потребує підтвердження.

Основа походить із [Wokwi Nucleo-C031C6 example](https://github.com/wokwi/stm32-hello-wokwi). Ліцензії збережено у LICENSE і firmware/Drivers.
