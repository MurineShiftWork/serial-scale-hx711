# Usage

## Basic connection

```python
from serial_scale_hx711 import Scale

scale = Scale(serial_port="/dev/ttyACM0")
scale.start()   # open port and wait for firmware initialisation
```

Always call `start()` before reading. It blocks until the firmware has
finished its power-on tare (up to `timeout` seconds, default 10).

## Auto-detecting the port

`connect_serial_scale` tries each port in order and returns the first one
that responds correctly:

```python
from serial_scale_hx711 import connect_serial_scale

scale = connect_serial_scale(["/dev/ttyACM0", "/dev/ttyACM1", "/dev/ttyACM2"])
if scale is None:
    raise RuntimeError("No scale found")
```

The default port list (`DEFAULT_TEST_PORTS`) covers `/dev/ttyACM0` through
`/dev/ttyACM4`.

## Taring

Remove all weight from the platform, then call `tare()` to zero the scale:

```python
scale.tare()
```

`tare()` waits 500 ms after sending the command so that the firmware
rolling-average buffer fully flushes before the next read.

## Single reading

```python
weight = scale.read_weight()   # float (grams) or None on parse error
```

A return value of `None` means the firmware sent an unparseable response;
check the serial connection and try again.

## Stable / averaged readings

For a more reliable measurement, use one of the higher-level helpers.

### Fixed number of reads with statistical reduction

```python
weight = scale.read_weight_reliable(n_readings=10, inter_read_delay=0.05)
# default measure is statistics.median; pass any callable
import statistics
weight = scale.read_weight_reliable(n_readings=10, measure=statistics.mean)
```

### Blocking until N valid reads are collected

Use this when you need to be sure the measurement is trustworthy before
proceeding, for example in an automated calibration workflow:

```python
weight = scale.read_weight_blocking(n_valid=5, timeout=30)
```

`read_weight_blocking` collects reads until `n_valid` non-None values have
been received, then returns their median. It raises `TimeoutError` if that
does not happen within `timeout` seconds.

### Collecting raw repeated reads

```python
readings = scale.read_weight_repeated(n_readings=10, inter_read_delay=0.1)
# returns a list of valid (non-None) floats
```

## Continuous reading loop

```python
import time

scale.tare()
try:
    while True:
        w = scale.read_weight()
        if w is not None:
            print(f"{w:.2f} g")
        time.sleep(0.1)
except KeyboardInterrupt:
    pass
finally:
    scale.disconnect()
```

## Reading the calibration factor

The firmware stores a fixed calibration factor compiled into the sketch.
You can read it back at runtime for logging or diagnostics:

```python
factor = scale.get_calibration_factor()
print(f"Calibration factor: {factor}")
```

To change the calibration factor you need to edit `CALIBRATION_FACTOR` in
`scale_firmware/scale_firmware.ino` and re-flash the Arduino.

## Disconnecting

```python
scale.disconnect()
```

`disconnect()` is also called automatically when the `Scale` object is
garbage-collected, but calling it explicitly is recommended.
