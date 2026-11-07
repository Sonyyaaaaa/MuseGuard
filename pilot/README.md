# Мінімальний пілот FreeRTOS для C031

Це окрема діагностична прошивка, не реалізація MuseGuard. Ядро: `freertos/SOURCE.md`. Зберігає linker layout ЛР3, зокрема 4 KiB журналу, але сам журнал і датчики в пілоті не використовуються.

```sh
docker run --rm -v "$PWD:/workspace" -w /workspace --user "$(id -u):$(id -g)" museguard-build:lab3 make -B -C pilot
```

Збірка: `firmware/build/pilot/pilot.elf`. Для окремого запуску в Wokwi використати `diagram.pilot.json` та Upload Firmware and Start Simulation з цим ELF. PA4/D4 позначає виникнення тестового notification у SysTick ISR; PA6/D5 — виконання event task; PA7/D6 — роботу нижчопріоритетного load task. PA2/D2 — UART 115200 8N1. Подія програмна, а не зовнішній сенсор. Мета — простежити, як ISR будить заблоковану high-priority task та вона випереджає load; самі маркери ще не є виміром вартості context switch. Раз на секунду UART повідомляє HAL/RTOS ticks, число подій, free/minimum heap і watermarks periodic,event,load,idle у словах по 4 B. Load навмисно не блокується; idle може не виконуватися, тому його watermark не перевіряє робочий шлях idle.

Після Stop зберегти оригінальні UART/VCD і hash саме запущеного ELF у `Lab04/Work`. Перевірити кілька послідовних повідомлень, збільшення обох лічильників часу і подій, порядок фронтів та те, що PA7 повертається після PA6. Залежність обох годинників від одного SysTick не є незалежним виміром часу. UART та GPIO за відсутності запуску не вважаються перевіреними.

HAL_Init і RCC налаштовують SysTick до старту ОС. Після старту один ISR викликає HAL_IncTick і port tick; kernel tick не викликається до запуску планувальника. Tickless вимкнено. Затримки HAL усередині ISR або з вимкненими перериваннями неприпустимі. Витіснення, час маскування IRQ, стеки та робота під збоєм потребують фактичного виміру. Failed allocations, stack overflow та assert ведуть у зупинку; це не відновлення продукту.
