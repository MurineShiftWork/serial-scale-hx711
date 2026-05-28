import logging
import statistics
import time
from collections.abc import Callable

from serial_scale_hx711.connection import SerialConnection


class Scale(SerialConnection):
    _identity_response = "<SerialWeighingScale>"

    def __init__(self, serial_port: str, baudrate: int = 115200, timeout: float = 1) -> None:
        # init serial connection
        super().__init__(serial_port=serial_port, baudrate=baudrate, timeout=timeout)

    def start(self, timeout=10) -> None:
        """Connect and wait for the scale firmware to finish initialising.

        Firmware only responds to <i> (identify) once the HX711 tare is complete,
        so polling identify() is sufficient — no need to hammer read_weight().
        """
        self.connect()

        start_time = time.time()
        while time.time() - start_time < timeout:
            try:
                if self.identify():
                    elapsed = time.time() - start_time
                    logging.info(f"Scale ready: {self.serial_port} (after {elapsed:.2f}s)")
                    return
            except (UnicodeDecodeError, Exception):
                pass
            time.sleep(0.1)

        raise TimeoutError(
            f"Scale did not respond within {timeout}s on {self.serial_port}. "
            f"Check wiring and that firmware is loaded."
        )

    @property
    def is_ready(self) -> bool:
        """True if the firmware is initialised and responding to identity queries."""
        return self.identify()

    def read_weight(self) -> float | None:
        """
        Get the weight from the scale.
        """
        self.send(command="w", order="c")
        weight_result = self.read_line()

        # Convert to float
        try:
            weight = round(float(weight_result), 2)
            return weight
        except ValueError:
            logging.error(f"Failed to convert weight result to float: {weight_result}")
            return None

    def tare(self) -> None:
        """
        Tare the scale.
        """
        self.send(command="t", order="c")
        # LoadCell.tare() on firmware is blocking (~100 ms at 10 Hz / 1 sample).
        # The rolling-average buffer also needs time to flush old pre-tare values.
        # Wait 0.5 s so the next read sees fully tared samples.
        time.sleep(0.5)

    def identify(self) -> bool:
        """
        Identify the device connected to the serial port.
        This method sends a command to the device and waits for a response.
        """
        self.send(command="i", order="c")
        response = self.read_line()
        return response == self._identity_response

    def get_calibration_factor(self) -> float | None:
        """
        Get the calibration factor from the scale.
        """
        self.send(command="f", order="c")

        calibration_result = self.read_line()

        # Convert to float
        try:
            calibration_factor = float(calibration_result)
            return calibration_factor
        except ValueError:
            logging.error(f"Failed to convert calibration result to float: {calibration_result}")
            return None

    def read_weight_repeated(self, n_readings: int = 5, inter_read_delay: float = 0.1) -> list:
        """Read weight n times, return list of valid (non-None) readings."""
        readings = []
        for _ in range(n_readings):
            reading = self.read_weight()
            if reading is not None:
                readings.append(reading)
            time.sleep(inter_read_delay)
        return readings

    def read_weight_reliable(
        self,
        n_readings: int = 5,
        inter_read_delay: float = 0.1,
        measure: Callable = statistics.median,
    ) -> float:
        """Repeated reads with statistical measure. Raises if no valid readings."""
        readings = self.read_weight_repeated(
            n_readings=n_readings, inter_read_delay=inter_read_delay
        )
        if not readings:
            raise RuntimeError(
                f"Scale on {self.serial_port} returned no valid readings "
                f"after {n_readings} attempts."
            )
        return measure(readings)

    def read_weight_blocking(
        self,
        n_valid: int = 3,
        inter_read_delay: float = 0.2,
        timeout: float = 30,
    ) -> float:
        """Block until n_valid successful weight readings are collected, return their median.

        Use this at every point in the calibration where the task must not proceed
        until the scale has returned a trustworthy value.  Raises TimeoutError if
        the scale does not produce enough valid readings within `timeout` seconds.
        """
        readings = []
        deadline = time.time() + timeout
        while time.time() < deadline:
            reading = self.read_weight()
            if reading is not None:
                readings.append(reading)
                if len(readings) >= n_valid:
                    return statistics.median(readings)
            time.sleep(inter_read_delay)
        raise TimeoutError(
            f"Scale on {self.serial_port} could not produce {n_valid} valid readings "
            f"within {timeout}s.  Check connection and sensor."
        )


if __name__ == "__main__":
    print("TEST")

    s = Scale(serial_port="/dev/ttyACM1", baudrate=115200, timeout=1)
    s.connect()
    s.read_weight()
