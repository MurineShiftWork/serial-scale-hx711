# Hardware

## Components

| Part | Example | Notes |
|---|---|---|
| Arduino Uno | Any Uno-compatible board | USB-A to USB-B cable for host connection |
| HX711 amplifier | SparkFun SEN-13879 | 24-bit ADC; 10 Hz or 80 Hz sample rate |
| Load cell 100 g | SparkFun SEN-14727 | For small-rodent water rewards |
| Load cell 500 g | SparkFun SEN-14728 | For heavier payloads or food hopper |

## Firmware

Flash the Arduino with the firmware from `scale_firmware/` in this repository.
The firmware uses [olkal/HX711_ADC](https://github.com/olkal/HX711_ADC) and reports
weight over serial at 115200 baud. The scale identifies itself as
`<SerialWeighingScale>` when first connected; the driver waits for this string during
`Scale.start()`.

## Wiring

Connect the HX711 board to the Arduino and wire the load cell to the HX711:

```
Load cell   HX711
----------  ------
Red (E+)    E+
Black (E-)  E-
White (A-)  A-
Green (A+)  A+

HX711       Arduino Uno
----------  -----------
VCC         3.3 V (or 5 V for HX711 modules with built-in regulator)
GND         GND
DT          pin 4
SCK         pin 5
```

## Calibration

The calibration factor is stored in Arduino EEPROM and loaded on boot.
To recalibrate:

1. Connect the scale and run `Scale.start()`.
2. Place a known weight on the load cell.
3. Send the calibration command via serial or use the firmware's interactive prompt.
4. The new factor is saved to EEPROM automatically.

The `Scale.get_calibration_factor()` method reads the current stored value.
