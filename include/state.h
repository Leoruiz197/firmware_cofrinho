#pragma once

#include <Arduino.h>
#include "config.h"

struct RgbColor {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

struct DeviceConfig {
  char backendHost[64];
  uint16_t backendPort;
  char deviceToken[96];
  char deviceId[32];
<<<<<<< HEAD
  uint16_t backendPort;
=======
  int doorOpenAngle;
>>>>>>> 0061d9280d5d7fc53cc2d63004cb959bdc004a27
  int doorCloseAngle;
  uint8_t passwordCount;
  RgbColor teamColor;
};

extern DeviceConfig deviceConfig;
extern int lockServoPosition;
extern int doorServoPosition;
extern uint8_t currentStage;
extern RgbColor stageColors[MAX_STAGES];

void resetStages();
