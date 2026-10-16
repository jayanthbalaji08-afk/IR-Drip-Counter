/*
  Project: IR Drip Counter
  Controller: ESP32 DevKit
  Display: 0.96-inch SSD1306 I2C OLED

  Pins:
    IR Sensor : GPIO34
    Green LED : GPIO25
    Red LED   : GPIO26
    Buzzer    : GPIO27
    OLED SDA  : GPIO21
    OLED SCL  : GPIO22

  Educational prototype only.
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// Pin definitions
const int IR_SENSOR_PIN = 34;
const int GREEN_LED_PIN = 25;
const int RED_LED_PIN = 26;
const int BUZZER_PIN = 27;

// Adjust after testing your actual sensor
const int SENSOR_THRESHOLD = 2000;

// Detection timing
const unsigned long DEBOUNCE_MS = 100;
const unsigned long RATE_WINDOW_MS = 60000;
const unsigned long ALARM_TIMEOUT_MS = 3000;

// Drop counter
unsigned long totalDrops = 0;
unsigned long windowDrops = 0;

unsigned long lastDetectionTime = 0;
unsigned long lastDropTime = 0;
unsigned long windowStartTime = 0;

float dropsPerMinute = 0.0;

bool alarmActive = false;

// Sensor state
bool previousSensorState = false;

// Read sensor and determine whether the beam is interrupted.
// The threshold and polarity must be calibrated for your hardware.
bool readDropSensor() {
  int sensorValue = analogRead(IR_SENSOR_PIN);

  // In the reference circuit, less IR light should produce
  // a higher voltage at the phototransistor signal node.
  return sensorValue > SENSOR_THRESHOLD;
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("IR DRIP COUNTER");

  display.drawLine(0, 12, 127, 12, SSD1306_WHITE);

  display.setCursor(0, 19);
  display.print("Total drops: ");
  display.println(totalDrops);

  display.setCursor(0, 32);
  display.print("Rate: ");
  display.print(dropsPerMinute, 1);
  display.println(" drops/min");

  display.setCursor(0, 46);

  if (alarmActive) {
    display.println("STATUS: DROP DETECTED");
  } else {
    display.println("STATUS: MONITORING");
  }

  display.display();
}

void setup() {
  Serial.begin(115200);

  pinMode(IR_SENSOR_PIN, INPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  analogReadResolution(12);

  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED initialization failed!");
    while (true) {
      delay(1000);
    }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("IR Drip Counter");
  display.println("Initializing...");
  display.display();

  delay(1000);

  windowStartTime = millis();

  previousSensorState = readDropSensor();

  updateDisplay();

  Serial.println("IR Drip Counter started.");
}

void loop() {
  unsigned long now = millis();

  bool sensorState = readDropSensor();

  // Detect the beginning of a sensor interruption.
  if (sensorState &&
      !previousSensorState &&
      now - lastDetectionTime >= DEBOUNCE_MS) {

    lastDetectionTime = now;
    lastDropTime = now;

    totalDrops++;
    windowDrops++;

    alarmActive = true;

    Serial.print("Drop detected. Total: ");
    Serial.println(totalDrops);
  }

  previousSensorState = sensorState;

  // Calculate drops per minute using a 60-second window.
  if (now - windowStartTime >= RATE_WINDOW_MS) {
    dropsPerMinute =
      windowDrops * 60000.0 /
      (now - windowStartTime);

    windowDrops = 0;
    windowStartTime = now;
  }

  // Turn off the temporary alarm after the timeout.
  if (alarmActive &&
      now - lastDropTime >= ALARM_TIMEOUT_MS) {
    alarmActive = false;
  }

  // LED indication.
  if (alarmActive) {
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(GREEN_LED_PIN, HIGH);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }

  // Refresh display periodically.
  static unsigned long lastDisplayUpdate = 0;

  if (now - lastDisplayUpdate >= 250) {
    lastDisplayUpdate = now;
    updateDisplay();
  }

  delay(5);
}
