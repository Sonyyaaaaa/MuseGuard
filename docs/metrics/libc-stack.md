# Доступна оцінка libc та heap без нового запуску

Базовий ELF має SHA-256 b1d755384c47e32e24f800c49f8198b86e86a10a2bb0351238fe101361dd63fc. ARM GCC13.2.1, multilib thumb/v6-m/nofp, newlib nano. Дизасемблювання цього ELF доповнює власні .su, але не дає автоматично глобальної верхньої межі.

| Функція | Локальний frame, B | Підстава |
|---|---:|---|
| printf, alias iprintf | 32 | push r0–r3, push lr, sub sp12 |
| _vfprintf_r, alias _vfiprintf_r | 160 | push20, push16, sub sp124 |
| _printf_i | 64 | push20, push16, sub sp28 |
| _printf_common | 40 | push24, push16 |
| __smakebuf_r | 112 | push20, sub sp92 |
| _malloc_r | 32 | push24, push8 |
| _free_r | 16 | push16 |
| __sfvwrite_r | 48 | push20, push16, sub sp12 |
| __swrite | 24 | push24 |
| _write_r / _write | 16 /16 | push16 кожна |
| __io_putchar | 16 | push8, sub sp8 |
| HAL_UART_Transmit | 32 | push20, push4, sub sp8 |

Це фактичні prologue allocations, а не максимум вкладеного виклику. Наприклад відомий фрагмент main/poll/output/printf/vfprintf/printf_i/printf_common має суму8+40+96+32+160+64+40=440 B, якщо ці frames одночасно активні на відповідному шляху. Він ще не включає всю передачу потоку й IRQ та не є верхньою межею. У print/stream code є непрямі виклики через registers і FILE callbacks; усі їхні targets, альтернативні шляхи та stack restoration треба довести для глобального bound. У повному disassembly27 blx/jump-table sites, частина не є окремими викликами. Їх не можна просто сумувати як27 frames.

Compiler .su: SysTick_Handler8 B, USART2_IRQHandler16 B, lab3_us8 B, EXTI4_15_IRQHandler8 B, HAL_GPIO_EXTI_IRQHandler8 B; HAL_GetTick і HAL_IncTick0 B. Hardware exception entry окремо зберігає8 words32 B і можливий alignment word4 B. Налаштовані пріоритети SysTick3, UART1, EXTI0 допускають nesting; lab3_us на короткий час зберігає PRIMASK і маскує IRQ. Довільні HardFault/NMI шляхи не оцінено. Тому runtime baseline watermark664 B і неповний static fragment440 B не є двома взаємозамінними гарантіями.

Бібліотека libc_nano.a надає mallinfo/_mallinfo_r і структуру mallinfo у malloc.h. Базовий ELF містить _malloc_r, _free_r, __malloc_free_list, __malloc_sbrk_start, але mallinfo відкинуто section GC, оскільки його не викликають. Значення RESOURCE heap_peak1032 B показує поточний sbrk break−_end, не розподіл allocated/free. За одним лише цим числом free не визначається.

mallinfo реалізує arena як поточний break мінус __malloc_sbrk_start, fordblks як суму вільних chunks і uordblks як arena−fordblks. Allocated chunks включають службову структуру allocator; це не чиста сума requested payload. Для виміру підготовлено окремий діагностичний link із wrapper, який друкує ці поля перед RESOURCE. Його власні stacks/FLASH і timing відрізняються, тому його результат не додається до S1–S3 як начебто та сама збірка.

Первинні disassembly, символи, malloc.h і linker arguments збережено в локальному Lab03/Work. Діагностичний ELF виконано: mallinfo arena1032 B, uordblks1032 B, fordblks0 B у трьох snapshots. Дані та його окремий ELF hash наведено в memory.md. Відсутність плати не заважає цій перевірці у Wokwi; фізичні power-cut/електричні властивості залишаються іншою категорією невідомого.
