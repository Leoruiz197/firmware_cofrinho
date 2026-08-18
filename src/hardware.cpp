#include <Adafruit_NeoPixel.h>
#include <ESP32Servo.h>

#include "config.h"
#include "hardware.h"

namespace {
Servo lockServo;
Servo doorServo;
Adafruit_NeoPixel leds(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void moveServoSmoothly(Servo& servo, int& currentPosition, int targetPosition, uint8_t stepDelayMs) {
  targetPosition = constrain(targetPosition, 0, 180);
  const int step = currentPosition <= targetPosition ? 1 : -1;

  while (currentPosition != targetPosition) {
    currentPosition += step;
    servo.write(currentPosition);
    delay(stepDelayMs);
  }
}

void setProgressColor(RgbColor color) {
  for (uint8_t index = PROGRESS_LED_START; index <= PROGRESS_LED_END; ++index) {
    leds.setPixelColor(index, leds.Color(color.red, color.green, color.blue));
  }
  leds.show();
}
}

void initializeHardware() {
  lockServo.setPeriodHertz(50);
  doorServo.setPeriodHertz(50);
  lockServo.attach(LOCK_SERVO_PIN, 500, 2400);
  doorServo.attach(DOOR_SERVO_PIN, 500, 2400);
  lockServo.write(lockServoPosition);
  doorServo.write(doorServoPosition);

  leds.begin();
  leds.clear();
  leds.show();
  applyTeamColor();
}

void applyTeamColor() {
  restoreStageLeds();
}

void turnInternalLightOn() {
  for (uint8_t index = INTERNAL_LED_START; index <= INTERNAL_LED_END; ++index) {
    leds.setPixelColor(index, leds.Color(255, 255, 255));
  }
  leds.show();
}

void turnInternalLightOff() {
  for (uint8_t index = INTERNAL_LED_START; index <= INTERNAL_LED_END; ++index) {
    leds.setPixelColor(index, 0);
  }
  leds.show();
}

void turnProgressGreen() { setProgressColor({0, 255, 0}); }
void turnProgressRed() { setProgressColor({255, 0, 0}); }
void turnProgressOff() { setProgressColor({0, 0, 0}); }

void turnAllLedsOff() {
  leds.clear();
  leds.show();
}

void restoreStageLeds() {
  const uint8_t progressLeds = PROGRESS_LED_END - PROGRESS_LED_START + 1;
  const uint8_t ledsPerStage = progressLeds / deviceConfig.passwordCount;

  for (uint8_t index = 0; index < progressLeds; ++index) {
    uint8_t stage = index / ledsPerStage;
    if (stage >= deviceConfig.passwordCount) {
      stage = deviceConfig.passwordCount - 1;
    }
    const RgbColor color = stage < currentStage ? stageColors[stage] : deviceConfig.teamColor;
    leds.setPixelColor(PROGRESS_LED_START + index, leds.Color(color.red, color.green, color.blue));
  }
  leds.show();
}

void showCorrectAttempt(RgbColor color) {
  for (uint8_t attempt = 0; attempt < 4; ++attempt) {
    turnProgressGreen();
    delay(200);
    turnProgressOff();
    delay(150);
  }
  openLock();
  turnInternalLightOn();
  openDoor();
  setProgressColor(color);
}

void showCorrectStage(uint8_t stage, RgbColor color) {
  if (stage == 0 || stage > deviceConfig.passwordCount) {
    return;
  }
  stageColors[stage - 1] = color;
  if (stage > currentStage) {
    currentStage = stage;
  }
  restoreStageLeds();
}

void showIncorrectAttempt() {
  for (uint8_t attempt = 0; attempt < 2; ++attempt) {
    turnProgressRed();
    openLock();
    delay(150);
    turnProgressOff();
    closeLock();
    delay(150);
  }
  restoreStageLeds();
}

void openDoor() { moveServoSmoothly(doorServo, doorServoPosition, DOOR_OPEN_ANGLE, 10); }
void closeDoor() { moveServoSmoothly(doorServo, doorServoPosition, deviceConfig.doorCloseAngle, 10); }
void openLock() { moveServoSmoothly(lockServo, lockServoPosition, LOCK_OPEN_ANGLE, 3); }
void closeLock() { moveServoSmoothly(lockServo, lockServoPosition, LOCK_CLOSED_ANGLE, 3); }
