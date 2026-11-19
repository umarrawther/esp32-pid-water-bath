const int LED_PIN = 2;
unsigned long lastBlink = 0;
unsigned long lastPrint = 0;
bool ledOn = false;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  Serial.println("PART A VERSION 2 STARTED");
}

void loop() {
  unsigned long now = millis();

  // Task 1: toggle the LED every 1000 ms
  if (now - lastBlink >= 1000) {
    lastBlink = now;
    ledOn = !ledOn;
    if (ledOn) {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("LED ON");
    } else {
      digitalWrite(LED_PIN, LOW);
      Serial.println("LED OFF");
    }
  }

  // Task 2: print the time every 300 ms
  if (now - lastPrint >= 300) {
    lastPrint = now;
    Serial.println(now);
  }
} 