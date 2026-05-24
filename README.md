# serial-scale-hx711

Python driver for Arduino+HX711 serial weighing scales.

> **Renamed from `serial-weighing-scale`** (PyPI: `serial-weighing-scale` ≤ 2.0.4).
> New releases publish under `serial-scale-hx711`. A deprecation stub remains on PyPI
> under the old name pointing here.

## Hardware

- Arduino Uno (USB-A to USB-B cable)
- HX711 load cell amplifier (e.g. SparkFun SEN-13879)
- Load cell 100 g or 500 g (e.g. SEN-14727 / SEN-14728)
- Firmware: see `scale_firmware/` (uses [olkal/HX711_ADC](https://github.com/olkal/HX711_ADC))

The scale identifies itself as `<SerialWeighingScale>` over 115200 baud.

## Install

```bash
pip install serial-scale-hx711
```

Or editable from this repo:

```bash
pip install -e .
```

## Usage

```python
from serial_scale_hx711 import Scale

scale = Scale(serial_port="/dev/ttyACM1")
scale.start()          # connect and wait for firmware init
scale.tare()
weight = scale.read_weight_blocking()   # grams, blocks until stable
scale.disconnect()
```

### Auto-detect port

```python
from serial_scale_hx711 import connect_serial_scale

scale = connect_serial_scale(["/dev/ttyACM0", "/dev/ttyACM1", "/dev/ttyACM2"])
```

## API

| Method | Description |
|---|---|
| `start(timeout=10)` | Connect and wait for firmware ready |
| `tare()` | Zero the scale |
| `read_weight()` | Single reading (float or None) |
| `read_weight_blocking(n_valid, timeout)` | Block until N valid readings, return median |
| `read_weight_reliable(n_readings, measure)` | Repeated reads with custom aggregation |
| `identify()` | True if firmware responds with identity string |
| `get_calibration_factor()` | Read calibration factor from firmware |
| `disconnect()` | Close serial connection |

## murineshiftwork integration

Used via `murineshiftwork.logic.scale.SerialWeighingScaleAdapter`. Install with:

```bash
pip install "murineshiftwork[calibration]"
```
