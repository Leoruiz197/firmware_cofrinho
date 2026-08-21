#include <ArduinoJson.h>

#include "command_handler.h"
#include "hardware.h"
#include "websocket_service.h"
#include "settings.h"
#include "state.h"

namespace {
bool internalLightEnabled = false;

RgbColor readColor(JsonVariantConst value, RgbColor fallback) {
  if (!value.is<JsonObjectConst>()) {
    return fallback;
  }
  JsonObjectConst color = value.as<JsonObjectConst>();
  return {
      static_cast<uint8_t>(constrain(color["R"] | fallback.red, 0, 255)),
      static_cast<uint8_t>(constrain(color["G"] | fallback.green, 0, 255)),
      static_cast<uint8_t>(constrain(color["B"] | fallback.blue, 0, 255)),
  };
}
}

void handleCommand(const String& payload) {
  JsonDocument document;
  if (deserializeJson(document, payload)) {
    Serial.println("[WS] Erro ao processar comando: JSON invalido.");
    publishStatus("command", "error", "invalid_json");
    return;
  }

  JsonVariantConst command = document["comando"];
  if (command.isNull()) {
    Serial.println("[WS] Erro ao processar comando: comando ausente.");
    publishStatus("command", "error", "missing_command");
    return;
  }

  if (command.is<const char*>()) {
    String action = command.as<String>();
    action.toLowerCase();
    if (action == "abrir") openDoor();
    else if (action == "fechar") closeDoor();
    else if (action == "luz") {
      internalLightEnabled = !internalLightEnabled;
      if (internalLightEnabled) turnInternalLightOn();
      else turnInternalLightOff();
    }
    else if (action == "apagar") {
      internalLightEnabled = false;
      turnAllLedsOff();
    }
    else if (action == "tranca_direita") openLock();
    else if (action == "tranca_esquerda") closeLock();
    else if (action == "correta") showCorrectAttempt(readColor(document["cor"], deviceConfig.teamColor));
    else if (action == "erro") showIncorrectAttempt();
    else {
      Serial.printf("[WS] Erro ao processar comando: comando desconhecido (%s).\n", action.c_str());
      publishStatus("command", "error", "unknown_command");
      return;
    }
    publishStatus("command", "ok", action);
    return;
  }

  JsonObjectConst attempt = command.as<JsonObjectConst>();
  if (!(attempt["tentando"] | false)) {
    showIncorrectAttempt();
    publishStatus("command", "ok", "incorrect_attempt");
    return;
  }
  const uint8_t stage = attempt["etapa"] | 1;
  if (stage == 0 || stage > deviceConfig.passwordCount) {
    Serial.println("[WS] Erro ao processar comando: etapa invalida.");
    publishStatus("command", "error", "invalid_stage");
    return;
  }
  showCorrectStage(stage, readColor(attempt["cor"], deviceConfig.teamColor));
  publishStatus("command", "ok", "correct_stage");
}

void handleConfiguration(const String& payload) {
  JsonDocument document;
  if (deserializeJson(document, payload)) {
    Serial.println("[WS] Erro ao processar configuracao: JSON invalido.");
    publishStatus("configuration", "error", "invalid_json");
    return;
  }

  bool changed = false;
  if (!document["angulo-max"].isNull()) {
    deviceConfig.doorCloseAngle = constrain(document["angulo-max"].as<int>(), 0, 180);
    changed = true;
  }
  if (!document["num_senhas"].isNull()) {
    deviceConfig.passwordCount = constrain(document["num_senhas"].as<int>(), 1, MAX_STAGES);
    resetStages();
    changed = true;
  }
  if (!document["cor_equipe"].isNull()) {
    deviceConfig.teamColor = readColor(document["cor_equipe"]["cor"], deviceConfig.teamColor);
    resetStages();
    applyTeamColor();
    changed = true;
  }
  if (changed) {
    saveSettings();
    publishStatus("configuration", "ok", "saved");
  } else {
    Serial.println("[WS] Erro ao processar configuracao: nenhum campo suportado.");
    publishStatus("configuration", "error", "no_supported_fields");
  }
}
