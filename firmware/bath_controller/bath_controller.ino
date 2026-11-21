// bath_controller.ino - PID water bath controller, version 1
// Reads the probe, runs a controller and drives the heater with slow PWM (Pulse Width Modulation)
// all at once using millis() timing (no delay()).
// TEST SETUP: the "heater" is the onboard LED on pin 2. Once I test later this pin will change to the pin wired to the MOSFET pin

#include <OneWire.h>
#include <DallasTemperature.h>

// ---------- Pins ----------
const int ONE_WIRE_PIN = 4;    // probe data wire (D4)
const int HEATER_PIN   = 2;    // onboard LED for now

// ---------- Timing ----------
const unsigned long SAMPLE_MS = 1000;  // read temperature once per second
const unsigned long WINDOW_MS = 2000;  // slow PWM window: 2 seconds

// ---------- Safety ----------
const float MAX_TEMP_C   = 60.0;  // software cutoff (thermal fuse is at 73 C). Temp shouldn't even reach this high but just in case.
const float MAX_SETPOINT = 55.0;  // the 30 W heater can't go much higher anyway

// ---------- Control ----------
const float HYSTERESIS = 0.5;     // on/off control: switch at setpoint +/- 0.5 C

enum Mode { MODE_OFF, MODE_MANUAL, MODE_ONOFF };
Mode mode = MODE_OFF;

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

float tempC       = 0.0;   // latest temperature reading
float setpoint    = 40.0;  // target temperature (on/off mode)
float manualPower = 0.0;   // power chosen with the MAN command
float power       = 0.0;   // power actually sent to the heater, 0-100 %
bool  fault       = false; // true after a safety trip, until RESET

unsigned long lastSample  = 0;
unsigned long windowStart = 0;
String inputLine = "";

void setup() {
  Serial.begin(115200);
  pinMode(HEATER_PIN, OUTPUT);
  digitalWrite(HEATER_PIN, LOW);         // heater OFF at power-up, always

  sensors.begin();
  sensors.setResolution(12);             // 0.0625 C steps, ~750 ms per reading
  sensors.setWaitForConversion(false);   // don't freeze while the probe measures
  sensors.requestTemperatures();         // start the first measurement

  Serial.println("BATH CONTROLLER v1 STARTED");
  Serial.println("Commands: OFF | MAN <0-100> | ONOFF <setpoint> | RESET");
  Serial.println("DATA columns: time_s,temp_C,setpoint_C,power_pct,mode");

  lastSample  = millis();
  windowStart = millis();
}

void loop() {
  unsigned long now = millis();

  // Task 1: check for typed commands (every pass)
  readCommands();

  // Task 2: once per second - collect the reading, start the next one,
  // check safety, update the controller, print a data line
  if (now - lastSample >= SAMPLE_MS) {
    lastSample += SAMPLE_MS;
    tempC = sensors.getTempCByIndex(0);  // result of the measurement started 1 s ago
    sensors.requestTemperatures();       // start the next one (returns immediately)
    checkSafety();
    updateControl();
    printData(now);
  }

  // Task 3: slow PWM - switch the heater on/off (every pass)
  driveHeater(now);
}

// ---------- Safety ----------
void checkSafety() {
  if (tempC == DEVICE_DISCONNECTED_C || tempC == 85.0) {
    // -127 = probe not responding; 85.0 = the probe's power-on default value
    triggerFault("sensor error");
  } else if (tempC > MAX_TEMP_C) {
    triggerFault("over temperature");
  }
}

void triggerFault(const char* reason) {
  if (!fault) {
    Serial.print("FAULT: ");
    Serial.print(reason);
    Serial.println(" - heater off. Type RESET to clear.");
  }
  fault = true;
  mode  = MODE_OFF;
  power = 0;
}

// ---------- Controller ----------
void updateControl() {
  if (fault || mode == MODE_OFF) {
    power = 0;
  } else if (mode == MODE_MANUAL) {
    power = manualPower;
  } else if (mode == MODE_ONOFF) {
    if (tempC < setpoint - HYSTERESIS) {
      power = 100;                       // too cold: full on
    } else if (tempC > setpoint + HYSTERESIS) {
      power = 0;                         // too hot: off
    }
    // inside the band: keep the previous power (stops rapid flicking)
  }
}

// ---------- Slow PWM ----------
void driveHeater(unsigned long now) {
  if (now - windowStart >= WINDOW_MS) {
    windowStart += WINDOW_MS;            // start a new 2 s window
  }
  unsigned long onTime = (unsigned long)(power / 100.0 * WINDOW_MS);
  bool heaterOn = !fault && (now - windowStart < onTime);
  digitalWrite(HEATER_PIN, heaterOn ? HIGH : LOW);
}

// ---------- Output ----------
void printData(unsigned long now) {
  Serial.print("DATA,");
  Serial.print(now / 1000.0, 1);  Serial.print(",");
  Serial.print(tempC, 2);         Serial.print(",");
  Serial.print(setpoint, 1);      Serial.print(",");
  Serial.print(power, 1);         Serial.print(",");
  Serial.println(modeName());
}

const char* modeName() {
  if (fault) return "FAULT";
  if (mode == MODE_MANUAL) return "MAN";
  if (mode == MODE_ONOFF) return "ONOFF";
  return "OFF";
}

// ---------- Commands ----------
void readCommands() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (inputLine.length() > 0) handleCommand(inputLine);
      inputLine = "";
    } else {
      inputLine += c;
    }
  }
}

void handleCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();

  if (cmd == "OFF") {
    mode = MODE_OFF;
    Serial.println("OK: heater off");
  } else if (cmd == "RESET") {
    fault = false;
    mode  = MODE_OFF;
    power = 0;
    Serial.println("OK: fault cleared, heater off");
  } else if (fault) {
    Serial.println("Refused: there is a fault. Type RESET first.");
  } else if (cmd.startsWith("MAN ")) {
    manualPower = constrain(cmd.substring(4).toFloat(), 0, 100);
    mode = MODE_MANUAL;
    Serial.print("OK: manual power ");
    Serial.print(manualPower, 1);
    Serial.println(" %");
  } else if (cmd.startsWith("ONOFF ")) {
    float sp = cmd.substring(6).toFloat();
    if (sp < 15 || sp > MAX_SETPOINT) {
      Serial.println("Refused: setpoint must be between 15 and 55 C");
    } else {
      setpoint = sp;
      mode = MODE_ONOFF;
      // start from a fresh decision instead of whatever power the last mode left behind
      power = (tempC < setpoint) ? 100 : 0;
      Serial.print("OK: on/off control, setpoint ");
      Serial.print(setpoint, 1);
      Serial.println(" C");
    }
  } else {
    Serial.println("Unknown command. Use: OFF | MAN <0-100> | ONOFF <setpoint> | RESET");
  }
}
