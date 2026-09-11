#include <Preferences.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include "config.h"
#include "settings.h"
#include "state.h"

namespace {
Preferences preferences;
char backendPortValue[6];

int readColor(const char* key, int fallback) {
  return constrain(preferences.getInt(key, fallback), 0, 255);
}

String normalizeBackendHost(String host) {
  host.trim();
  if (host.startsWith("https://")) host.remove(0, 8);
  if (host.startsWith("http://")) host.remove(0, 7);
  const int pathStart = host.indexOf('/');
  if (pathStart >= 0) host.remove(pathStart);
  return host;
}
}

void loadSettings() {
  preferences.begin("cofrinho", true);
  String backendHost = preferences.getString("backendHost", DEFAULT_BACKEND_HOST);
  deviceConfig.backendPort = constrain(preferences.getUInt("backendPort", DEFAULT_BACKEND_PORT), 1, 65535);
  String deviceToken = preferences.getString("deviceToken", DEFAULT_DEVICE_TOKEN);
  String deviceId = preferences.getString("deviceId", DEFAULT_DEVICE_ID);
  if (deviceToken.isEmpty()) deviceToken = DEFAULT_DEVICE_TOKEN;
  if (deviceId.isEmpty()) deviceId = DEFAULT_DEVICE_ID;
  backendHost.toCharArray(deviceConfig.backendHost, sizeof(deviceConfig.backendHost));
  deviceToken.toCharArray(deviceConfig.deviceToken, sizeof(deviceConfig.deviceToken));
  deviceId.toCharArray(deviceConfig.deviceId, sizeof(deviceConfig.deviceId));
<<<<<<< HEAD
  deviceConfig.backendPort = constrain(preferences.getUInt("backendPort", DEFAULT_BACKEND_PORT), 1, 65535);
=======
  deviceConfig.doorOpenAngle = constrain(preferences.getInt("doorOpen", DOOR_OPEN_ANGLE), 0, 180);
>>>>>>> 0061d9280d5d7fc53cc2d63004cb959bdc004a27
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
  preferences.putUInt("backendPort", deviceConfig.backendPort);
  preferences.putString("deviceToken", deviceConfig.deviceToken);
  preferences.putString("deviceId", deviceConfig.deviceId);
<<<<<<< HEAD
  preferences.putUInt("backendPort", deviceConfig.backendPort);
=======
  preferences.putInt("doorOpen", deviceConfig.doorOpenAngle);
>>>>>>> 0061d9280d5d7fc53cc2d63004cb959bdc004a27
  preferences.putInt("doorClose", deviceConfig.doorCloseAngle);
  preferences.putUInt("passwords", deviceConfig.passwordCount);
  preferences.putInt("colorR", deviceConfig.teamColor.red);
  preferences.putInt("colorG", deviceConfig.teamColor.green);
  preferences.putInt("colorB", deviceConfig.teamColor.blue);
  preferences.end();
}

void configureWifi() {
  WiFiManager wifiManager;
<<<<<<< HEAD
  char backendPort[6];
  snprintf(backendPort, sizeof(backendPort), "%u", deviceConfig.backendPort);
  WiFiManagerParameter backendHostParameter("backendHost", "Backend host/IP", deviceConfig.backendHost, sizeof(deviceConfig.backendHost));
  WiFiManagerParameter backendPortParameter("backendPort", "Porta backend", backendPort, sizeof(backendPort));
=======
  snprintf(backendPortValue, sizeof(backendPortValue), "%u", deviceConfig.backendPort);
  WiFiManagerParameter backendHostParameter("backendHost", "Backend host/IP", deviceConfig.backendHost, sizeof(deviceConfig.backendHost));
  WiFiManagerParameter backendPortParameter("backendPort", "Backend port", backendPortValue, sizeof(backendPortValue));
>>>>>>> 0061d9280d5d7fc53cc2d63004cb959bdc004a27
  WiFiManagerParameter deviceTokenParameter("deviceToken", "Device token", deviceConfig.deviceToken, sizeof(deviceConfig.deviceToken));
  WiFiManagerParameter deviceParameter("deviceId", "Identificador do cofrinho", deviceConfig.deviceId, sizeof(deviceConfig.deviceId));

  wifiManager.addParameter(&backendHostParameter);
  wifiManager.addParameter(&backendPortParameter);
  wifiManager.addParameter(&deviceTokenParameter);
  wifiManager.addParameter(&deviceParameter);
  wifiManager.setConfigPortalTimeout(180);

  // Mantem DHCP para IP/gateway e substitui DNS instavel de alguns hotspots.
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, IPAddress(1, 1, 1, 1), IPAddress(8, 8, 8, 8));
  Serial.println("[WiFi] DNS configurado: 1.1.1.1 e 8.8.8.8");

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
        preferences.remove("backendPort");
        preferences.remove("deviceToken");
        preferences.remove("deviceId");
        preferences.end();

        strlcpy(deviceConfig.backendHost, DEFAULT_BACKEND_HOST, sizeof(deviceConfig.backendHost));
        deviceConfig.backendPort = DEFAULT_BACKEND_PORT;
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

<<<<<<< HEAD
  // WiFiManager/DHCP pode substituir o DNS definido antes da conexao.
  if (!WiFi.config(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(), IPAddress(1, 1, 1, 1), IPAddress(8, 8, 8, 8))) {
    Serial.println("[WiFi] Nao foi possivel reaplicar o DNS publico.");
  }
  const unsigned long dnsStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - dnsStart < 5000) {
    delay(50);
  }
  Serial.printf("[WiFi] DNS ativo: %s e %s\n", WiFi.dnsIP(0).toString().c_str(), WiFi.dnsIP(1).toString().c_str());

  const String host = normalizeBackendHost(backendHostParameter.getValue());
  host.toCharArray(deviceConfig.backendHost, sizeof(deviceConfig.backendHost));
  deviceConfig.backendPort = constrain(String(backendPortParameter.getValue()).toInt(), 1, 65535);
=======
  strlcpy(deviceConfig.backendHost, backendHostParameter.getValue(), sizeof(deviceConfig.backendHost));
  char* portSeparator = strchr(deviceConfig.backendHost, ':');
  if (portSeparator != nullptr) {
    *portSeparator = '\0';
  }
  deviceConfig.backendPort = constrain(atoi(backendPortParameter.getValue()), 1, 65535);
>>>>>>> 0061d9280d5d7fc53cc2d63004cb959bdc004a27
  strlcpy(deviceConfig.deviceToken, deviceTokenParameter.getValue(), sizeof(deviceConfig.deviceToken));
  strlcpy(deviceConfig.deviceId, deviceParameter.getValue(), sizeof(deviceConfig.deviceId));
  saveSettings();
}
