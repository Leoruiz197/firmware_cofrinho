#include <Arduino.h>

#include "hardware.h"
#include "websocket_service.h"
#include "settings.h"

void setup() {
  Serial.begin(115200);
  delay(300);

  loadSettings();
  initializeHardware();
  configureWifi();
  initializeWebSocket();
}

void loop() {
  runWebSocket();
}
