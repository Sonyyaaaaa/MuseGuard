# Runtime діагностичних ELF, 15.11.2026

Nucleo-C031C6, 48 MHz, Wokwi 4.1.0 / Code 1.146.0, чинна Community license. Два окремі ELF зберігають прикладні функції й додають TIM3/trace облік; вихідні файли та manifests збережені у runtime-source, runtime100-source та diagnostic-source-manifest.json.

TIM3 має PSC=47, ARR=65535, крок 1 мкс. traceTASK_SWITCHED_IN додає unsigned16 різницю CNT до попередньої задачі. Всі інтервали коротші за 65 мс, bad=0 в обох запусках. Три кошики sensor/service/idle взаємовиключні; частина IRQ і ядра між hooks приписується поточній задачі. Це не чистий час лише прикладних інструкцій та не окрема частка ядра.

1000 Hz: sensor/service/idle 11.938728/25.952989/62.108283%, сума counters 250.073477 s, початок рядка RUNTIME у VCD 250.1191185 s; 409528 точок обліку. 100 Hz: 11.926851/12.716439/75.356711%, сума 250.110449 s, рядок у VCD 250.1563995 s; 59028 точок. Різниця близько 46 мс містить startup до TIM3 та вивід. Це перевірка масштабу часу на всьому інтервалі, не фізичне калібрування.

На обох S3 отримано 12400 sample, 30 PIR, skipped=finish_miss=0. Незалежний VCD містить 12153 фронти UART_RX/PIR із тим самим порядком і рівнями; максимальна різниця timestamp 25 ns. Байтова ідентичність хешів timestamp-масивів не очікується за такої дискретизації; runtime-input-equality.json зберігає обидва хеші й порівняння з допуском.

За 100 Hz HAL timebase та service delay адаптовано до 10 ms, sensor period збережено 20 ms. Діагностичні FLASH/RAM: 28264/11456 B та 28284/11456 B. Вартість hooks входить у результати. Дані попередніх GPIO-маркерів TICK/SWITCH перекриваються з runtime і не додаються до 100%.

Повні UART, native VCD, ELF/HEX та analysis.json — stands/runtime1000 і stands/runtime100. Продуктовий ELF ea0b2d79cbef504014e5bba4b70fdfa6b6061507f528a24c0a8b060cc6b33044 не змінено; це не новий S2.
