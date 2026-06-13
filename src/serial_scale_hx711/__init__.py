"""Python driver for Arduino+HX711 serial weighing scales.

The main entry point is :class:`Scale`. Use :func:`connect_serial_scale` to
auto-detect the port when it is not known in advance.
"""

__author__ = "Lars B. Rollik"

from importlib.metadata import PackageNotFoundError, version

from serial_scale_hx711.scale import Scale

try:
    __version__ = version("serial-scale-hx711")
except PackageNotFoundError:
    __version__ = "unknown"

DEFAULT_TEST_PORTS = [f"/dev/ttyACM{x}" for x in range(5)]

# Backwards-compatibility alias for code that imported SerialWeighingScale
SerialWeighingScale = Scale


def connect_serial_scale(
    serial_port_list: list = DEFAULT_TEST_PORTS,
) -> Scale | None:
    """Connect to the first available serial scale from the provided list of ports."""
    from serial import SerialException

    for serial_port in serial_port_list:
        try:
            scale = Scale(serial_port=serial_port)
            scale.start()
            return scale
        except SerialException:
            pass

    return None


__all__ = ["Scale", "SerialWeighingScale", "connect_serial_scale", "__version__"]
