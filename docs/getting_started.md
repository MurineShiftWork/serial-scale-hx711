# Getting Started

## Installation

```bash
pip install serial-scale-hx711
```

Python 3.10 or later is required. The only runtime dependency is
[pyserial](https://pypi.org/project/pyserial/).

## Hardware setup

You need three components:

| Component | Example part |
|---|---|
| Arduino Uno | any USB-serial Arduino |
| HX711 amplifier | SparkFun SEN-13879 |
| Load cell | SparkFun SEN-14727 (100 g) or SEN-14728 (500 g) |

### Wiring

Connect the HX711 to the Arduino using the pin assignments hard-coded in
the firmware:

| HX711 pin | Arduino pin |
|---|---|
| DOUT | Digital 2 |
| SCK | Digital 3 |
| VCC | 5 V |
| GND | GND |

Connect the load cell to the HX711 following the colour-coding printed on
the amplifier breakout board (typically: red = E+, black = E-, white = A-,
green = A+).

### Firmware

Flash `scale_firmware/scale_firmware.ino` to the Arduino using the Arduino
IDE. The sketch depends on the
[HX711_ADC](https://github.com/olkal/HX711_ADC) library (install via the
Arduino Library Manager). After flashing, the scale communicates at
**115200 baud** and identifies itself as `<SerialWeighingScale>`.

## First reading

Plug the Arduino in via USB. Find its port (e.g. `/dev/ttyACM0` on Linux,
`COM3` on Windows) then run:

```python
from serial_scale_hx711 import Scale

scale = Scale(serial_port="/dev/ttyACM0")
scale.start()          # open port and wait for firmware to finish taring
print(scale.read_weight())  # prints weight in grams
scale.disconnect()
```

`start()` polls the firmware until the HX711 tare is complete (up to 10 s
by default). If you do not know which port to use, see
[Usage - auto-detecting the port](usage.md#auto-detecting-the-port).
