#pragma once

#include "state.h"

void initializeHardware();
void applyTeamColor();
void turnInternalLightOn();
void turnInternalLightOff();
void turnProgressGreen();
void turnProgressRed();
void turnProgressOff();
void turnAllLedsOff();
void restoreStageLeds();
void showCorrectAttempt(RgbColor color);
void showCorrectStage(uint8_t stage, RgbColor color);
void showIncorrectAttempt();

void openDoor();
void closeDoor();
void openLock();
void closeLock();
