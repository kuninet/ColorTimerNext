#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include <Arduino.h>

// GPIO Definitions (based on target design)
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define LED_RED_PIN 25
#define LED_BLUE_PIN 26
#define BUZZER_PIN 27
#define BUTTON_PIN 33

// PWM Configuration for Buzzer
#define BUZZER_CHANNEL 0
#define BUZZER_FREQ 2000
#define BUZZER_RES 8

void initHardware();

// OLED Display Functions
void displayStatus(const char *statusStr, const char *detailStr = "");
void displayError(const char *errorStr);

// LED Functions
enum LedColor { LED_OFF, LED_RED, LED_BLUE, LED_BLUE_BLINK, LED_RED_BLINK };
void setLedState(LedColor color);
void updateLedBlink(); // Call this in loop()

// Buzzer Functions
void playBeep(int durationMs);
void playErrorSound();
void playColorTimerSound(); // タイムアウト用カラータイマー音

#endif
