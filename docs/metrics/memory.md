# Пам'ять фінального ELF ЛР3

Джерело: monotonic-build.txt, final-memory-toolchain.txt, source-manifest.json. ARM GCC 13.2.1, -Og, sections GC, Cortex-M0+, STM32C031C6. Значення не включають debug-секції ELF, які не завантажуються до MCU.

| Область | Значення | Інтерпретація |
|---|---:|---|
| FLASH, образ + initializers | 23872 B / 28672 B | 83.26%, запас коду 4800 B |
| Journal | 4096 B | фізичні сторінки 14–15, поза linker FLASH region |
| RAM, .data | 248 B | initialized runtime data |
| RAM, .bss | 2156 B | static zero data |
| Static runtime RAM | 2404 B | .data + .bss |
| Heap/stack reservation з padding | 2564 B | 512 B мінімум heap, 2048 B MSP та alignment |
| Linker RAM total | 4968 B / 12288 B | 40.43%, не фактичний heap/stack peak |
| Стек S1 | 584 B / 2048 B | sentinel від main, консервативна межа з guard 16 words |
| Heap S1 | 1032 B | поточна арена sbrk у snapshot; actual live used/free не виміряно |

`size` друкує text=23616, data=256, bss=4720. Тут data включає два 4-byte масиви ініціалізації у flash, а bss включає резерв heap/stack. Тому не можна оголосити data+bss фактичним SRAM peak. Розміри .o у raw log є верхніми оцінками до garbage collection; точні адреси й секції є у firmware.map.

Найбільші власні SRAM об'єкти: RX timestamps 128×4=512 B; RX bytes 128 B; pending museum records 16×12=192 B; timing і report_timing по 192 B. Буфери мають сталу місткість, malloc у прикладному коді немає. Newlib stdout збільшує sbrk-арену. Поле RESOURCE heap_peak відповідає відстані поточного break від _end; у цьому навантаженні воно не зменшувалось. Це не malloc used/free і не доказ відсутності всіх витоків усередині вже виділеної арени.

Статичні .su: main 8 B, runner_poll 40 B, sample_task 88 B, output_task 96 B, journal_task 56 B, mg_store_append 88 B. Відомий ланцюг запису без printf: 8+40+56+88+16+24+16 = 248 B до врахування exception context та вкладених функцій. Ланцюг main/poll/output має 144 B власних frame плюс printf/newlib. Для prebuilt newlib .su немає, тому повна статична верхня межа не встановлена. Вимір sentinel покриває виконані шляхи після main, а не всі можливі шляхи чи startup до paint.

S2 понад годину та S3: stack=584 B, heap arena=1032 B. При boot-with-SDA-low у моделі Wokwi stack=592 B, heap arena=1032 B. Після додаткового runtime SDA тесту поточний найбільший baseline sentinel peak становить664 B, резерв MSP2048 B і спостережений залишок1384 B. Обсяг .su не дає повної статичної межі для prebuilt libc; цей залишок стосується виконаних шляхів.

## Додаткові базові та діагностичні runs

P20 на тому самому базовому ELF дав stack snapshot600 B, runtime SDA fault664 B. Загальний спостережений baseline peak тепер664 B із2048 B, запас1384 B для виконаних шляхів. S1/S2/S3 watermark584 B і boot-with-SDA-low592 B залишаються власними результатами цих сценаріїв. Попередній максимум592 B замінено додатковим виміром664 B, не іншим розміром reserve.

Окремий heap-reset diagnostic ELF0e9691bc5eac504dd6862d41775565aca9f17ed85b6ef11ee0426daac9281171 має FLASH24800 B і linker RAM5008 B, додатковий mallinfo wrapper. Три snapshots дали arena1032/uordblks1032/fordblks0 B, до та після навантаження і reset. uordblks включає overhead allocator, ordblks_raw у nano не оновлюється. Його stack648 B не включено до baseline peak. Це доступний runtime live-heap результат окремої інструментальної збірки, а не ретроспективний вимір free протягом години S2. [Raw](heap-reset-after.uart.txt), [manifest](heap-reset-probe-manifest.json), [доступні libc frames](libc-stack.md).
