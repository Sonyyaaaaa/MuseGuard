# Межі суперциклу MuseGuard

Це результати одного C031C6 ELF з інструментуванням. Спостереження відокремлені від причин і гіпотез. Цілі взято з FR-13. ОС у ЛР3 не додано.

## 1. Спільний foreground затримує sample

Факт: S2 skipped1/finish_miss1, S3 skipped2/finish_miss3; S3 VCD max period36.665 мс при цілі20 мс. Синхронний UART у command/output і flash працюють у тому самому poll, їх не можна перервати іншою foreground задачею. S2 output max26 мс і loop29 мс узгоджуються з цією причиною, але VCD потрібного циклу S2 вже не записувався, точний винуватець одиничного пропуску невідомий. Докази scenarios.csv, S3-steady.json і runner.c.

Без ОС: TX IRQ ring, обмежений byte budget на poll, розділення довгих операцій на стани. ОС: sensor task вищого пріоритету і UART worker, що блокується на IRQ notification. Витрати: окремі stacks/TCB, queues, context switches. H1: за тим самим S3 release_max≤2 мс та finish_miss=0. Busy-wait HAL усередині високопріоритетної задачі цього не забезпечить.

## 2. Приймальна черга має скінченну місткість

Факт: 127 корисних байтів SPSC ring. n*128+LF спричинив drop1, усі128 n оброблені; n*129+LF дав drop2 та128 відповідей, отже втрачено n і LF. n*512+LF дав131 оброблений n та382 dropped bytes. ORE=0, це програмне переповнення. Max очікування1759 мс, для8 команд уже108 мс. Доказ uart-boundary.json і первинні UART.

Причина: відповідь на n значно довша за однобайтовий запит. Без ОС: throttling/ACK protocol, коротші replies, bounded TX/RX queues і явна помилка busy. ОС: queue з backpressure й UART worker. Витрати: RAM черги і синхронізація. H2: повторити ті самі рівні; ОС сама не збільшить115200 bit/s, без зміни протоколу відсутність drop не обіцяється.

## 3. Polling втрачає короткі входи під навантаженням

Факт P20:400 pulse high20.000ms, кожна група100 охоплює20phase bins. Calm300/300 EVENT, stress92/100,37 фронтів при COMMAND high. Вісім відсутніх EVENT є виміром, на відміну від попередньої гіпотези за max gap36.665ms. Для6 пропущених pulse не було SAMPLE rise під час high, для2 PIR читається наприкінці task після fall. Event_drop/flash_fail/RX_drop/ORE0, terminal і VCD містять392 PRESENCE. Фізичний короткий удар на MPU6050 цим input fixture не перевіряється.

Без ОС: EXTI latch, IMU data-ready/FIFO за підтвердженої підтримки, зменшення невитіснюваних фрагментів. ОС: ISR notification окремій пріоритетній sensor task; вона не відновить сигнал, який периферія не зафіксувала. Ціна: ISR/queue RAM, IRQ latency і правильне reset/rearm. H3: повторити той самий400-pulse сценарій, ціль0 пропусків та reaction≤100ms. Фізичний стенд є окремою перевіркою після формиB. [P20](metrics/P20.json), [fixture](../tests/fixtures/phase20/README.md).

## 4. Recovery теж блокує сусідні задачі

Факт: схема SDA-GND у Wokwi дає повторні I²C timeout; recovery min/avg/max22/22/22 мс у першому snapshot, loop max37 мс, release max30 мс. Після розмикання збережено RECOVERED і WHO_AM_I104. Стек592 B. Доказ SDA-fault-recovery.uart.txt і SDA-fault.json. Це справжній зіпсований сигнал усередині моделі, не фізичний стенд.

Без ОС: неблокувальний I²C state machine, timeout по одному кроку. ОС: нижчий пріоритет recovery, peripheral mutex, ISR completion. Mutex має бути коротким; priority inheritance зменшує inversion, не тривалість самого transfer. Витрати: mutex/TCB/stack та контроль starvation. H4: при утриманій SDA sample для ADC/PIR зберігає20 мс без misses, IMU позначено invalid.

## 5. Бюджет пам'яті обмежує розбиття на потоки

Факт: образ23872 із28672 B, запас4800 B плюс окремий journal4096 B. Static runtime RAM2404 B, MSP reserve2048 B, heap arena1032 B. Найбільший виконаний baseline stack664 B, повна статична верхня межа printf невідома. Доказ memory.md і map/su logs.

Без ОС: статичні буфери, bounded formatting, прибрати дублювання snapshots після окремого виміру. ОС: static task creation та stack watermarks. Наприклад stacks512+1024+512=2048 B додатково до збереженого MSP reserve; це арифметичний план, не реальна витрата kernel. TCB/queues/kernel ще невідомі. H5: після S1–S3 і SDA fault кожен stack має≥25% невикористаного резерву, linker FLASH≤28672 B, немає heap growth між рівнозначними snapshots. Якщо умови не виконано, переглянути розбиття задач.

## 6. Очікування не переводить ядро в сон

Факт за кодом: poll виконується без WFI. Busy estimate10.7% у S2 рахує вибрані задачі, решту часу ядро обертає loop. Це не вимір енергії й не89.3% фізичного sleep. Без ОС можна додати WFI із race-safe перевіркою pending work. ОС може використовувати idle hook/tickless лише після налаштування. Витрати: wake-up latency і коректне джерело часу. H6: на платі порівняти струм bare loop, bare WFI та RTOS idle, зберігши reaction≤100 мс; PWR у Wokwi не моделюється.

## Обмеження журналу та вимірювання

NOR-контракт дає мінімум64 останні committed records, дві сторінки по2 KiB. ОС не збільшить місткість і ресурс стирання. Host NOR model перевіряє1→0, CRC, torn writes, усі2049 позицій перерваного page erase та33 позиції rollover program. Actual power cuts не виконані.

У Wokwi після253 записів залишились valid seq126–128 у slots61–63 сторінки, яка мала бути стерта при seq193. Правильно стерта сторінка лишила б125 valid seq129–253, а не128 seq126–253. HAL повернув success. Невідповідність flash model є обґрунтованою гіпотезою; фізичний erase і retention цим запуском не підтверджено. Firmware викликає HAL erase/program; host NOR model перевіряє алгоритм за контрактом окремо від MCU.

PA7 у S3 показав близько1.4 мс на append із чотирьох doublewords; boot operation з erase також близько1.455 мс. DS13867 rev4 table43 дає typ22/max40 мс для page erase та typ85/max125 мкс для64-bit program. Цифри симулятора не є фізичними межами. Виконання з того самого flash bank і SysTick під час фізичного erase потребують зовнішнього аналізатора, ОС саме по собі цього не усуне.

Фінальний clock=SysTick_1ms; короткі інтервали квантуються до1 мс, mean має фазове зміщення. VCD незалежно дає SAMPLE≈2.348 мс, firmware mean≈2.006–2.056 мс. Task busy не CPU utilization. У базовому ELF виміряно лише sbrk arena; live malloc used/free перевірено окремою діагностичною збіркою нижче. Повна .su верхня межа libc відсутня. Ранні TIM3 та SysTick VAL спроби дали немонотонність і виключені. clock_ambiguous=0 фінальних запусків не доводить відсутності фізично втрачених ticks при stall.

Джерела: [ST DS13867](https://www.st.com/resource/en/datasheet/stm32c031c6.pdf), [Wokwi C031](https://docs.wokwi.com/parts/board-st-nucleo-c031c6), [FreeRTOS scheduling](https://github.freertos.org/Documentation/02-Kernel/02-Kernel-features/01-Tasks-and-co-routines/04-Task-scheduling). Для гіпотез ОС це опис механізму, а не результат вимірювання.

## Уточнення за виконаним повторним тестом

Обмеження3 тепер має прямий вимір P20:400 точних20-ms pulse, три calm групи300/300 EVENT і stress група92/100, усі20 phase bins у кожній групі. Під час stress37 фронтів припали на COMMAND high. Для6 пропущених pulse не було SAMPLE rise під час high, для2 task почалась біля кінця high і читала PIR після fall. Event_drop/flash_fail/RX_drop/ORE0, decoded raw містить рівно392 PRESENCE. Отже вісім відсутніх подій є виміром втрати при polling; попередній висновок із gap був гіпотезою до цього тесту. H3 тепер слід перевірити тим самим400-pulse fixture на моделі, ціль0 пропусків і reaction≤100ms. Фізичний удар на MPU не відтворено. [P20](metrics/P20.json), [input fixture](../tests/fixtures/phase20/phase20.chip.c).

Обмеження4: runtime SDA low7.157821–20.375030s; sample timeout max22ms, recovery max22ms, loop31ms, stack664B. RECOVERED UART22.033634s, тобто1658.604ms від release. Після release c спершу дав id0, потім id104; причина транзієнта остаточно не встановлена. Retry раз/с не гарантує повне відновлення за1s. Це доповнює boot-with-SDA-low, де loop37ms і stack592B. [Runtime raw](metrics/SDA-runtime.uart.txt).

Обмеження5: новий baseline stack peak664B із2048B, observed reserve1384B. Окремий diagnostic link показав mallinfo allocated_chunks1032/free_chunks0B та warm reset із GUARD/alarm0, retained seq1–5 і незалежно звіреним CRC. Ця перевірка закриває доступність live heap/reset у моделі, але не physical power cut, NOR erase або повний static stack bound. Таблиці S1–S3 не змінюються числами іншої інструментальної збірки. [Результат](metrics/retest-results.json), [ресурси](metrics/memory.md).
