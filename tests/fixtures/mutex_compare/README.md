# MuseGuard: однакове навантаження для двох меж mutex

Тестовий компонент `mgload` задає сигнали з часу Wokwi, не реалізує логіку продукту. Вхід UART: `z` на 2 с, 17 пакетів по 32 `n` з 4 до 20 с, `d` 21 с, `s` 23 с, `c` 33 с, `d` 34 с, `s` 43 с; 50 PIR high по 100 мс через 400 мс із 3 до 22.7 с; SDA примусово low 25–30 с. RX, PIR і SDA потрібно перевірити за сирими VCD, а не вважати програму генератора доказом. На PA3 джерело тільки TX генератора. Це не фізичний сенсор, і тривала низька SDA — тест моделі.

```sh
python3 lab4/build_identity.py
mkdir -p firmware/build/compare-wide firmware/build/compare-short
cp firmware/build/lab4/lab4_build_id.h firmware/build/compare-wide/lab4_build_id.h
cp firmware/build/lab4/lab4_build_id.h firmware/build/compare-short/lab4_build_id.h
docker run --rm -v "$PWD:/workspace" -w /workspace --user "$(id -u):$(id -g)" museguard-build:lab3 make -C firmware MG_FREERTOS=1 BUILD_DIR=build/compare-wide TARGET=wide MG_EXTRA_DEFS='-DMG_MUTEX_TRACE -DMG_MUTEX_WIDE' all
docker run --rm -v "$PWD:/workspace" -w /workspace --user "$(id -u):$(id -g)" museguard-build:lab3 make -C firmware MG_FREERTOS=1 BUILD_DIR=build/compare-short TARGET=short MG_EXTRA_DEFS='-DMG_MUTEX_TRACE' all
```

`MG_MUTEX_WIDE` повертає mutex навколо service UART/flash; обидва ELF мають той самий код, priorities, heap і tracing; без цих визначень штатна збірка лишається short без GPIO probe. Створити у власному проєкті C Custom Chip з назвою `mgload`, вставити вміст `.chip.c` і `.chip.json`, потім `diagram.json`. Дочекатися компіляції чипа й перевірити сигнали до збору. Завантажити wide ELF через Upload Firmware and Start Simulation, після ≥44 с MCU зупинити й зберегти обидва VCD та хеш; так само окремо завантажити short. Перевірити BOOT і початковий стан flash для кожного запуску. `analyze.py` приймає абсолютний шлях до папки даних із `compare-{wide,short}.vcd` та `compare-{wide,short}-input.vcd`; перевіряє вхідні дані, GPIO, CRC, виводить JSON та UART, декодований з VCD.

D4 основного аналізатора — PA9, high після успішного взяття state mutex та low безпосередньо перед release. Вимір включає переривання, але не включає невеликі інтервали між викликом mutex і перемиканням GPIO; витіснення в цих вікнах може спотворити ширину. Probe додає власні інструкції і не є незміненою production-збіркою. Жоден VCD із симулятора не встановлює фізичні затримки MCU чи поведінку flash під вимкненням живлення.

Для порівняння I²C-діагностики обидві збірки мають `-DMG_MUTEX_TRACE`, контроль додатково `-DMG_I2C_SYNC_LOG`, нова — ні. Прапорець контролю повертає безпосередній вивід повідомлення timeout; не плутати його з `MG_MUTEX_WIDE`. Для чинного `analyze.py` покласти контрольні VCD під іменами `compare-wide.vcd` і `compare-wide-input.vcd`, нові — під іменами `compare-short.vcd` і `compare-short-input.vcd`. Назви wide/short тут лише позначають сторони тесту. Ідентичність UART/PIR/SDA, CRC і повноту інтервалів перевіряє аналізатор.
