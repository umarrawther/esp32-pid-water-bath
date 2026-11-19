#include <OneWire.h>
#include <DallasTemperature.h>

const int ONE_WIRE_PIN = 4;            // probe data goes to D4
OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);
  sensors.begin();
  Serial.print("Sensors found: ");
  Serial.println(sensors.getDeviceCount());
  sensors.setResolution(12);           // 0.0625 °C steps
}

void loop() {
  sensors.requestTemperatures();       // probe measures (~0.75 s)
  float tempC = sensors.getTempCByIndex(0);

  if (tempC == DEVICE_DISCONNECTED_C) {
    Serial.println("Error: probe not found");
  } else {
    Serial.print(millis() / 1000.0, 1);
    Serial.print(" s, ");
    Serial.print(tempC, 2);
    Serial.println(" C");
  }
  delay(250);
}