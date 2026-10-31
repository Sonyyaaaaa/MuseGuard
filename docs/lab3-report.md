# ЛР 3 MuseGuard: результат прототипу без ОС

STM32C031C6, 48 MHz, UART115200 8N1, HAL, Ubuntu24.04 Docker, ARM GCC13.2.1, -Og. S1–S3/P20/SDA використовують один base ELF, source f8a557c75edecf2461b712bfe0667b9c0069caccdc3f814856416e294387d3e4, git15280fb5e0e1-dirty. Heap/reset перевірено окремим diagnostic link. Код ЛР 4 не реалізовано.

[Задачі та IRQ](tasks.md), [архітектура](architecture.md), [pinout](pinout.md), [CSV результатів](metrics/scenarios.csv), [пам'ять](metrics/memory.md), [повний реєстр обмежень](limitations.md), [база ЛР 4](baseline-lab4.md).

S2 щонайменше 3635 с без reset/reload:181776 sample, skipped1/finish_miss1, release max23 мс, jitter±17 мс, command max38 мс, stack584/heap arena1032 B. S3:30 PIR під час drain30 пакетів по 64 n, всі 30 EVENT; UART latency min/avg/max15.144/26.167/40.765 мс, conservative next-sample-end bound max26.761 мс. S3 skipped2/finish_miss3, command max978 мс. Це порушує 20-мс дедлайн, навіть коли середній sample близько 2 мс.

S1/S2/S3 snapshot JSON і первинні UART містять min/avg/max кожної задачі. [VCD S1](metrics/S1-steady.json), [VCD S3](metrics/S3-steady.json) відокремлені від 1-мс firmware timestamps. Об'єднаний час probes/serial є нижньою межею активності, не CPU idle; loop не має WFI.

## Статус кожної FR

| FR | Статус | Доказ і межа |
|---|---|---|
|01|Виконана у моделі|S1–S3: BOOT і періодичний HEART, source hash.|
|02|З обмеженням у P20|S3:30 із 30 PIR по 1 с. P20:300/300 calm,92/100 stress для 20-мс входу; межа 3.|
|03|Виконана у моделі|S2: ADC1001 до 4063, LIGHT без alarm; повернення 999.|
|04|З обмеженням|Z1.6g дає SHOCK, зокрема в ACCESS. Короткий удар не перевірено; межа 3.|
|05|Виконана для нахилу|X0.6/Z0.8g, TILT; у GUARD через 532 мс після g. Перенос без нахилу не заявлено.|
|06|Виконана у моделі|EVENT містить t_ms і bits; у journal sequence/CRC.|
|07|З обмеженням|Усі три сенсори в одній прошивці S2/S3; всі комбінації не перевірено.|
|08|Виконана у моделі|PRESENCE/LIGHT інформаційні, SHOCK/TILT у GUARD фіксують alarm.|
|09|Виконана у моделі|Fresh baseline і NVIC reset окремого diagnostic ELF повертають GUARD/alarm0. Фізичний reset не перевірено.|
|10|Виконана у моделі|ACCESS60 с, автоматичне закриття у S2; no-renewal/tick wrap у host tests.|
|11|Виконана у моделі|TILT у ACCESS без нового alarm, GUARD повторно виявляє; SHOCK у ACCESS alarm=1, k підтверджує.|
|12|З обмеженням|S2 CRC readback; diagnostic warm reset зберіг seq1–5, CRC звірено незалежно. NOR erase суперечливий, фізичний power-cut не перевірено.|
|13|Недосяжна під навантаженням|S3 finish_miss3; P20 stress пропускає 8/100 імпульсів 20 мс. Для зареєстрованих S3 max40.765 мс, P20 max32.194 мс. Межі 1–3.|
|14|З обмеженням|Runtime SDA-GND: timeout22 мс, RECOVERED через 1.659 с від release. f/v, rx_drop/ORE перевірено. Flash failure лише host model; межі 2,4.|

## Функції та відмови

ACCESS TILT t27329ms без alarm; g t38617ms і повторний GUARD TILT t39149ms з alarm. k підтверджує, a відкриває; SHOCK t54669ms у ACCESS дає alarm. f/v перевірено програмно; v t90851ms, RECOVERED t91029ms. [Логи](metrics/access-tilt.uart.txt), [GUARD](metrics/guard-tilt.uart.txt), [SHOCK](metrics/access-shock.uart.txt), [software recovery](metrics/fault-recovery.uart.txt).

Реальний сигнал SDA-GND усередині Wokwi утримувався від старту до 32.717592 с VCD. RECOVERED UART33.033380 с, observable315.789 мс після release, ID104. Recovery22 мс, loop max37 мс, stack592 B. [Первинний UART](metrics/SDA-fault-recovery.uart.txt), [аналіз](metrics/SDA-fault.json), [clip](captures/lab3/SDA-release.vcd). Це не фізична плата. Runtime SDA-GND перевірено окремо; фізичний power cut не виконано.

Перший ADC MARK завершився 39.040ms, перший sample із valid IMU61.654ms від старту моделі S1. GPIO PIR low читається до 39.103ms, фізичну готовність PIR це не встановлює. Намальований RESET не породив BOOT, спроба не зарахована.

S2 readback37 valid records seq1–37. Після 253 записів у functional run залишились 128 valid seq126–253, старі 126–128 не мали лишитися після erase сторінки. HAL success не довів NOR erase у моделі. [Rotation log](metrics/flash-rotation.uart.txt). Host tests перевіряють NOR1→0,33 byte cuts,2049 erase boundaries,33 rollover cuts. Реальні power-cut/retention залишаються невідомими. Немає fake flash delay або RAM заміни storage у прошивці.

[UART boundary](metrics/uart-boundary.json):1/8/32 n без втрат,128+LF втрачає LF,129+LF втрачає n і LF,512+LF обробляє 131 n і drop382 bytes. ORE0. Max wait1759ms. Ці рівні залежать від фактичного темпу, а не лише payload size.

## Тести та проблеми

[Linux checks і coverage](metrics/host-final.txt), [ASan/UBSan](metrics/host-sanitizers.txt), [ARM build](metrics/monotonic-build.txt). Нові cadence/store/exhibit100% lines, branch directions88.46/81.82/88.89%. Core/Platform/HAL не входять до host coverage. Старі тести ЛР 2 збережені та проходять.

1. C headers відсутні в старому image: tag не відповідав Dockerfile; окремий museguard-build:lab3 з gcc/libc-dev, docker/Dockerfile.
2. TIM3 timestamp давав backwards/unsigned giant duration; root cause snapshot/model не доведено; спроби виключено, Platform/lab3_io.c використовує HAL tick.
3. SysTick VAL fine snapshot теж немонотонний; sub-tick метод вилучено, VCD незалежно перевіряє фінальні 1-мс виміри.
4. z всередині циклу змішував measurement windows; runner.c відкладає reset до наступного poll і копіює stats перед printf.
5. Wokwi startup flash нульова/corrupt128; store scan відкидає invalid і викликає HAL erase для нової сторінки.
6. Rollover лишив stale valid records попри HAL success; модель не підтверджує erase, виправлення симулятора відсутнє, host NOR tests та явне обмеження.
7. S2/S3 misses і UART drops збережені як результат меж суперциклу, H1/H2 задають наступну перевірку.

Зміни прошивки й інструментування зафіксовано в коміті `cc00eb259b0fa753e109e5b3a63be4af807fa170`, сценарії та аналіз — у `dd83e5444913b46fae5f68193f52081fb2bf1e38`. Виміряний ELF зберігає власний source hash, зазначений вище; коміти створено після вимірювань.

Перевірену версію ідентифікують source manifest, source SHA-256 та ELF hash. Для кожної зміни вказано відповідний модуль. Компоненти ЛР 2 залишено у власному repo, ліцензії vendor збережено.

## Ролі та підсумок

Стенд у цьому виконанні Wokwi; фізичні характеристики плати не перевірені.

Найбільшу виміряну проблему створює невитіснюваний command/output/recovery foreground. Пріоритети та blocking worker ОС можуть допомогти, як і неблокувальна реалізація без ОС. Остаточну потребу в ОС слід обґрунтувати повторними H1–H6 і її RAM/FLASH вартістю. Super loop має один MSP і простий послідовний debug; числової переваги над ще не реалізованою ОС за startup/енергією не заявлено.

Непідтверджені технічні властивості: фізичні power cuts/reset/retention, реальний короткий удар, електричні параметри та повна статична libc stack bound. P20/model-reset/mallinfo виконано окремими перевірками, наведеними нижче. Host tests не встановлюють фізичні властивості плати.


## P20, runtime SDA та окрема діагностика

[P20 raw analysis](metrics/P20.json) містить 400 pulse high20.000ms із tolerance±0.025us, всі 20phase bins у кожній групі 100. Calm групи 1/2/4 дали 300/300 EVENT, stress група 3 дала 92/100,37 фронтів при COMMAND high. Для отриманих calm min/avg/max3.168/13.215/23.115ms, stress3.260/16.990/32.194ms. Вісім відсутніх EVENT не включено як нульову latency. Decoded VCD та terminal UART містять 392 PRESENCE, без пізніх orphan подій. Це пряма втрата при polling під навантаженням. Для 6 pulse немає SAMPLE rise протягом high, для 2 PIR читається після завершення pulse. [Fixture](../tests/fixtures/phase20/README.md) дозволяє повторення на ЛР 4.

[Runtime SDA raw](metrics/SDA-runtime.uart.txt) на тому самому baseline ELF: low7.157821–20.375030s, sample timeout22ms, loop31ms, stack664B. RECOVERED UART22.033634s, через 1658.604ms після release. Початковий id0 змінився на 104 після retry, причина transient не встановлена. Baseline peak тепер 664B із 2048B, reserve1384B лише для виконаних шляхів. S1–S3 tables залишають власні watermark584B.

[Heap/reset diagnostic manifest](metrics/heap-reset-probe-manifest.json) відокремлює ELF0e9691bc5eac504dd6862d41775565aca9f17ed85b6ef11ee0426daac9281171 від базового. Він має FLASH24800/linkerRAM5008B. Три mallinfo snapshots до/після workload та reset: arena1032/uordblks1032/fordblks0B, включно з allocator bookkeeping. Його stack648B не змішується зbaseline. [Raw](metrics/heap-reset-after.uart.txt). Після NVIC_SystemReset із access1/alarm1 новий BOOT повернув GUARD/alarm0 і зберіг seq1–5. Всі 8 records після додавання 6–8 незалежно перевірено zlib CRC. [Підсумок](metrics/retest-results.json). Це model warm-reset retention; фізичні power cut/NOR erase лишаються обмеженнями. Перелінкування через tools/build_heap_diagnostic.py дало той самий hash. Toolbar Restart запустив default Hello STM32 і виключений зMuseGuard доказів.
