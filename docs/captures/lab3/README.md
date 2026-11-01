# Власні фрагменти ЛР3

Ці VCD обрізано з власних справжніх записів Wokwi через tools/clip_vcd.py. Source SHA-256, початок і кінець у наносекундах вбудовано в $comment, час фрагмента починається з0. Переходи не синтезовано.

- S1-steady.vcd:13.0–13.15s первинного S1, для регулярного sample.
- S3-worst-presence.vcd:76.064–76.120s первинного S3, найгірший PIR→початок UART EVENT40.765ms. PIR фронт припав у кінці sample, наступне читання й command backlog відклали повідомлення.
- SDA-release.vcd:32.7–33.1s власного SDA-fault run, release SDA і подальший RECOVERED. Це сигнал у Wokwi, не фізична плата.

Канали SCL/SDA/UART_TX/PIR/ADC_MARK/SAMPLE/FLASH/COMMAND, UART115200 8N1, original timescale1ns. Повні первинні файли та run-manifest знаходяться у локальному Lab03/Work, не підміняються цими короткими clips. S2-start.vcd містить лише початок години; тривалість S2 встановлено за MCU UART без reset/reload. [Результати](../../lab3-report.md), [аналіз S3](../../metrics/S3-steady.json).

Повторний сеанс: P20-missed-input.vcd51.991–52.071s, реальний20ms pulse без SAMPLE під час high. NVIC-reset.vcd24.235–24.380s стосується окремого diagnostic ELF і містить RESETDIAG та новий BOOT. SDA-runtime-release.vcd20.36–22.06s стосується baseline ELF і показує release та RECOVERED через1.659s. Повні source hashes та offset містяться в кожному clip. Toolbar default Hello STM32 captures виключені з MuseGuard результатів.
# Startup NACK / ACK

`startup-nack.vcd` та `startup-ack-sample.vcd` обрізано з власного повного `startup-ready.vcd`, SHA256 b8467ccb88e6bf43ed9935f8c1b8972a5707c5ee2e1aebcd2a377e29061a1d90, відповідно з вікон 0.035–0.045 с і 5.005–5.035 с. Час у clips починається від нуля відповідного вікна; переходи збережені. Умови і обмеження — у tests/fixtures/startup-not-ready/README.md.
