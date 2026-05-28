import sys


def test_python_version() -> None:
    assert sys.version_info >= (3, 10)


def test_package_importable() -> None:
    import serial_scale_hx711

    assert hasattr(serial_scale_hx711, "__version__")
