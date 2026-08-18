#include <Preferences.h>
#include <WiFiManager.h>

#include "config.h"
#include "settings.h"
#include "state.h"

namespace {
Preferences preferences;

int readColor(const char* key, int fallback) {
  return constrain(preferences.getInt(key, fallback), 0, 255);
}
}

void loadSettings() {
  preferences.begin("cofrinho", true);
  String backendHost = preferences.getString("backendHost", DEFAULT_BACKEND_HOST);
  String deviceToken = preferences.getString("deviceToken", DEFAULT_DEVICE_TOKEN);
  String deviceId = preferences.getString("deviceId", DEFAULT_DEVICE_ID);
  if (deviceToken.isEmpty()) deviceToken = DEFAULT_DEVICE_TOKEN;
  if (deviceId.isEmpty()) deviceId = DEFAULT_DEVICE_ID;
  backendHost.toCharArray(deviceConfig.backendHost, sizeof(deviceConfig.backendHost));
  deviceToken.toCharArray(deviceConfig.deviceToken, sizeof(deviceConfig.deviceToken));
  deviceId.toCharArray(deviceConfig.deviceId, sizeof(deviceConfig.deviceId));
  deviceConfig.doorCloseAngle = constrain(preferences.getInt("doorClose", 60), 0, 180);
  deviceConfig.passwordCount = constrain(preferences.getUInt("passwords", 1), 1, MAX_STAGES);
  deviceConfig.teamColor = {
      static_cast<uint8_t>(readColor("colorR", 255)),
      static_cast<uint8_t>(readColor("colorG", 255)),
      static_cast<uint8_t>(readColor("colorB", 255)),
  };
  preferences.end();

  doorServoPosition = deviceConfig.doorCloseAngle;
  resetStages();
}

void saveSettings() {
  preferences.begin("cofrinho", false);
  preferences.putString("backendHost", deviceConfig.backendHost);
  preferences.putString("deviceToken", deviceConfig.deviceToken);
  preferences.putString("deviceId", deviceConfig.deviceId);
  preferences.putInt("doorClose", deviceConfig.doorCloseAngle);
  preferences.putUInt("passwords", deviceConfig.passwordCount);
  preferences.putInt("colorR", deviceConfig.teamColor.red);
  preferences.putInt("colorG", deviceConfig.teamColor.green);
  preferences.putInt("colorB", deviceConfig.teamColor.blue);
  preferences.end();
}

void configureWifi() {
  WiFiManager wifiManager;
  WiFiManagerParameter backendHostParameter("backendHost", "Backend host/IP", deviceConfig.backendHost, sizeof(deviceConfig.backendHost));
  WiFiManagerParameter deviceTokenParameter("deviceToken", "Device token", deviceConfig.deviceToken, sizeof(deviceConfig.deviceToken));
  WiFiManagerParameter deviceParameter("deviceId", "Identificador do cofrinho", deviceConfig.deviceId, sizeof(deviceConfig.deviceId));

  wifiManager.addParameter(&backendHostParameter);
  wifiManager.addParameter(&deviceTokenParameter);
  wifiManager.addParameter(&deviceParameter);
  wifiManager.setConfigPortalTimeout(180);

  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  bool forceConfigPortal = false;

  if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
    const unsigned long pressStart = millis();
    Serial.println("[WiFi] BOOT pressionado. Segure por 3 segundos para resetar a rede.");

    while (digitalRead(BOOT_BUTTON_PIN) == LOW) {
      if (millis() - pressStart >= WIFI_RESET_HOLD_MS) {
        Serial.println("[WiFi] Reset de rede confirmado. Abrindo portal de configuracao.");
        wifiManager.resetSettings();

        // Mantem os ajustes mecanicos do cofrinho e limpa somente a conectividade.
        preferences.begin("cofrinho", false);
        preferences.remove("backendHost");
        preferences.remove("deviceToken");
        preferences.remove("deviceId");
        preferences.end();

        strlcpy(deviceConfig.backendHost, DEFAULT_BACKEND_HOST, sizeof(deviceConfig.backendHost));
        strlcpy(deviceConfig.deviceToken, DEFAULT_DEVICE_TOKEN, sizeof(deviceConfig.deviceToken));
        strlcpy(deviceConfig.deviceId, DEFAULT_DEVICE_ID, sizeof(deviceConfig.deviceId));
        forceConfigPortal = true;
        break;
      }
      delay(10);
    }
  }

  const bool connected = forceConfigPortal
      ? wifiManager.startConfigPortal("Cofrinho_Config")
      : wifiManager.autoConnect("Cofrinho_Config");

  if (!connected) {
    Serial.println("[WiFi] Configuracao nao concluida. Reiniciando...");
    delay(2000);
    ESP.restart();
  }

  strlcpy(deviceConfig.backendHost, backendHostParameter.getValue(), sizeof(deviceConfig.backendHost));
  strlcpy(deviceConfig.deviceToken, deviceTokenParameter.getValue(), sizeof(deviceConfig.deviceToken));
  strlcpy(deviceConfig.deviceId, deviceParameter.getValue(), sizeof(deviceConfig.deviceId));
  saveSettings();
}
