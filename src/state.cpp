#include "state.h"

DeviceConfig deviceConfig = {
    "",
    "cofrinho-device-token",
    "cofre01",
    60,
    1,
    {255, 255, 255},
};

int lockServoPosition = LOCK_CLOSED_ANGLE;
int doorServoPosition = 60;
uint8_t currentStage = 0;
RgbColor stageColors[MAX_STAGES];

void resetStages() {
  currentStage = 0;
  for (uint8_t stage = 0; stage < MAX_STAGES; ++stage) {
    stageColors[stage] = deviceConfig.teamColor;
  }
}
