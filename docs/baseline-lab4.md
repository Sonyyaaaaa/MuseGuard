# Базова лінія MuseGuard для ЛР4

ЛР4 не реалізована. Базова прошивка ЛР3 є bare metal, C031C6 48 MHz, ADC/PIR/MPU6050, UART 115200 8N1. Версія у BOOT: source `f8a557c75edecf2461b712bfe0667b9c0069caccdc3f814856416e294387d3e4`, git `15280fb5e0e1-dirty`. Source manifest зберігається у metrics, локальний run-manifest і повні VCD у Lab03/Work.

## Незмінні умови порівняння

Повторити ті самі wiring, частоту, пороги, розмір 32-byte journal record, дві сторінки flash, формат команд і трафік. Залишити PA4/6/7/8, той самий VCD decoder і крок 1 мс firmware counters. Новий microsecond counter не порівнювати безпосередньо з старим; VCD лишається незалежним способом. ISR RX 127-byte ring і лічильники drop/ORE мають однакову семантику. Вимірювання включає інструментування.

`STAT name=... n=... min_us=... avg_us=... max_us=...` має крок 1000 мкс; середнє також може мати фазове зміщення через квантування. `CADENCE` дає skipped releases, completion misses і signed jitter. `busy_permille` є часткою часу задач, не сумарним CPU utilization сплячого/активного ядра. Loop обертається без WFI. Heap є sbrk high-water, не mallinfo used/free.

## Сценарії

| ID | Дії | Тривалість / повтори | Артефакт |
|---|---|---|---|
| S1 | fresh ELF, спокій, після прогріву `z`, наприкінці `s` | ≥60 с, тисячі sample | S1 UART і повний VCD |
| S2 | той самий ELF, `z`; рідкісні `a`, PIR, зміни світла; `s` на початку/в кінці, `d` для CRC | ≥3600 с без reset і firmware reload | S2 UART, wall/MCU interval, CRC dump, VCD початку |
| S3 | той самий ELF, `z`, `d`, потік `n`, PIR під час обробки backlog, зміни режиму й записи | ≥30 фронтів PIR; тисячі sample | S3 UART/VCD, drop/ORE, latencies |
| F1 | `f`, пауза, `v`, перевірити FAULT і RECOVERED | окремий лог | програмна ін'єкція, не SDA short |
| F2 | ступінчасті UART bursts 1/8/32/128/512 символів `n` | stats після кожного рівня | межа rx ring і response |
| F3 | diagram.lab3-fault.json, SDA-GND на старті, розімкнути switch | Wokwi timeout повторюється раз/с, RECOVERED t33029ms, ID104 | SDA-fault-recovery UART/VCD; фізична плата не перевірена |
| F4 | reset MCU після `d`; потім відключення живлення під час запису | ≥30 actual power cuts з CRC | host byte-cut тест не замінює це |

Для веб-Wokwi F1, Upload Firmware and Start Simulation. Після Stop зберегти оригінальний VCD до повторного запуску. Analyzer має 3 000 000 transitions capacity, тому годинний S2 не має суцільного VCD усієї години; тривалість підтверджує UART. Скрипт `tools/analyze_lab3.py VCD --output PREFIX` обчислює ширини probes і PIR→початок UART EVENT та консервативну межу PIR→кінець наступного SAMPLE, який почався після фронту. `tools/clip_vcd.py` лише обрізає справжній запис і додає source hash/offset у header.

## Відомий S1

3829 releases за вікно від `z` до `s`, skipped=0, finish_miss=0, max release lateness 6 мс, max command latency 10 мс, task busy estimate 106/1000, stack watermark 584 B і heap arena high-water 1032 B. VCD steady window 12.05–88.5 с: 3823 повні імпульси SAMPLE, min/avg/max 2345.75/2347.64/2352.85 мкс, період 13386.15..26639 мкс. Це окремі оцінки з різною роздільністю й межами вікна. S2 тривав щонайменше 3635 с після z, без reset/reload. 181776 sample, skipped=1, finish_miss=1, release max23 мс, jitter ±17 мс, loop max29 мс, command latency max38 мс, busy107/1000. S3 має 30 PIR фронтів під час drain 64-command packets: 12118 sample, skipped=2, finish_miss=3, release max21 мс, jitter −11..+16 мс, loop max25 мс, command latency max978 мс, busy212/1000. В усіх S1–S3 rx_drop/ORE/event_drop/flash_fail/clock_ambiguous=0, stack584 і heap arena1032 B. Повні min/avg/max усіх задач у S*-snapshot.json.

VCD S3: PIR→початок EVENT n30, min/avg/max15.144/26.167/40.765 мс. Консервативна верхня межа PIR→кінець наступного після фронту SAMPLE max26.761 мс, це не прямий probe LED. Жодний фронт не припав на FLASH high; 22 припали на COMMAND high. Не називати це 30 подіями всередині erase. S3 VCD повний до247.215 с, stats після цього підтверджено UART.

За burst n*129+LF втрачено один n і LF, rx_drop збільшився на2, max command latency1759 мс. При n*128+LF втрачено лише LF. Це конкретна межа даного темпу надходження, не універсальна паспортна пропускна здатність.

## Гіпотези

Вищий пріоритет sensor task і окремий UART worker можуть зменшити max release lateness при burst traffic до ≤2 мс, якщо TX блокує лише свою задачу й ISR не маскуються. Черга команд сама не збільшує пропускну здатність UART. Той самий 127-byte backlog має бути перевірений при тому самому потоці.

ОС не збільшить flash і не прибере зупинку виконання з єдиного flash bank під час erase. Потрібно повторити зовнішні probes, а не обіцяти фізичний дедлайн за симулятором. Для RAM планується порівняння залишку після окремих task stacks з поточними static RAM 2404 B і єдиним MSP reserve 2048 B; це план перевірки, не вимір FreeRTOS.

## Критерії наступного порівняння

Застосувати однакові S1–S3 і burst boundary. Гіпотези H1–H6 та витрати наведені в limitations.md. Для sensor task перевірити release≤2 мс і finish_miss=0, для 30 PIR UART≤100 мс, для command nominal≤100 мс, виміряти максимуми по кожному scenario. Зберегти raw, source/ELF hash і точні умови. Порівнювати idle sleep окремо від task busy та одночасно рахувати RAM всіх stacks, TCB, queues і kernel. Механізми ОС ще не реалізовано і результати для них відсутні.

## Обов'язкове повторення додаткових вимірів

P20 fixture tests/fixtures/phase20: незмінний baseline ELF, C input driver на PA10 замість PIR OUT. У web Wokwi створити Custom Chip phase20, вставити .chip.c/.json, схему diagram.phase20.json, завантажити ELF через Upload Firmware. High20000us/low180200us,100 pulse, групи кожні25s із1s. Перша/друга/четверта групи calm, третя під12 пакетами64 n, фактичні інтервали збережено у P20-stress-trials.json локально. Повторити до400 inputs,≥30 fronts при COMMAND high, усі20 bins1ms. Аналіз tools/analyze_phase20.py рахує300/300 calm,92/100 stress;20ms widths у VCD±25ns. Отримані latency3.168/13.215/23.115ms calm та3.260/16.990/32.194ms stress стосуються лише отриманих подій. Вісім відсутніх не зменшують max і mean штучним нулем.

Runtime SDA на baseline ELF: почати зі справної шини, замкнути switch після HEART fault0, отримати FAULT, розімкнути, чекати фактичного RECOVERED і id104. Порівняти sample timeout22ms, recovery max22ms, release-to-RECOVERED1658.604ms, stack664B. Ціль H4 стосується ADC/PIR deadline, не обіцянки миттєвої готовності IMU.

Startup із підключеною тестовою I²C-моделлю: NACK до 5 с, потім ACK; RECOVERED t_ms=5029, перший повний кадр 5.029333–5.030698 с, завершення SAMPLE 5.031062 с. Повторити [fixture](../tests/fixtures/startup-not-ready/README.md) та зберегти UART/VCD. Boot-with-SDA-low є іншим збоєм; жоден із двох тестів не встановлює фізичний час готовності MPU6050.

Діагностику mallinfo і NVIC reset повторювати окремим link із tools/diagnostics/heap-reset-probe.c, власним ELF hash та DIAG marker. Поточні results: allocated1032/free0B; із ACCESS/alarm1 reset повертає GUARD/alarm0, CRC-valid seq1–5 лишаються. Не підставляти її timings або stack648B у baseline. Toolbar Restart у web-проєкті запускає default sketch Hello STM32, тому повторний baseline старт робити саме Upload Firmware. Фізичні power cut, струм, wear і електричні рівні є іншою перевіркою після переходу з формиB до плати.
