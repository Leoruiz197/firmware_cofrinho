#include "state.h"

DeviceConfig deviceConfig = {
    "",
    DEFAULT_BACKEND_PORT,
    "cofrinho-device-token",
    "cofre01",
<<<<<<< HEAD
    DEFAULT_BACKEND_PORT,
=======
    DOOR_OPEN_ANGLE,
>>>>>>> 0061d9280d5d7fc53cc2d63004cb959bdc004a27
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
