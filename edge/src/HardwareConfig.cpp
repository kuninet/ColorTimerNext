#include "HardwareConfig.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// State internally used for blinking led
static LedColor currentLedState = LED_OFF;
static unsigned long lastBlinkTime = 0;
static bool ledToggleState = false;

void initHardware() {
  // Initialize I2C for OLED
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    // Try to proceed anyway
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Initializing...");
    display.display();
  }

  // Initialize LEDs
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_BLUE_PIN, LOW);

  // Initialize Buzzer via PWM (ledc mechanism in ESP32)
  ledcSetup(BUZZER_CHANNEL, BUZZER_FREQ, BUZZER_RES);
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
  ledcWrite(BUZZER_CHANNEL, 0); // Keep quiet initially
}

void displayStatus(const char *statusStr, const char *detailStr) {
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 5);
  display.println(statusStr);

  if (detailStr && strlen(detailStr) > 0) {
    display.setTextSize(1);
    display.setCursor(0, 30);
    display.println(detailStr);
  }

  display.display();
}

void displayError(const char *errorStr) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("--- ERROR ---");
  display.setCursor(0, 20);
  display.println(errorStr);
  display.display();
}

void setLedState(LedColor color) {
  currentLedState = color;
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_BLUE_PIN, LOW);

  switch (color) {
  case LED_RED:
    digitalWrite(LED_RED_PIN, HIGH);
    break;
  case LED_BLUE:
    digitalWrite(LED_BLUE_PIN, HIGH);
    break;
  case LED_BLUE_BLINK:
    // Initial state is on, toggle logic handled in updateLedBlink()
    digitalWrite(LED_BLUE_PIN, HIGH);
    ledToggleState = true;
    lastBlinkTime = millis();
    break;
  default:
    break;
  }
}

void updateLedBlink() {
  if (currentLedState == LED_BLUE_BLINK) {
    if (millis() - lastBlinkTime >= 500) { // Toggle every 500ms
      lastBlinkTime = millis();
      ledToggleState = !ledToggleState;
      digitalWrite(LED_BLUE_PIN, ledToggleState ? HIGH : LOW);
    }
  }
}

void playBeep(int durationMs) {
  ledcWrite(BUZZER_CHANNEL, 128); // 50% duty cycle
  delay(durationMs);
  ledcWrite(BUZZER_CHANNEL, 0);
}

void playErrorSound() {
  for (int i = 0; i < 3; i++) {
    playBeep(100);
    delay(100);
  }
}
