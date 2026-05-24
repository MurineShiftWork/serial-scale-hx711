/*
  Firmware version: "2.1.0"
  -------------------------------------------------------------------------------------
  Adapted example from HX711 library for `serial-weighing-scale`. Lars Rollik, nov2021.
  github.com/larsrollik/serial-weighing-scale
  -------------------------------------------------------------------------------------
  HX711_ADC
  Arduino library for HX711 24-Bit Analog-to-Digital Converter for Weight Scales
  Olav Kallhovd sept2017
  -------------------------------------------------------------------------------------
  Startup behaviour:
    - HX711 init begins immediately when Arduino powers on (non-blocking).
    - setup() returns in microseconds; the Timer1 ISR drives LoadCell.update().
    - Init completes within ~500 ms; the scale then processes commands normally.
    - Commands received before init is done are silently discarded (serial buffer
      is cleared on first command check after init).
    - Python must NOT toggle DTR on port open (dsrdtr=False in pyserial) to avoid
      resetting the Arduino on connect.  If a reset does occur the init re-runs and
      completes in <500 ms — Python polls <i> until it gets a valid identity response.
*/

#include <HX711_ADC.h>

// DEFINITIONS
#define CMD Serial
#define HX711_DOUT 2      // DATA
#define HX711_SCK 3       // CLOCK
#define SAMPLES_IN_USE 1  // 1 = fastest; increase for less noise (slower)
// #define CALIBRATION_FACTOR -3150  // scale 1
#define CALIBRATION_FACTOR -2630  // scale 2
#define SCALING_FACTOR 1.0
#define STABILIZING_TIME 500  // ms — enough for HX711 to settle after power-on
#define PERFORM_TARE true
#define DEBUG_PRINT false
#define ID_STRING "<SerialWeighingScale>"
#define BUFFER_SIZE 10

// INSTANCE of LoadCell object
HX711_ADC LoadCell(HX711_DOUT, HX711_SCK);

float buffer[BUFFER_SIZE];
uint8_t bufIndex = 0;
bool bufFilled = false;

// Set true by Timer1 ISR every 100 ms
volatile bool sampleFlag = false;

// Set true once HX711 tare completes; commands are gated on this flag
bool initDone = false;


void setup() {
  CMD.begin(115200);

  LoadCell.begin();
  LoadCell.setSamplesInUse(SAMPLES_IN_USE);
  LoadCell.setGain(128);

  for (int i = 0; i < BUFFER_SIZE; i++) buffer[i] = 0;

  // Timer1: fires every 100 ms (10 Hz), drives LoadCell.update() in loop()
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A  = 24999;                              // 16 MHz / (64 * 10 Hz) - 1
  TCCR1B |= (1 << WGM12);                     // CTC mode
  TCCR1B |= (1 << CS11) | (1 << CS10);        // prescaler 64
  TIMSK1 |= (1 << OCIE1A);                    // enable compare interrupt
  interrupts();

  // Stabilise then kick off async tare; loop() polls getTareStatus() via Timer1 ISR
  delay(STABILIZING_TIME);
  LoadCell.tareNoDelay();
}  // setup


void loop() {
  // --- Phase 1: wait for HX711 init to complete ---
  if (!initDone) {
    if (sampleFlag) {
      sampleFlag = false;
      LoadCell.update();
    }
    if (LoadCell.getTareStatus()) {
      if (LoadCell.getTareTimeoutFlag()) {
        logMessage("Timeout — check MCU>HX711 wiring and pin designations");
        while (1);
      }
      LoadCell.setCalFactor(CALIBRATION_FACTOR);
      initDone = true;
      // Discard anything Python may have sent during init
      while (CMD.available()) CMD.read();
      if (DEBUG_PRINT) logMessage("ready");
    }
    return;
  }

  // --- Phase 2: normal operation ---

  if (sampleFlag) {
    sampleFlag = false;
    if (LoadCell.update()) {
      addToBuffer(LoadCell.getData());
    }
  }

  static byte startByte = '<';
  static byte stopByte  = '>';
  static byte commandChar;

  static bool receiving = false;
  static byte rxBuf[256];
  static int  rxIdx = 0;

  while (CMD.available()) {
    byte b = CMD.read();

    if (b == startByte) {
      receiving = true;
      rxIdx = 0;
      continue;
    }

    if (b == stopByte) {
      receiving = false;
      commandChar = rxBuf[0];

      switch (commandChar) {
        case 'i':
          identifyMyself();
          break;

        case 'w':
          if (DEBUG_PRINT) logMessage("reading weight");
          CMD.println(String(getRollingAverage(2)));
          break;

        case 't':
          tare_scale();
          break;

        case 'c':
          calibrate();
          break;

        case 'f':
          CMD.println(String(CALIBRATION_FACTOR));
          break;

        default:
          if (DEBUG_PRINT) logMessage("Invalid command");
          break;
      }
    }

    if (receiving && rxIdx < (int)sizeof(rxBuf)) {
      rxBuf[rxIdx++] = b;
    }
  }
}  // loop


// Timer1 ISR — sets flag every 100 ms; actual update() call is in loop()
ISR(TIMER1_COMPA_vect) {
  sampleFlag = true;
}


void addToBuffer(float val) {
  buffer[bufIndex++] = val;
  if (bufIndex >= BUFFER_SIZE) {
    bufIndex = 0;
    bufFilled = true;
  }
}

float getRollingAverage(int precision) {
  uint8_t size = bufFilled ? BUFFER_SIZE : bufIndex;
  if (size == 0) return 0.0;

  float sum = 0;
  for (uint8_t i = 0; i < size; i++) sum += buffer[i];
  float avg = sum / size;

  float factor = pow(10, precision);
  return round(avg * factor) / factor;
}

void identifyMyself() {
  CMD.println(ID_STRING);
}

void logMessage(const String& msg) {
  CMD.print("LOG: ");
  CMD.println(msg);
}

void sendFloatAsBytes(float value) {
  union { float f; byte b[4]; } data;
  data.f = value;
  for (int i = 0; i < 4; i++) CMD.write(data.b[i]);
}

void readWeight() {
  if (DEBUG_PRINT) logMessage("fct: readWeight");
  LoadCell.update();
  float i = LoadCell.getData() / SCALING_FACTOR;
  CMD.println(String(i));
}

int tare_scale() {
  LoadCell.tare();
}

void calibrate() {
  LoadCell.setCalFactor(1.0);
  tare_scale();

  logMessage("Send known mass as float, then 'a' to confirm weight placed.");
  while (!CMD.available()) delay(100);

  float known_mass = CMD.parseFloat();
  logMessage("Known mass: " + String(known_mass));
  logMessage("Place weight on scale, send 'a' to calibrate.");

  boolean weight_added = false;
  while (!weight_added) {
    if (CMD.available()) {
      char cmdChar = CMD.read();
      LoadCell.update();
      if (cmdChar == 'a') {
        LoadCell.refreshDataSet();
        float new_calibration_factor = LoadCell.getNewCalibration(known_mass);
        logMessage("New calibration factor: " + String(new_calibration_factor));
        weight_added = true;
      }
    }
    delay(10);
  }
}
