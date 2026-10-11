#!/usr/bin/env python3
"""Keep firmware configuration and simulated circuit consistent."""
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
diagram = json.loads((root / "diagram.json").read_text())
connections = {frozenset(c[:2]) for c in diagram["connections"]}
expected = [
    ("imu:SCL", "nucleo:PB8"), ("imu:SDA", "nucleo:PB9"),
    ("imu:AD0", "nucleo:GND.1"), ("imu:VCC", "nucleo:3V3.1"),
    ("pir:OUT", "nucleo:PA10"), ("pir:VCC", "nucleo:5V.1"),
    ("ldr:AO", "nucleo:PA0"), ("ldr:VCC", "nucleo:3V3.1"),
    ("logic:D0", "nucleo:PB8"), ("logic:D1", "nucleo:PB9"),
    ("logic:D2", "nucleo:PA2"), ("logic:D3", "nucleo:PA10"),
    ("logic:D4", "nucleo:PA4"), ("logic:GND", "nucleo:GND.1"),
]
for link in expected:
    assert frozenset(link) in connections, f"Missing connection {link}"
ioc = (root / "firmware/firmware.ioc").read_text()
for setting in ("PA0.Signal=ADC1_IN0", "PA10.Signal=GPIO_Input",
                "PA4.Signal=GPIO_Output", "PB8.Signal=I2C1_SCL",
                "PB9.Signal=I2C1_SDA", "PA2.Signal=USART2_TX", "PA3.Signal=USART2_RX"):
    assert setting in ioc, f"Missing CubeMX setting {setting}"
print("PASS: diagram.json matches the GPIO/ADC/I2C/UART configuration")
