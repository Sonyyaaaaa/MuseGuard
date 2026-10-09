# Перевірка драйверів та логіки

`make -C tests test` компілює App і Devices без HAL, CMSIS та MCU toolchain. Транспорт MPU6050 підмінено callbacks fake bus з контрольованими ID, кадрами та помилками. Вісім груп перевіряють порядок і значення п'яти записів конфігурації, BUS_ERROR/TIMEOUT на кожному кроці, неправильний ID, signed big-endian та одиниці, незмінність вихідного sample після помилки, PIR фронти, межі ADC, гістерезис подій, latch/reset, 500 мс нахилу, переривання підтвердження, переповнення tick і fault/recovery.

Окрема група читає `tests/fixtures/mpu6050_capture.h`. Байти спокою і нахилу взято з реальних VCD у `docs/captures`; вона звіряє 1000 mg у спокої та 599/799 mg при нахилі, температуру 24001 m°C і TILT на t=74671. Решта тестових значень синтетичні й перевіряють межі та помилки.

Перевірено 12 жовтня 2026:

| Середовище | Результат |
| --- | --- |
| macOS arm64, clang, C11, -Wall -Wextra -Werror | PASS, усі 8 груп |
| Linux aarch64, Docker Ubuntu 22.04, GCC | PASS, усі 8 груп |
| macOS, AddressSanitizer та UndefinedBehaviorSanitizer | PASS, без діагностик |
| tools/check_architecture.py | PASS, переносимі include і залежності |
| tools/check_wiring.py | PASS, відповідність diagram.json та .ioc |

## Покриття

Обрана ціль для переносимого коду: ≥90% рядків і ≥80% напрямків гілок. Core, Platform і сторонні бібліотеки виключено з host coverage: ці тести не виконують регістри МК та HAL. Інтеграцію перевіряють Wokwi й окрема ARM-збірка.

| Модуль | GCC рядки | GCC branch taken | clang рядки | clang branch taken |
| --- | --- | --- | --- | --- |
| Devices/mpu6050.c | 100% / 34 | 96.88% / 32 | 100% / 40 | 96.88% / 32 |
| Devices/simple_sensors.c | 100% / 8 | 100% / 8 | 100% / 9 | 100% / 6 |
| App/museguard.c | 100% / 48 | 98.44% / 64 | 100% / 59 | 98.21% / 56 |

Знаменники відрізняються через облік компіляторами. Це не покриття всієї прошивки і не доказ фізичної точності порогів. Відсоток гілок самого test runner нижчий через невиконані failure branches assert; він не входить до цілі.

Команди повторення:

```sh
make test
make coverage
# macOS для gcov-формату clang
make coverage GCOV='xcrun llvm-cov gcov'
```

Артефакти розділено за OS та архітектурою в `tests/build` і `tests/coverage`, щоб Linux не виконував Mach-O binary, створений macOS. Coverage щоразу видаляє старі .gcda, щоб зміна тестів не змішувала різні профілі.

## Обмеження перевірки

Wokwi підтвердив self-test, WHO_AM_I, burst read, NACK на відсутній адресі, PIR, світло, пороги SHOCK і окремий TILT. Короткий фізичний удар, реальне енергоспоживання, електричний шум, усі комбінації одночасних впливів та перенос на плату не перевірено. ADC_MARK показує тривалість виклику в симуляторі. PIR READABLE перевіряє доступність входу, а не справність фізичного датчика. Повні докази і часові виміри в [captures/analysis.md](captures/analysis.md).
